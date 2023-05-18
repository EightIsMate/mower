#include <MeAuriga.h>
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

// Defines
#define AURIGARINGLEDNUM 12
#define RINGALLLEDS 0

#define FORWARD 1
#define REVERSE 2
#define LEFT 3
#define RIGHT 4
#define STOP 5

#define REVERSEDURATION 1.5 * 1000 // 1,5
#define TURNINGDURATION 1.0 * 1000 // 1 sec

#define DURATION 2.0 * 1000 // 2 sek

#define MANUALSPEED 125

#define FOUND_OBJECT 5

#define ALIGNING 1
#define TAKEPICTURE 2
#define AVOIDING 3

#define GYRO_OFFSET 5.0

MeRGBLed led_ring(0, 12);
MeLineFollower lineFinder(PORT_9);
MeGyro gyro(0, 0x69);
MeUltrasonicSensor ultraSensor(PORT_7);
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);

const float WHEEL_DIAMETER = 40.0;                                        // Diameter of the wheels in mm
const float WHEEL_CIRCUMFERENCE = M_PI * WHEEL_DIAMETER;                  // Circumference of the wheels
const int ENCODER_RESOLUTION = 180;                                       // Encoder resolution (number of ticks per revolution)
const float DISTANCE_PER_TICK = WHEEL_CIRCUMFERENCE / ENCODER_RESOLUTION; // Distance traveled per encoder tick

// declared variables
bool calkGyroAngle = false;
bool hasntCrossedLineTwice = false;
bool avoidObstaclesInit = false;
bool doneAvoiding = false;
bool haveChosenRandomValue = false;
bool doneAligning = false;
bool doneTurning = false;
bool hasStopped = false;
bool isReversing = false;

int sensorState = 0;
long int reverseDuration = 0;
long int turningDuration = 0;
long int lineDepartureDelay = 0;

bool setDurations = false;
long int sendingDelay = 0;

float deltaX = 0.0;
float deltaY = 0.0;

int randomTurningNumber = 0;
char mowerMode[3] = " "; // ex. M10
char manualState = 0;
int avoidState = 0;
char doneTakingPicture = ' ';
int i = 0; // counter for message length received
int lineSensor = 0;
// Receiving desired angle from lidar
int receievedAngle = 0;
char objectIsClose = ' ';
char lidarAngle[3] = " "; // ex. 320

// Variable to know from which sensor the object is detected
char detectedBySensor = ' ';

// Variables for dead reckoning
float x = 0.0;
float y = 0.0;
float heading = 0.0;

float prev_x = 0.0;
float prev_y = 0.0;

// function declarations
void move(int direction, int speed);
void avoidObstacles();
void autoMow(); 
void manualMow(char direction, char turnDirection);
void objectDetected();
void update_position();
void isr_process_encoder1(void);
void isr_process_encoder2(void);
void serialFlush();

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.setTimeout(1);
    gyro.begin();
    attachInterrupt(Encoder_1.getIntNum(), isr_process_encoder1, RISING);
    attachInterrupt(Encoder_2.getIntNum(), isr_process_encoder2, RISING);
    // 12 LED Ring controller is on Auriga D44/PWM
    led_ring.setpin(44);


    while (!Serial)
    {
        ; // Wait for serial port to connect to raspberry pi
    }
  
    // Set PWM 8KHz
    TCCR1A = _BV(WGM10);
    TCCR1B = _BV(CS11) | _BV(WGM12);

    TCCR2A = _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS21);

    // read line follower sensor
    sensorState = lineFinder.readSensors();

    // Mower is Idle untill app comands it 
    mowerMode[0] = 'I';
    mowerMode[1] = '0';
    mowerMode[2] = '0';

    update_position();
    Serial.print(x);
    Serial.print(",");
    Serial.print(y);
    Serial.print(",");
    Serial.println(heading);
}

// put your main code here, to run repeatedly:
void loop()
{

    while (Serial.available() > 0)
    {
        char data = Serial.read();

        if (data != '\n')
        {
            if (isAlpha(data))
            {
                String letter = String(data);
                letter.toUpperCase();
                char letter_char[2];
                letter.toCharArray(letter_char, 2);

                if (letter_char[0] == 'K') // Pi is done taking a picture
                {
                    doneTakingPicture = letter_char[0];
                }
                else if (letter_char[0] == 'L') // object is 30 cm from mower,detected by lidar
                {
                    objectIsClose = letter_char[0];
                }
                else if (letter_char[0] == 'M')
                {
                    objectIsClose = ' '; // ignore lidar while in manual state
                    mowerMode[0] = letter_char[0];
                }
                else if (letter_char[0] == 'A') // mower's move modes
                {
                    mowerMode[0] = letter_char[0];
                }
                else if(letter_char[0] == 'I')
                {
                    mowerMode[0] = letter_char[0];
                    objectIsClose = ' '; //ignore lidar in this state
                }
                else if(letter_char[0] == 'R')
                {
                    serialFlush();
                    break;
                }
            }

            // if manual move mode chosen
            if (mowerMode[0] == 'M')
            {
                // fill array with bits from pi to determine direction
                if (i < 3)
                {
                    if (i != 0)
                        mowerMode[i] = data;
                    i += 1;
                }
            }
            else if (objectIsClose == 'L') // if an object is closer than 30cm from  the robot
            {
                // fill array with bits to determine lidar angle
                if (i < 3)
                {
                    if (data != 'L')
                    {
                        lidarAngle[i] = data;
                        // Serial.println(lidarAngle[i]);
                        i += 1;
                    }
                }
                else 
                {
                    // Serial.print("Before: ");
                    // Serial.println(detectedBySensor);
                    detectedBySensor = objectIsClose;
                    // Serial.print("After: ");
                    // Serial.println(detectedBySensor);
                    objectIsClose = ' ';
                }
                if (i == 3)
                {          
                    calkGyroAngle = true;
                    sensorState = FOUND_OBJECT;
                }
            }
        }
        else
        {
            // reset i
            i = 0;
        }
    }

    String inputMode = String(mowerMode[0]);
    inputMode.toUpperCase();

    if (inputMode == "M") // Controlling the mower manually
    {
        manualMow(mowerMode[1], mowerMode[2]);
        led_ring.setColor(RINGALLLEDS, 50, 50, 0);
        led_ring.show();
    }
    else if (inputMode == "A") // automode
    {
        autoMow();
        led_ring.setColor(RINGALLLEDS, 0, 0, 50);
        led_ring.show();
    }
    else if (inputMode == "I")
    {
        led_ring.setColor(RINGALLLEDS, 10, 250, 10);
        led_ring.show();
        move(STOP,0);
    }
    else // wrong input
    {
        move(STOP, 0);
        led_ring.setColor(RINGALLLEDS, 50, 0, 0);
        led_ring.show();
        hasStopped = true;
    }

    // Serial.println(String(raspCom));
    /*
    //LEDRING
    switch (raspCom)
    {
    case '0':
      // all LEDs off
      led_ring.setColor(RINGALLLEDS, 0, 0, 0);
      led_ring.show();
      delay(500);
      Serial.write("A", 1);
      break;

    case '1':
      led_ring.setColor(RINGALLLEDS, 0, 50, 0);
      led_ring.show();
      delay(500);
      Serial.write("A", 1);
    default:
      Serial.write("E", 1);
      break;
    }

    */
    Encoder_1.loop();
    Encoder_2.loop();
    update_position();
    // Send the position data to the backend server
    // Check if x or y has changed
    if (x != prev_x || y != prev_y)
    {
        deltaX = abs(x - prev_x);
        deltaY = abs(y - prev_y);
        // if (deltaX != 0.0 || deltaY != 0.0)
        // {
        //     Serial.print("deltaX: ");
        //     Serial.println(deltaX);
        //     Serial.print("deltaY: ");
        //     Serial.println(deltaY);
        // }

        if (setDurations == false)
        {
            sendingDelay = millis() + 1000;
            setDurations = true;
        }

        if (millis() > sendingDelay)
        {
            // Send the position data to the backend server
            Serial.print(x);
            Serial.print(",");
            Serial.print(y);
            Serial.print(",");
            Serial.println(heading);
            setDurations = false;
        }

        // Update the previous values of x and y
        prev_x = x;
        prev_y = y;
    }

} //--------end of loop--------------

// A function to control the movement direction and speed of the mower
void move(int direction, int speed)
{
    int leftSpeed = 0;
    int rightSpeed = 0;

    if (direction == FORWARD)
    {
        leftSpeed = -speed;
        rightSpeed = speed;
        hasStopped = false;
        isReversing = false;
    }
    else if (direction == REVERSE)
    {
        leftSpeed = speed;
        rightSpeed = -speed;
        hasStopped = false;
        isReversing = true;
    }
    else if (direction == LEFT)
    {
        leftSpeed = -speed;
        rightSpeed = -speed;
        hasStopped = false;
        isReversing = false;
    }
    else if (direction == RIGHT)
    {
        leftSpeed = speed;
        rightSpeed = speed;
        hasStopped = false;
        isReversing = false;
    }
    else if (direction == STOP)
    {
        leftSpeed = 0;
        rightSpeed = 0;
        hasStopped = true;
        isReversing = false;
    }

    Encoder_1.setMotorPwm(leftSpeed);
    Encoder_2.setMotorPwm(rightSpeed);
    // update_position();
}

void avoidObstacles()
{
    if (avoidObstaclesInit == false)
    {
        avoidObstaclesInit = true;
        reverseDuration = millis() + REVERSEDURATION;
        turningDuration = millis() + TURNINGDURATION + REVERSEDURATION;
        lineDepartureDelay = millis() + 200;
        lineSensor = sensorState;
        hasntCrossedLineTwice = true;
        doneAvoiding = false;
    }

    if (millis() < reverseDuration && hasntCrossedLineTwice == true)
    {
        move(REVERSE, 125);
    }
    else if ((millis() >= reverseDuration) && (millis() < turningDuration) && hasntCrossedLineTwice == true)
    {
        if (haveChosenRandomValue == false)
        {
            haveChosenRandomValue = true;
            // Turn left in a random speed
            randomTurningNumber = random(80, 101);
        }

        move(LEFT, randomTurningNumber);
    }
    else
    {
        doneAvoiding = true;
    }

    if (lineSensor != lineFinder.readSensors())
    {
        lineSensor = lineFinder.readSensors();
    }

    // avoid crossing line twice
    if (hasntCrossedLineTwice == true && millis() > lineDepartureDelay)
    {
        if (lineSensor != S1_OUT_S2_OUT)
        {
            hasntCrossedLineTwice = false;
            lineDepartureDelay = millis() + 300; // give it some time to move to the confined area
            move(FORWARD, 125);
        }
    }

    if ((doneAvoiding == true && hasntCrossedLineTwice == true) || (hasntCrossedLineTwice == false && millis() > lineDepartureDelay))
    {
        move(FORWARD, 125);
        sensorState = S1_OUT_S2_OUT;
    }
}

void autoMow()
{
    switch (sensorState)
    {
    case S1_IN_S2_OUT:
        avoidObstacles();
        break;

    case S1_OUT_S2_IN:
        avoidObstacles();
        break;

    case S1_IN_S2_IN:
        avoidObstacles();
        break;

    case S1_OUT_S2_OUT:
        avoidObstaclesInit = false;
        haveChosenRandomValue = false;
        hasntCrossedLineTwice = true;

        move(FORWARD, 125);

        if (sensorState != lineFinder.readSensors())
        {
            sensorState = lineFinder.readSensors();
        }
        else if (detectedBySensor != 'L' && ultraSensor.distanceCm() <= 15)
        {
            detectedBySensor = 'U'; // UltraSonicSensor
            sensorState = FOUND_OBJECT;
        }
        break;

    case FOUND_OBJECT:
        objectDetected();
        detectedBySensor = ' ';
        break;

    default:
        break;
    }
}

void manualMow(char direction, char turnDirection)
{
    // Serial.println("im mowing manually now hihi");

    int intDirection = String(direction).toInt();

    int intTurnDirection = String(turnDirection).toInt();

    int manualState = intDirection * 10 + intTurnDirection;

    int leftSpeed = 0;
    int rightSpeed = 0;

    switch (manualState)
    {
    case 10: // forward
        leftSpeed = -MANUALSPEED;
        rightSpeed = MANUALSPEED;
        hasStopped = false;
        isReversing = false;

        // Serial.println("im moving forward");
        break;

    case 20: // Reverse
        leftSpeed = MANUALSPEED;
        rightSpeed = -MANUALSPEED;

        hasStopped = false;
        isReversing = true;
        // Serial.println("im moving backward");
        break;

    case 13: // forward_left
        leftSpeed = -MANUALSPEED;
        rightSpeed = MANUALSPEED / 3;
        hasStopped = false;
        isReversing = false;

        // Serial.println("im moving forward_left");
        break;

    case 14: // forward_right
        leftSpeed = -MANUALSPEED / 3;
        rightSpeed = MANUALSPEED;
        hasStopped = false;
        isReversing = false;
        // Serial.println("im moving forward_right");
        break;

    case 23: // reverse_left
        leftSpeed = MANUALSPEED;
        rightSpeed = -MANUALSPEED / 3;
        hasStopped = false;
        isReversing = false;
        // Serial.println("im moving reverse_left");
        break;

    case 24: // reverse_right
        leftSpeed = MANUALSPEED / 3;
        rightSpeed = -MANUALSPEED;
        hasStopped = false;
        isReversing = false;
        // Serial.println("im moving reverse_right");
        break;

    case 03: // moving left
        leftSpeed = -MANUALSPEED;
        rightSpeed = -MANUALSPEED;
        hasStopped = false;
        isReversing = false;
        // Serial.println("im moving left")
        break;

    case 04: // right
        leftSpeed = MANUALSPEED;
        rightSpeed = MANUALSPEED;
        hasStopped = false;
        isReversing = false;
        // Serial.println("im moving right")
        break;

    case 00: // stop
        leftSpeed = 0;
        rightSpeed = 0;
        hasStopped = true;
        isReversing = false;
        // Serial.println("im stopping");
        break;

    default:
        break;
    }
    Encoder_1.setMotorPwm(leftSpeed);
    Encoder_2.setMotorPwm(rightSpeed);
}

// used when the mower detects an object inside the confined area
void objectDetected()
{
    // Serial.println("Found object");

    // get current gyro angle
    gyro.update();

    // print distance from low object detected by ultrasonic sensor
    //  Serial.print("Ultrasonic distance : ");
    //  Serial.print(ultraSensor.distanceCm());
    //  Serial.println(" cm");

    float CurrentgyroAngleZ = gyro.getAngleZ();

    // avoidState = AVOIDING; // For debugging since we do not have code in ALIGNING state

    if (doneAligning == false )
    {
        avoidState = ALIGNING;
    }

    switch (avoidState)
    {

    case ALIGNING:

        move(STOP, 0);
        if (detectedBySensor == 'U')
        {
            avoidState = TAKEPICTURE;
            doneAligning = true;
        }
        else{
            // Serial.print("currentgyroAngleZ: ");
            // Serial.println(CurrentgyroAngleZ);
            float getLidarAngle;
            float turningGyroAngleZ;    // converting a lidarangle to a gyroangle
            float newDesiredGyroAngleZ; // the desired angle that we want to turn to

            // convert the lidarvalues to float in order to compare with gyrovalues
            getLidarAngle = atof(lidarAngle);

            if (getLidarAngle > 180) // on the left side
            {
                turningGyroAngleZ = -(360 - getLidarAngle);
            }
            else // on the right
            {
                turningGyroAngleZ = getLidarAngle;
            }

            if (calkGyroAngle == true)
            {
                newDesiredGyroAngleZ = CurrentgyroAngleZ + turningGyroAngleZ;
                calkGyroAngle = false;
            }

            if (newDesiredGyroAngleZ > 180)
            {
                newDesiredGyroAngleZ = 180 - newDesiredGyroAngleZ;
            }
            else if (newDesiredGyroAngleZ < (-180))
            {
                newDesiredGyroAngleZ + 360;
            }

            // Serial.print("The desired angle of Gyroscope is :");
            // Serial.println(newDesiredGyroAngleZ);
            // Serial.print("Current angle of Z:");
            // Serial.println(CurrentgyroAngleZ);

            // if already aligned
            if (CurrentgyroAngleZ > (newDesiredGyroAngleZ - 2) && CurrentgyroAngleZ < (newDesiredGyroAngleZ + 2))
            {
                move(STOP, 0);
                doneAligning = true;
                // Serial.println("doneAligning first time");
                // Serial.println(doneAligning);
                avoidState = TAKEPICTURE;
            }
            else // not aligned yet
            {
                // use lidar angle to determine turning left or right
                if (getLidarAngle > 180)
                    move(LEFT, 200);
                else
                    move(RIGHT, 200);
            }
        }


        break;

    case TAKEPICTURE:
        
        move(STOP, 0);

        // set next state if pi is done taking picture
        Serial.println("P"); // tell pi to take picture

        if (doneTakingPicture == 'K')
        { 
            doneTakingPicture = ' ';
            avoidState = AVOIDING;
        }

        break;

    case AVOIDING:
    
        led_ring.setColor(RINGALLLEDS, 100, 100, 100);
        led_ring.show();
        delay(500);
        avoidState = 0;
        doneAligning = false;
        sensorState = S1_IN_S2_OUT; // to prevent entering to aligning again
        // doneAligning = false;
        avoidObstacles();
        // break;
        break;

    default:
        return;
    }
}
void isr_process_encoder1(void)
{
    if (digitalRead(Encoder_1.getPortB()) == 0)
    {
        Encoder_1.pulsePosMinus();
    }
    else
    {
        Encoder_1.pulsePosPlus();
    }
}

void isr_process_encoder2(void)
{
    if (digitalRead(Encoder_2.getPortB()) == 0)
    {
        Encoder_2.pulsePosMinus();
    }
    else
    {
        Encoder_2.pulsePosPlus();
    }
}

void update_position()
{

    // Get the change in encoder ticks
    int left_ticks = Encoder_1.getPulsePos();
    int right_ticks = Encoder_2.getPulsePos();

    // Calculate the distance traveled by each wheel
    float left_distance = left_ticks * DISTANCE_PER_TICK;
    float right_distance = right_ticks * DISTANCE_PER_TICK;

    float distance = 0.0;
    // Calculate the average distance traveled
    if (left_distance + right_distance == 0 && hasStopped == false)
    {
        if (isReversing == true)
        {
            distance -= 1.0;
        }
        else
        {
            distance += 1.0;
        }
    }
    else
    {
        distance = (left_distance + right_distance) / 2;
    }
    // Update the gyro heading
    gyro.update();
    heading = gyro.getAngleZ();

    // Calculate the change in x and y based on the distance and heading
    float delta_x = distance * cos(heading * M_PI / 180);
    float delta_y = distance * sin(heading * M_PI / 180);
    // float delta_x = left_distance * cos(heading * M_PI / 180);
    // float delta_y = left_distance * sin(heading * M_PI / 180);

    // Update the position
    x += delta_x;
    y += delta_y;

    // Reset the encoder ticks
    Encoder_1.setPulsePos(0);
    Encoder_2.setPulsePos(0);
}

void serialFlush(){
     while (Serial.available() > 0)
    {
        char data = Serial.read();
    }
}
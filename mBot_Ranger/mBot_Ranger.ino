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
#define TURNINGDURATION 1.0 * 1000 // 1sec

#define MANUALSPEED 125

#define FOUND_OBJECT 5

#define ALIGNING 1
#define TAKEPICTURE 2
#define AVOIDING 3


MeRGBLed led_ring(0, 12);
MeLineFollower lineFinder(PORT_9);
MeGyro gyro(0, 0x69); 
MeUltrasonicSensor ultraSensor(PORT_7);
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);

const float WHEEL_DIAMETER = 4.0;    // Diameter of the wheels in cm
const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER;  // Circumference of the wheels
const int ENCODER_RESOLUTION = 180;  // Encoder resolution (number of ticks per revolution)
const float DISTANCE_PER_TICK = WHEEL_CIRCUMFERENCE / ENCODER_RESOLUTION;  // Distance traveled per encoder tick

//declared variables
bool hasntCrossedLineTwice = false;
bool avoidObstaclesInit = false;
bool doneAvoiding = false;
bool haveChosenRandomValue = false;
bool doneAligning = false;
bool doneTurning = false;
int sensorState = 0;
long int reverseDuration = 0;
long int turningDuration = 0;
long int lineDepartureDelay = 0;
int randomTurningNumber = 0;
char mowerMode[3] = " "; // array to save bits from pi to mower
char manualState = 0;
int avoidState = 0;
char doneTakingPicture = ' ';
int i = 0; // counter for message length received
int lineSensor = 0;
// Receiving desired angle from lidar
int receievedAngle = 0;
char doneSwtichingAngle = ' ';

//Variables for dead reckoning
float x = 0.0;
float y = 0.0;
float heading = 0.0;

float prev_x = 0.0;
float prev_y = 0.0;

//function declarations
void move(int direction, int speed);
void avoidObstacles();
void autoMow();
void manualMow(char direction, char turnDirection);
void objectDetected();
void update_position();
void isr_process_encoder1(void);
void isr_process_encoder2(void);

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
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

    mowerMode[0] = 'A';
    mowerMode[1] = '0';
    mowerMode[2] = '0';
}

// put your main code here, to run repeatedly:
void loop()
{
    while (Serial.available() > 0)
    {
        char data = Serial.read();

        if (data != '\n')
        {
            if (data == 'K') //Pi is done taking a picture
            {
                doneTakingPicture = data;
            }
            else if (data == 'L')
            {
                doneSwtichingAngle = data;
                objectDetected();
            }
            else
            {        
                // fill array with bits from pi
                if (i < 3)
                {
                    mowerMode[i] = data;
                    i +=1;
                } 
            }
        }
        else{
            i = 0;
        }
    }
   
    /*
    Serial.print("mowerMode[0]: ");
    Serial.println(String(mowerMode[0]));
    Serial.print("mowerMode[1]: ");
    Serial.println(String(mowerMode[1]));
    Serial.print("mowerMode[2]: ");
    Serial.println(String(mowerMode[2]));
    */
    

    String inputMode = String(mowerMode[0]);
    inputMode.toUpperCase();
    // Serial.println(inputMode);

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
    else // wrong input
    {
        move(STOP, 0);
        led_ring.setColor(RINGALLLEDS, 50, 0, 0);
        led_ring.show();
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
        // Send the position data to the backend server    
        Serial.print(x);
        Serial.print(",");
        Serial.print(y);
        Serial.print(",");
        Serial.println(heading);

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
    }
    else if (direction == REVERSE)
    {
        leftSpeed = speed;
        rightSpeed = -speed;
    }
    else if (direction == LEFT)
    {
        leftSpeed = -speed;
        rightSpeed = -speed;
    }
    else if (direction == RIGHT)
    {
        leftSpeed = speed;
        rightSpeed = speed;
    }
    else if (direction == STOP)
    {
        leftSpeed = 0;
        rightSpeed = 0;
    }
    
    Encoder_1.setMotorPwm(leftSpeed);
    Encoder_2.setMotorPwm(rightSpeed);
    //update_position();
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
    if(hasntCrossedLineTwice == true && millis() > lineDepartureDelay){

        if (lineSensor != S1_OUT_S2_OUT )
        {
            hasntCrossedLineTwice = false;
            lineDepartureDelay = millis() + 300; // give it some time to move to the confined area
            move(FORWARD, 125);
        }
    }

    if ((doneAvoiding == true && hasntCrossedLineTwice == true)|| (hasntCrossedLineTwice == false && millis() > lineDepartureDelay))
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

        move(FORWARD, 125 );

        if (sensorState != lineFinder.readSensors())
        {
            sensorState = lineFinder.readSensors(); 
        }
        // else if(ultraSensor.distanceCm() <= 30 /*or lidar gives angle directions*/)
        // {
        //     sensorState = FOUND_OBJECT;
        // }
        break;

    case FOUND_OBJECT:
        objectDetected();
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
        break;

    case 20: // AVOIDING
        leftSpeed = MANUALSPEED;
        rightSpeed = -MANUALSPEED;
        break;

    case 13: // forward_left
        leftSpeed = -MANUALSPEED;
        rightSpeed = MANUALSPEED / 3;
        break;

    case 14: // forward_right
        leftSpeed = -MANUALSPEED / 3;
        rightSpeed = MANUALSPEED;
        break;

    case 23: // reverse_left
        leftSpeed = MANUALSPEED;
        rightSpeed = -MANUALSPEED / 3;
        break;

    case 24: // reverse_right
        leftSpeed = MANUALSPEED / 3;
        rightSpeed = -MANUALSPEED;
        break;

    case 03: // moving left
        leftSpeed = -MANUALSPEED;
        rightSpeed = -MANUALSPEED;
        break;

    case 04: // right
        leftSpeed = MANUALSPEED;
        rightSpeed = MANUALSPEED;
        break;

    case 00: // stop
        leftSpeed = 0;
        rightSpeed = 0;
        break;

    default:
        break;
    }
    Encoder_1.setMotorPwm(leftSpeed);
    Encoder_2.setMotorPwm(rightSpeed);
}


//used when the mower detects an object inside the confined area
void objectDetected()
{
    Serial.println("Found object");

    //get gyro angle
    gyro.update(); 
    Serial.print(" Z:");
    Serial.println(gyro.getAngleZ());
    
    Serial.print("Distance : ");
    Serial.print(ultraSensor.distanceCm());
    Serial.println(" cm");

    float CurrentgyroAngleZ = gyro.getAngleZ();
    // line up with the object in detection area to take picture then back and turn
    // if object is detected either via lidar or ultrasonic sensor
    // be infront of the object
    // take picture
    // done taking photo?
    // if yes
    // backing
    // turning
    // continue forward
    // avoidState = AVOIDING; // For debugging since we do not have code in ALIGNING state
    avoidState = ALIGNING;
    switch (avoidState)
    {
    case ALIGNING:
        if (doneSwtichingAngle == 'L'){ //make sure do not let lidar keep sending this message or the robot will dance
            Serial.write("R",1);
        }
            avoidState = AVOIDING;
        //if aligning done then go to TAKEPICTURE state
        // if( alignedGyroValue != gyroValue)  //-z is left min. -180 and z is right with max. 180 degrees
        // {
                // if (CurrentgyroAngleZ > 43 && CurrentgyroAngleZ < 47){
                    // center to the desired angle
                //     move(STOP, 0);
                // } 
                // else{
                //     move(LEFT, 200);
                // }
        //     //align the robot
        // } else {
        //     doneAligning = true;
        // }

        // if(doneAligning == true){
        //     avoidState = TAKEPICTURE;  
        // }
        
        break;
        
    case TAKEPICTURE:
        //stop for a few sec and then take a picture of the obstacle. tell raspberrypi to take a picture
        move(STOP,0);
        Serial.write("P",1); //tell pi to take picture

        // set next state if pi is done taking picture
        if (doneTakingPicture == 'K'){
            doneTakingPicture = ' ';
            avoidState = AVOIDING;
        } 
            
        break; 
    
    case AVOIDING:
        //  if (doneSwtichingAngle == 'L'){
        //     Serial.write("R",1);
        //     doneSwtichingAngle = ' ';
        // }
        avoidObstacles();
        avoidState = 0;
        break;
    
    default:
        break;
    }
}
void isr_process_encoder1(void)
{
  if(digitalRead(Encoder_1.getPortB()) == 0)
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
  if(digitalRead(Encoder_2.getPortB()) == 0)
  {
    Encoder_2.pulsePosMinus();
  }
  else
  {
    Encoder_2.pulsePosPlus();
  }
}

void update_position(){

    // Get the change in encoder ticks
    int left_ticks = Encoder_1.getPulsePos();
    int right_ticks = Encoder_2.getPulsePos();

    // Calculate the distance traveled by each wheel
    float left_distance = left_ticks * DISTANCE_PER_TICK;
    float right_distance = right_ticks * DISTANCE_PER_TICK;

    // Calculate the average distance traveled
    float distance = (left_distance + right_distance) / 2;

    // Update the gyro heading
    gyro.update();
    heading = gyro.getAngleZ();

    // Calculate the change in x and y based on the distance and heading
    float delta_x = distance * cos(heading * M_PI / 180);
    float delta_y = distance * sin(heading * M_PI / 180);

    // Update the position
    x += delta_x;
    y += delta_y;

    // Reset the encoder ticks
    Encoder_1.setPulsePos(0);
    Encoder_2.setPulsePos(0);
}
#include <MeAuriga.h>
#include <Arduino.h>
#include <Wire.h>

//Defines
#define AURIGARINGLEDNUM 12
#define RINGALLLEDS 0

#define FORWARD 1
#define REVERSE 2
#define LEFT 3
#define RIGHT 4
#define STOP 5

#define REVERSEDURATION 1.5 * 1000 //1,5
#define TURNINGDURATION 1.0 * 1000  //1sec
#ifdef MeAuriga_H

// on-board LED ring, at PORT0 (onboard), with 12 LEDs
MeRGBLed led_ring(0, 12);

#endif

// linefollower sensor
MeLineFollower lineFinder(PORT_9);

MeGyro gyro(0, 0x69);
MeUltrasonicSensor ultraSensor(PORT_7);
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);

bool foundLine = false; 
bool doneAvoiding = false;
bool haveChosenRandomValues = false;
int sensorState = 0;
long int reverseDuration = 0;
long int turningDuration = 0;
int randomTurningNumber = 0;
int randomTurningDirection = 0;
char mowerMode[3] = " "; //array to save bits from pi to mower

void moveForward();
void moveBackwards();
void turnRandomLeftAndBacking();
void move(int direction, int speed);
void moveDuration(float seconds);
void avoidCrossingLine();
void autoMow();
void manualMow();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  gyro.begin();

#ifdef MeAuriga_H
  // 12 LED Ring controller is on Auriga D44/PWM
  led_ring.setpin(44);
#endif

  while (!Serial)
  {
    ; // Wait for serial port to connect to raspberry pi
  }

  // Set PWM 8KHz
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);

  //read line follower sensor
  sensorState = lineFinder.readSensors();
  mowerMode[0] = 'A';
  mowerMode[1] = '0';
  mowerMode[2] = '0';
}

// put your main code here, to run repeatedly:
void loop()
{

  while(Serial.available() > 0)
  {
    //fill array with bits from pi
    for(int i=0; i <= 2; i ++)
    {
      mowerMode[i] = Serial.read();
      //Serial.print("mowerMode: ");
    }
  }

  Serial.println(String(mowerMode[0]));

  if(mowerMode[0] == 'M') //Controlling the mower manually 
  {
    manualMow();
    led_ring.setColor(RINGALLLEDS, 0, 0, 0);
    led_ring.show();
  }
  else if(mowerMode[0] == 'A') //automode chosen
  {
    autoMow();
    led_ring.setColor(RINGALLLEDS, 0, 0, 0);
    led_ring.show();
  }
  else //wrong input
  {
    move(STOP, 0); 
    led_ring.setColor(RINGALLLEDS, 50, 0, 0);
    led_ring.show();
    
  }

  //autoMow();
    
    //Serial.println(String(raspCom));
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
  
  // read distance  in cm
  /*
  Serial.print("Distance : ");
  Serial.print(ultraSensor.distanceCm() );
  Serial.println(" cm");*/
  


}//--------end of loop--------------

void moveForward()
{
  Encoder_1.setMotorPwm(-100);
  Encoder_2.setMotorPwm(100);
}

void moveBackwards()
{
   long int start = millis();
   long int end = start + 2000; //2 seconds later

  Encoder_1.setMotorPwm(100);
  Encoder_2.setMotorPwm(-100);
}

void turnRandomLeftAndBacking()
{ // 45 degrees turn left backward
  int randomNumber = random(100, 200);

  Encoder_1.setMotorPwm(100);
  Encoder_2.setMotorPwm(-randomNumber);
}

//A function to control the movement direction and speed of the mower
void move(int direction, int speed)
{
  int leftSpeed = 0;
  int rightSpeed = 0;

  if(direction == FORWARD)
  {  
    leftSpeed = -speed;
    rightSpeed = speed;
  }
  else if(direction == REVERSE)
  {
    leftSpeed = speed;
    rightSpeed = -speed;
  }
  else if(direction == LEFT)
  {
    leftSpeed = -speed;
    rightSpeed = -speed;
  }
  else if(direction == RIGHT)
  {
    leftSpeed = speed;
    rightSpeed = speed;
  }
  else if(direction == STOP)
  {
    leftSpeed = 0;
    rightSpeed = 0;
  }

  Encoder_1.setMotorPwm(leftSpeed);
  Encoder_2.setMotorPwm(rightSpeed);
}


// A function to controll the duration of the movements
void moveDuration(float seconds)
{
  //avoid negative values
  if(seconds < 0.0)
  {
    seconds = 0.0;
  }

  unsigned long endTime = millis() + seconds * 1000;

  // run until the current time reaches endTime
  while(millis() < endTime)
  {
    ReadAndPrintGyro();
    Encoder_1.loop();
    Encoder_2.loop();
    //delay(100);
  }
}

void ReadAndPrintGyro()
{  
  gyro.update();  
  Serial.print(" X:");
  Serial.println( gyro.getAngleX());
  Serial.print(" Y:");
  Serial.println(gyro.getAngleY());
  Serial.print(" Z:");
  Serial.println(gyro.getAngleZ());
}

void avoidCrossingLine(){
      if (foundLine == false)
    {
      foundLine = true;
      reverseDuration = millis() + REVERSEDURATION; 
      turningDuration = millis() + TURNINGDURATION + REVERSEDURATION;
      doneAvoiding = false;
    }

    if(millis() < reverseDuration)
    {
      move(REVERSE, 125);
    }
    else if ((millis() >= reverseDuration) && (millis() < turningDuration))
    {
      if (haveChosenRandomValues == false)
      {
        haveChosenRandomValues = true;
        //Turn random direction in a random speed
        randomTurningNumber = random(75, 101);
        //Max random value exclusive ,hence the +1
        randomTurningDirection = random(LEFT, RIGHT +1);
      }
      
      move(LEFT, randomTurningNumber);
    }
    else{
      doneAvoiding = true;
    }

    if (doneAvoiding == true)
    {
      sensorState = S1_OUT_S2_OUT;
    }
}

void autoMow()
{
    switch (sensorState)
  {
  case S1_IN_S2_OUT:
    avoidCrossingLine();
    break;
  case S1_OUT_S2_IN:
    avoidCrossingLine();
    break;
  case S1_IN_S2_IN:
    avoidCrossingLine();
    break;
  
  case S1_OUT_S2_OUT:
    foundLine = false;
    haveChosenRandomValues = false;
    move(FORWARD, 125);
    if (sensorState != lineFinder.readSensors())
    {
      sensorState = lineFinder.readSensors();
    }
    break;
  default:
    break;
  }

  Encoder_1.loop();
  Encoder_2.loop();
}

void manualMow()
{
    move(STOP,0);
    Serial.println("im mowing manually now hihi");
}

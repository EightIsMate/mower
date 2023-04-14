#include <MeAuriga.h>
#include <Arduino.h>
#include <Wire.h>

#define AURIGARINGLEDNUM  12
#define RINGALLLEDS        0

#ifdef MeAuriga_H

// on-board LED ring, at PORT0 (onboard), with 12 LEDs
MeRGBLed led_ring( 0, 12 );

#endif

//linefollower sensor 
MeLineFollower lineFinder(PORT_9);

MeGyro gyro(0, 0x69);
MeUltrasonicSensor ultraSensor(PORT_7);
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);

bool isOnEdge = false;
bool foundLine = false;
int sensorState = 0;

void avoidObject();
void moveForward();
void moveBackwards();
void turnRandomLeftAndBacking();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  gyro.begin();
  
  #ifdef MeAuriga_H
    // 12 LED Ring controller is on Auriga D44/PWM
    led_ring.setpin( 44 );
  #endif
  
  while (!Serial)
  {
    ; //Wait for serial port to connect to raspberry pi
  }
  
  //Set PWM 8KHz
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);
}

// put your main code here, to run repeatedly:
void loop() { 
  
  //communication with raspberry pi
  char raspCom;

  if (Serial.available() > 0)
  {  
    raspCom = Serial.read();
    switch (raspCom)
    {
    case '0':
      // all LEDs off
      led_ring.setColor( RINGALLLEDS, 0, 0, 0 );
      led_ring.show();
      delay(500);
      Serial.write("A", 1);
      break;
    
    case '1':
      led_ring.setColor( RINGALLLEDS, 0, 50, 0 );
      led_ring.show();
      delay(500);
      Serial.write("A", 1);
    default:
    Serial.write("E", 1);
      break;
    }
  }

  //read distance in cm
  /*
  Serial.print("Distance : ");
  Serial.print(ultraSensor.distanceCm() );
  Serial.println(" cm");*/

  //read and print Z coordinate from gyro
  gyro.update();
  /*
  Serial.read();
  Serial.print(" Z:");
  Serial.println(gyro.getAngleZ() );*/
  
  int x = 0;
  int y = 0;
  /*

  if(x != int(gyro.getAngleX()) || y != int(gyro.getAngleY())){
    x = gyro.getAngleX();
    y = gyro.getAngleY();
    Serial.print(" X:");
    Serial.println(x ); 
    Serial.print(" Y:");
    Serial.println(y );  
  }
  

    x = gyro.getAngleX();
    y = gyro.getAngleY();
    Serial.print(" X:");
    Serial.println(x ); 
    Serial.print(" Y:");
    Serial.println(y );  
*/

/*
  if (ultraSensor.distanceCm() <= 5)
  {
    long int start = millis();
    long int end = start + 4000;
    long int doneBacking = start + 2000;
    
    while(end >= millis()){
      if(doneBacking >= millis()){
        moveBackwards();
      }
      else
      { 
        turnRandomLeftAndBacking();
      }
      
    }
    //Serial.println("I left the loop");
    
  }
  //Serial.println("I left the if statement and :" + String(ultraSensor.distanceCm()));
  */

  Encoder_1.loop();
  Encoder_2.loop();
  delay(100); /* the minimal measure interval is 100 milliseconds */

  //read the line follower sensors
  //long int now = millis(); 
  sensorState = lineFinder.readSensors();

 // Serial.println(sensorState);

  // only one of the sensors on black line
  if ((sensorState == S1_IN_S2_OUT )|| (sensorState == S1_OUT_S2_IN)){
    isOnEdge = true;
  }

  //stay inside confined area
  if((sensorState == S1_IN_S2_IN) || (isOnEdge == true)){
    turnRandomLeftAndBacking();
    isOnEdge = false;
  }else{
    moveForward();
  }
}

void moveForward(){
  Encoder_1.setMotorPwm(-100);
  Encoder_2.setMotorPwm(100);
}

void moveBackwards(){
  Encoder_1.setMotorPwm(100);
  Encoder_2.setMotorPwm(-100);
} 

void avoidObject(){
  
  while (ultraSensor.distanceCm() <= 15)
  {
    moveBackwards();
  }

  turnRandomLeftAndBacking();
  
}

void turnRandomLeftAndBacking(){ //45 degrees turn left backward
  int randomNumber = random(100, 200);

  Encoder_1.setMotorPwm(100);
  Encoder_2.setMotorPwm(-randomNumber);

}


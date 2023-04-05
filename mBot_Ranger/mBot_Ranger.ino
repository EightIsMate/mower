#include <MeAuriga.h>
#include <Arduino.h>
#include <Wire.h>

MeGyro gyro(0, 0x69);
MeUltrasonicSensor ultraSensor(PORT_7);
MeEncoderOnBoard Encoder_1(SLOT1);
MeEncoderOnBoard Encoder_2(SLOT2);

void avoidObject();
void moveForward();
void moveBackwards();
void turnRandomLeftAndBacking();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  gyro.begin();
  
  //Set PWM 8KHz
  TCCR1A = _BV(WGM10);
  TCCR1B = _BV(CS11) | _BV(WGM12);

  TCCR2A = _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);
}

void loop() { 
  // put your main code here, to run repeatedly:

  //read distance in cm
  Serial.print("Distance : ");
  Serial.print(ultraSensor.distanceCm() );
  Serial.println(" cm");

  //read and print Z coordinate from gyro
  gyro.update();
  Serial.read();
  Serial.print(" Z:");
  Serial.println(gyro.getAngleZ() );
  delay(10);

  moveForward();
  // Serial.print("Speed 1:");
  // Serial.print(Encoder_1.getCurrentSpeed());
  // Serial.print(" ,Speed 2:");
  // Serial.println(Encoder_2.getCurrentSpeed());


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
    Serial.println("I left the loop");
    
  }
  Serial.println("I left the if statement and :" + String(ultraSensor.distanceCm()));



  Encoder_1.loop();
  Encoder_2.loop();
  delay(100); /* the minimal measure interval is 100 milliseconds */
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
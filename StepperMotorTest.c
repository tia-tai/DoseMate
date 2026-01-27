#include <Stepper.h>

const int stepsPerRevolution = 64; 

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 9, 10, 11);

int stepCount = 0;

void setup() {
  
}

void loop() {
  // read the sensor value:
  int sensorReading = analogRead(A0); // This is a pot
  // map it to a range from 0 to 100:
  int motorSpeed = map(sensorReading, 0, 1023, 0, 100);
  // set the motor speed:
  if (motorSpeed > 0) {
    myStepper.setSpeed(motorSpeed);
    // step 1/100 of a revolution:
    myStepper.step(stepsPerRevolution / 100);
  } // Goal: figure out how many revolutions it takes to dispense a singular pill. 
}

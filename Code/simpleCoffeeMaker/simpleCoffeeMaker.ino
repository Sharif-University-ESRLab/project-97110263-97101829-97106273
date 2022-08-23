#include <Servo.h>


Servo myservo;  // create servo object to control a servo

int closeDegree = 170;          // variable to store the servo position for closing
int openDegree = 90;            // variable to store the servo position for opening
int armaturePin = 13;           // variable to set the pin of HIGH and LOW voltage for turn on and off armature
int waterPin = 7;               // variable to set the pin of HIGH and LOW voltage for open and close water tank
int servoDataPin = 9;           // variable to set the pin of data/degree for servo
int coffeePowerPin = 8;         // variable to set the pin of HIGH and LOW voltage for turn on and off coffee power
int startPin = 10;              // variable to read the pin for starting coffee process

int coffeeLevel = 3;            //
int waterLevel = 2;             //
int coffeeOpenTime = 2000;      // time (ms) that coffee tank is open
int waterOpenTime = 5000;       // time (ms) that water tank is open
int coffeeTurnOnTime = 100000;  // time (ms) that coffee maker must be on and boiling


void setCoffeePower(bool isOn) {
    // 1 for on and 0 for off
    if (isOn) {
      digitalWrite(coffeePowerPin, HIGH);
    } else {
      digitalWrite(coffeePowerPin, LOW);
    }
}

void setCoffeeTank(bool isOpen) {
    // 1 for open and 0 for close
    if (isOpen) {
      myservo.write(openDegree);
    } else {
      myservo.write(closeDegree);
    }
}

void setWaterTank(bool isOpen) {
    // 1 for open and 0 for close
    if (isOpen) {
      digitalWrite(waterPin, HIGH);
    } else {
      digitalWrite(waterPin, LOW);
    }
}

void setArmature(bool isOn) {
    // 1 for on and 0 for off
    if (isOn) {
      digitalWrite(armaturePin, HIGH);
    } else {
      digitalWrite(armaturePin, LOW);
    }
}

void drainCoffee() {
    ///////////////////##########//////////////////////
    setCoffeeTank(true);      // Open the coffee tank
    delay(100);               // Small wait
    setArmature(true);        // Turn on armature
    
    delay(coffeeOpenTime);    // Waiting...
    
    setArmature(false);       // Turn off armature
    delay(100);               // Small wait
    setCoffeeTank(false);     // Close the coffee tank
    ///////////////////##########//////////////////////
}

void drainWater() {
    setWaterTank(true);       // Open water tank
    delay(waterOpenTime);     // Waiting...
    setWaterTank(false);      // Close water tank
}

void setup() {
    myservo.attach(servoDataPin);  // attaches the servo on pin 9 to the servo object
    pinMode(armaturePin, OUTPUT);
    pinMode(waterPin, OUTPUT);
    pinMode(coffeePowerPin, OUTPUT);
    pinMode(startPin, INPUT);
    setCoffeeTank(false);
    setArmature(false);
    setWaterTank(false);
    setCoffeePower(false);
}

void loop() {
    int shouldStart = digitalRead(startPin);   // read the input pin
    
    if (shouldStart) {
      drainCoffee();
      drainWater();
      setCoffeePower(true);
      delay(coffeeTurnOnTime);
      setCoffeePower(false);
    }

    delay(1000);
}

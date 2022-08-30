#include <Servo.h>


Servo myservo;  // create servo object to control a servo

// TODO: change pins for nodeMCU: consider that "servoDataPin" is PWM pulse
// optional TODO: we don't need startPin: we read from the application

int closeDegree = 170;          // variable to store the servo position for closing
int openDegree = 90;            // variable to store the servo position for opening
int armaturePin = 13;           // variable to set the pin of HIGH and LOW voltage for turn on and off armature
int waterPin = 7;               // variable to set the pin of HIGH and LOW voltage for open and close water tank
int servoDataPin = 9;           // variable to set the pin of data/degree for servo
int coffeePowerPin = 8;         // variable to set the pin of HIGH and LOW voltage for turn on and off coffee power
int startPin = 10;              // variable to read the pin for starting coffee process

int coffeeLevel = 1;            // 1 to 3
int waterLevel = 1;             // 1 to 2
int coffeeLevelUpUnit = 2000;   // for each level up
int waterLevelUpUnit = 10000;   // for each level up

int coffeeInitTime = 5000;      // first level time for coffee
int waterInitTime = 18000;      // first level time for water
int coffeeOpenTime;             // time (ms) that coffee tank is open
int waterOpenTime;              // time (ms) that water tank is open
int coffeeTurnOnTime = 40000;   // time (ms) that coffee maker must be on and boiling


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
      digitalWrite(waterPin, LOW);
    } else {
      digitalWrite(waterPin, HIGH);
    }
}

void setArmature(bool isOn) {
    // 1 for on and 0 for off
    if (isOn) {
      digitalWrite(armaturePin, LOW);
    } else {
      digitalWrite(armaturePin, HIGH);
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
    Serial.begin(9600);
    myservo.attach(servoDataPin);  // attaches the servo on pin 9 to the servo object
    pinMode(armaturePin, OUTPUT);
    pinMode(waterPin, OUTPUT);
    pinMode(coffeePowerPin, OUTPUT);
    pinMode(startPin, INPUT_PULLUP); // INPUT_PULLUP is HIGH in default so it is active LOW
    setCoffeeTank(false);
    setArmature(false);
    setWaterTank(false);
    setCoffeePower(false);
}

void loop() {
    int shouldStart = digitalRead(startPin);   // read the input pin
    
    // TODO: read shouldStart from application
    // TODO: read coffeeLevel and waterLevel from application
    
    coffeeOpenTime = coffeeInitTime + (coffeeLevel - 1) * coffeeLevelUpUnit;
    waterOpenTime = waterInitTime + (waterLevel - 1) * waterLevelUpUnit;

    Serial.println("shouldStart:");
    Serial.println(shouldStart);
    Serial.println("coffeeOpenTime:");
    Serial.println(coffeeOpenTime);
    Serial.println("waterOpenTime:");
    Serial.println(waterOpenTime);

    // TODO: if you read from application shouldStart must be active HIGH
    if (shouldStart == LOW) {
      drainCoffee();
      drainWater();
      setCoffeePower(true);
      delay(coffeeTurnOnTime);
      setCoffeePower(false);
      // TODO: send to server that I'm turned off
    }

    delay(1000);
}

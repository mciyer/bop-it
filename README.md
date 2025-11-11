# bop-it
Code for Homemade Bop-It



#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}


//do interrupt instead of pulling would be faster and more effecient

/*Calibers for parts

Speaker : 0.40 inch Circle
Button: 0.50 inch Circle
Infared: 0.22 by 0.4 in Rectangle
Pull it Button: 0.15 in Circle




*/


void bopIt() {
  static const byte buttons[] = {34, 32, 35};  
  static bool initialized = false;
  static int prev = 0x00;
  static unsigned long timeout = 0;
  int data = 0;

  if (!initialized) {
    for (byte i = 0; i < 3; i++)
      pinMode(buttons[i], INPUT_PULLUP);
    initialized = true;
  }

  for (byte i = 0; i < 3; i++)
    if (digitalRead(buttons[i]) == LOW) data |= (1 << i);

  if (data != prev || millis() > timeout) {
    if (data != 0) {
      Serial.println("Button pressed");
    }
    prev = data;
    timeout = millis() + 1000;
  }

  delay(10);
}


void sliceIt() {
  static const int irPin = 27;   
  static bool initialized = false; 


  if (!initialized) {
    pinMode(irPin, INPUT);
    initialized = true;
  }

  int irValue = digitalRead(irPin);

  if (irValue == LOW) {
    Serial.println("Object detected!");
  } else {
    Serial.println("No object detected.");
  }

  delay(200); 
}



void loop() {
  bopIt();
  sliceIt();
}

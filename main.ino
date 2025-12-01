#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

#define VRX_PIN 15
#define VRY_PIN 13


void setup() { Serial.begin(115200); }

//do interrupt instead of pulling would be faster and more effecient

/*Calibers for parts

Speaker : 0.40 inch Circle Button: 0.50 inch Circle Infared: 0.22 by 0.4 in Rectangle Pull it Button: 0.15 in Circle

*/



void bopIt() { static const byte buttons[] = {34, 0, 35};
static bool initialized = false; static int prev = 0x00; static unsigned long timeout = 0; int data = 0; 

if (!initialized) { for (byte i = 0; i < 3; i++) pinMode(buttons[i], INPUT_PULLUP); initialized = true; }

for (byte i = 0; i < 3; i++) if (digitalRead(buttons[i]) == LOW) data |= (1 << i);

if (data != prev || millis() > timeout) { if (data != 0) { Serial.println("Button pressed");  } prev = data; timeout = millis() + 1000; }

delay(10);  }

// const int IR_D0 = 32;

// void irTest() {
//   static bool initialized = false;

//   if (!initialized) {
//     Serial.println("IR Sensor Test Started...");
//     pinMode(IR_D0, INPUT);
//     analogReadResolution(12);
//     analogSetAttenuation(ADC_11db);
//     initialized = true;
//   }

//   int digitalValue = digitalRead(IR_D0);

//   if (digitalValue == HIGH) {
//     Serial.println("NO OBJECT");
//   } else {
//     Serial.println("OBJECT DETECTED");
//   }

//   delay(100);
// }

void gyroTest() {
    MPU6050 mpu;
  static bool initialized = false;

  if (!initialized) {
    Serial.println("Initializing MPU6050...");
    Wire.begin(21, 22);
    mpu.initialize();

    if (!mpu.testConnection()) {
      Serial.println("MPU6050 connection failed!");
    } else {
      Serial.println("MPU6050 connected!");
    }

    initialized = true;
  }

  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  if (az > 8000) {
    Serial.println("Orientation: UPRIGHT");
  } else if (az < -8000) {
    Serial.println("Orientation: UPSIDE DOWN");
  } else {
    Serial.println("Orientation: SIDEWAYS / MOVING");
  }

  delay(200);
}


// void micTest() { static const int MIC_PIN = 34;
//   static bool initialized = false;

//   if (!initialized) {
//     Serial.println("Mic Test Started...");
//     analogReadResolution(12);
//     analogSetAttenuation(ADC_11db);
//     initialized = true;
//   }

//   int micValue = analogRead(MIC_PIN);

//   if (micValue > 2200) {
//     Serial.println("LOUD");
//   } else {
//     Serial.println("SOFT");
//   }

//   delay(50);
// }


void FlickIt() {
  static bool initialized = false;
  static int xNeutral = 0;
  static int yNeutral = 0;

  const int DEADZONE = 400; // tighten deadzone for better accuracy

  if (!initialized) {
    Serial.println("Joystick Ready!");
    xNeutral = analogRead(VRX_PIN); // measure neutral position
    yNeutral = analogRead(VRY_PIN);
    initialized = true;
  }

  int xValue = analogRead(VRX_PIN);
  int yValue = analogRead(VRY_PIN);

  String direction = "CENTER";


  if (xValue < xNeutral - DEADZONE || xValue > xNeutral + DEADZONE|| yValue < yNeutral - DEADZONE || yValue > yNeutral + DEADZONE) direction = "Flicked";

  if (direction != "CENTER") {
    Serial.println(direction);
  }
  delay(200);
}








void loop() {
  bopIt();
  gyroTest();
  // micTest();
  // irTest();
  //FlickIt();
}





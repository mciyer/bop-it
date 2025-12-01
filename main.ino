#include <Arduino.h>


bool bopIt();
bool sliceIt();

typedef bool (*ActionFunc)();
ActionFunc actions[] = { bopIt, sliceIt };          //The array of function pointers that the loop will run through
const int actionCount = sizeof(actions) / sizeof(actions[0]);

void setup() { Serial.begin(115200); }

//do interrupt instead of pulling would be faster and more effecient

/*Calibers for parts

Speaker : 0.40 inch Circle Button: 0.50 inch Circle Infared: 0.22 by 0.4 in Rectangle Pull it Button: 0.15 in Circle

*/

bool bopIt() { static const byte buttons[] = {34, 32, 35};
static bool initialized = false; static int prev = 0x00; static unsigned long timeout = 0; int data = 0;
static bool completed = false;

bool result = false;  

if (!initialized) { for (byte i = 0; i < 3; i++) pinMode(buttons[i], INPUT_PULLUP); initialized = true; }

for (byte i = 0; i < 3; i++) if (digitalRead(buttons[i]) == LOW) data |= (1 << i);

if (data != prev || millis() > timeout) { if (data != 0) { Serial.println("Button pressed"); completed = true; } prev = data; timeout = millis() + 1000; }

delay(10); 


if (completed) { result = true; completed = false; }

return result; }



bool sliceIt() { static const int irPin = 27;
static bool initialized = false;
static bool completed = false;

bool result = false;  

if (!initialized) { pinMode(irPin, INPUT); initialized = true; }

int irValue = digitalRead(irPin);

if (irValue == LOW) { Serial.println("Object detected!"); completed = true; } else { Serial.println("No object detected."); }

delay(200); 


if (completed) { result = true; completed = false; }

return result; }






void loop() { 
  static bool initialized = false;
  static ActionFunc current = nullptr;

  if (!initialized) {
    randomSeed(analogRead(0));
    current = actions[random(actionCount)];  // pick first action
    initialized = true;
  }

  // always run the current action
  bool done = current();

  // if the action finished, pick a new random one
  if (done) {
    Serial.println("Action completed!");
    current = actions[random(actionCount)];
  }
}


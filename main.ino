#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

// Pins

#define IR_PIN 32
#define MIC_PIN 13
#define JOYSTICK_X_PIN 27
#define JOYSTICK_Y_PIN 26
#define SDA_PIN 21
#define SCL_PIN 22
#define BOPIT_PIN 14
#define PULLIT_PIN 12

// Gyroscope States

#define GYRO_CENTER 0
#define GYRO_RIGHT 1
#define GYRO_LEFT -1

// Action Enum

#define BOP_IT 4
#define PULL_IT 5
#define FLICK_IT 0
#define SLICE_IT 1
#define TILT_IT 2
#define SHOUT_IT 3

// For Gyroscope
MPU6050 mpu;

void setup() { 
  Serial.begin(9600);              // Serial Output setup
  analogReadResolution(12);          // 0–4095 range
  analogSetAttenuation(ADC_11db);    // Good for microphones
  randomSeed(analogRead(0));         // Random number generator

  // Gyroscope Setup
  Wire.begin(SDA_PIN, SCL_PIN);  // SDA, SCL on ESP32
  mpu.initialize();

  if (!mpu.testConnection()) {  
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  // Other Component Setup
  pinMode(IR_PIN, INPUT); // IR Sensor
}

unsigned long timer_millis = 3000; // Default is 3 seconds
unsigned long action_delay = 1000; // Delay before setting action window timer
unsigned long grace_period = 2000; // Delay between actions performed

int current_round = 0;
bool in_normal_mode = true;

void loop() { 
  // // Select the mode by flicking the joystick
  // int flicked = flickIt();
  // int bopped = bopIt();

  // // Switches mode
  // if (flicked) { 
  //   in_normal_mode = !in_normal_mode;
    
  //   // Play audio for current mode
  //   if (in_normal_mode) {
  //     playAudioFile("BOP-IT.mp3");
  //   }
  //   else {
  //     playAudioFile("SLICE-IT.mp3");
  //   }
  // }

  // // Push the BOP-IT button to start the game
  // if (bopped) {
  //   countdown();

  //   if (in_normal_mode) {
  //     playNormalMode();
  //   }
  //   else {
  //     playSimonMode();
  //   }
  // }

  countdown();
  playNormalMode();
  restart();
  delay(3000);

}

//do interrupt instead of pulling would be faster and more effecient

/*Calibers for parts

Speaker : 0.40 inch Circle Button: 0.50 inch Circle Infared: 0.22 by 0.4 in Rectangle Pull it Button: 0.15 in Circle

*/

bool bop_idle = true;

int bopIt() {
  int pressed = (digitalRead(BOPIT_PIN) == LOW);

  if (pressed && bop_idle) {
    bop_idle = false;
    return true;
  }
  if (!pressed) {
    bop_idle = true;   // reset when released
  }
  return false;
}


/**
 * IR SENSOR
 * @returns TRUE if the sensor detects an object
 */
bool ir_idle = true;

int sliceIt() {
  int active = (digitalRead(IR_PIN) == LOW);  // LOW = object detected

  if (active && ir_idle) {
    ir_idle = false;
    return true;
  }
  if (!active) {
    ir_idle = true;
  }
  return false;
}

bool pull_idle = true;

int pullIt() {
  int pulled = (digitalRead(PULLIT_PIN) == LOW);

  if (pulled && pull_idle) {
    pull_idle = false;
    return true;
  }
  if (!pulled) {
    pull_idle = true;  // reset only when released
  }
  return false;
}

/**
 * MICROPHONE
 * 
 * @returns TRUE if input value is above the threshold
 */
bool shout_idle = true;
unsigned long shoutCooldown = 0;

int shoutIt() {
  const int mic_threshold = 2200;
  int mic_value = analogRead(MIC_PIN);

  bool loud = mic_value > mic_threshold;

  // Prevent retriggering for 150ms after each shout
  if (millis() < shoutCooldown) {
    if (!loud) shout_idle = true;
    return false;
  }

  if (loud && shout_idle) {
    shout_idle = false;
    shoutCooldown = millis() + 150; // debounce / cooldown
    return true;
  }

  if (!loud) {
    shout_idle = true;
  }

  return false;
}


/**
 * JOYSTICK
 * 
 * @returns TRUE if any direction is inputted
 */
bool joystick_idle = true;  

bool flickIt() {
  int xValue = analogRead(JOYSTICK_X_PIN); // 0–4095
  int yValue = analogRead(JOYSTICK_Y_PIN);

  const int center = 2048;
  const int deadzone = 250;

  bool outside =
    (abs(xValue - center) > deadzone) ||
    (abs(yValue - center) > deadzone);

  if (outside && joystick_idle) {
    joystick_idle = false;   // we just detected the flick
    return true;            // one-time trigger
  }

  if (!outside) {
    joystick_idle = true;    // stick returned to center → ready for next flick
  }

  return false; // not a new flick
}

/**
 * GYROSCOPE
 * 
 * @return 0 if center, 1 if tilt right, -1 if tilt left
 */
int tiltIt() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Because the board is mounted upside-down, invert Z
  az = -az;

  // Convert to g units (default 2g range)
  float ax_g = ax / 16384.0;
  float az_g = az / 16384.0;

  // Compute tilt angle along the board's long axis
  float angle = atan2(ax_g, az_g) * 180.0 / PI;

  const float DEADZONE = 37.5;

  if (angle > DEADZONE) {
    return GYRO_RIGHT;
  } 
  else if (angle < -DEADZONE) {
    return GYRO_LEFT;
  }

  return GYRO_CENTER;
}

void playAudioFile(String file) {
  Serial.print("PLAY:");
  Serial.println(file + ".mp3"); 
}

void displayAction(int a) {
  String actions[6] = {"FLICK-IT", "SLICE-IT", "TILT-IT", "SHOUT-IT", "BOP-IT", "PULL-IT"};
  String action = actions[a];

  String filename = action;
  playAudioFile(filename);
}

/**
 * Randomly the next action for the player to perform.
 * If in normal mode, the first 6 actions are always the same
 */
int selectAction() {
  int action;
    if (in_normal_mode && current_round <= 3) {
      action = current_round - 1; // Actions are 0 indexed
    }
    else {
      action = random(0, 3);
  }
  return action;
}

int checkInput(int input, int expected_input) {
  if (input == expected_input) {
    return 0;
  }
  else {
    return input + 1;
  }
}

/**
 * Reads inputs from components.
 * 
 * @param action the action the player must perform to score
 * @returns 1 if correct is read, 0 if incorrect, -1 if nothing was performed
 */
int readInputs(int correct_input) {
  // int bop_value = bopIt();
  int flick_value = flickIt();
  // int pull_value = pullIt();
  int shout_value = shoutIt();
  int slice_value = sliceIt();
  int tilt_value = tiltIt();

  // if (bop_value) {
  //   return checkInput(BOP_IT, correct_input);
  // }

  if (flick_value) {
    return checkInput(FLICK_IT, correct_input);
  }

  // if (pull_value) {
  //   return checkInput(PULL_IT, correct_input);
  // }

  if (slice_value) {
    return checkInput(SLICE_IT, correct_input);
  }

  if (tilt_value) {
    return checkInput(TILT_IT, correct_input);
  }

  if (shout_value) {
    return checkInput(SHOUT_IT, correct_input);
  }

  // No input recorded
  return -1;
}

// Countdown before starting a game
void countdown() {
  Serial.println("3...");
  delay(1000);
  Serial.println("2...");
  delay(1000);
  Serial.println("1...");
  delay(1000);
  Serial.println("GO!!!");
  delay(1000);
}

void playNormalMode() {
  bool win = true;
  while (++current_round <= 100) {
    // After 10 rounds, reduce timer window by a quarter second (lowest time is half a second)
    Serial.print("ROUND: ");
    Serial.println(current_round);

    if (current_round % 10 == 0) {
      Serial.println("Faster!");
      timer_millis = max(timer_millis - 300UL, 300UL);
      grace_period = max(grace_period - 200UL, 200UL);
    }

    // Step 1: Randomly choose an action (the first 6 have a set pattern)
    int action = selectAction();

    // Step 2: Communicate the command to the player 
    displayAction(action);
    
    delay(action_delay);

    // Step 3: Start timer for how long the player has to act (the time limit decreases as rounds go on)
    unsigned long start_time = millis();
    
    // Step 4: Read all inputs until the timer runs out
    bool success = false;
    while ((millis() - start_time) < timer_millis) {
      int status = readInputs(action);

      if (status == 0) { // Correct action inputted
        success = true;
        break;
      }
      
      if (status > 0) { // Incorrect action inputted
        success = false;
        break;
      }
    }

    // Step 5: Check if the correct input was performed, increase the score if so, end the game otherwise
    if (!success) {
      playAudioFile("WRONG");
      win = false;
      break;
    }
    else {
      playAudioFile("GOOD!");
    }

    // Step 6: Add in a recovery period before next action call
    delay(grace_period);

  }

  // Step 7: Display score to player
  if (win) {
    playAudioFile("YOU-WIN");
    delay(2500);
  }
  else {
    playAudioFile("YOU-LOSE");
    delay(2500);
  }

  playAudioFile("YOUR-SCORE");
 
}

void playSimonMode() {
  // Step 1: Randomly choose an action

  // Step 2: Repeat all the subsequent action to the player

  // Step 3: Read all inputs, wait until one is read as true

  // Step 4: Check if the input is next in the sequence, if so continue until all correct actions are played.
  // Any failure results in a gameover

}

void restart() {
  timer_millis = 3000; 
  action_delay = 1000; 
  grace_period = 2000; 
  current_round = 0;
}



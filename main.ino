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

#define BOP_IT 0
#define FLICK_IT 1
#define PULL_IT 2
#define SHOUT_IT 3
#define SLICE_IT 4
#define TILT_IT 5

// For Gyroscope
MPU6050 mpu;

void setup() { 
  Serial.begin(115200);              // Serial Output setup
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

int round = 0;
bool in_normal_mode = true;


void loop() { 
  // Select the mode by flicking the joystick
  int flicked = flickIt();
  int bopped = bopIt();

  // Switches mode
  if (flicked) { 
    in_normal_mode = !in_normal_mode;
    
    // Play audio for current mode
    if (in_normal_mode) {
      playAudioFile("normal_mode.mp3");
    }
    else {
      playAudioFile("simon_mode.mp3");
    }
  }

  // Push the BOP-IT button to start the game
  if (bopped) {
    if (in_normal_mode) {
      playNormalMode();
    }
    else {
      playSimonMode();
    }
  }


}

//do interrupt instead of pulling would be faster and more effecient

/*Calibers for parts

Speaker : 0.40 inch Circle Button: 0.50 inch Circle Infared: 0.22 by 0.4 in Rectangle Pull it Button: 0.15 in Circle

*/

int bopIt() { 
  return false;
}


/**
 * IR SENSOR
 * @returns TRUE if the sensor detects an object
 */
int sliceIt() { 
  int ir_value = digitalRead(IR_PIN); // High/Low Detection
  return (ir_value == LOW); 
}

int pullIt() {
  return false;
}

/**
 * MICROPHONE
 * 
 * @returns TRUE if input value is above the threshold
 */
int shoutIt() {
  const int mic_threshold = 2200;
  int mic_value = analogRead(MIC_PIN);
  return (mic_value > mic_threshold);
}

/**
 * JOYSTICK
 * 
 * @returns TRUE if any direction is inputted
 */
int flickIt() {
  int xValue = analogRead(JOYSTICK_X_PIN); // 0–4095
  int yValue = analogRead(JOYSTICK_Y_PIN);
  
  int center = 2048; // Center of joystick reads this value
  const int joystick_deadzone = 250;

  return (abs(xValue - center) > joystick_deadzone) || (abs(yValue - center) > joystick_deadzone);
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
  Serial.println(file); 
}

void displayAction(int a) {
  String actions[6] = {"BOP-IT", "FLICK-IT", "PULL-IT", "SHOUT-IT", "SLICE-IT", "TILT-IT"};
  String action = actions[a];

  String filename = action + ".mp3";
  playAudioFile(filename);
}

/**
 * Randomly the next action for the player to perform.
 * If in normal mode, the first 6 actions are always the same
 */
int selectAction() {
  int action;
    if (in_normal_mode && round <= 6) {
      action = round;
    }
    else {
      action = random(0, 6);
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

void playNormalMode() {
  bool win = true;
  while (++round <= 100) {
    // After 10 rounds, reduce timer window by a quarter second (lowest time is half a second)
    if (round % 10 == 0) {
      timer_millis = max(timer_millis - 250UL, 500UL);
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
      Serial.println("WRONG!");
      win = false;
      break;
    }
    else {
      Serial.println("GOOD!");
    }

    // Step 6: Add in a recovery period before next action call
    delay(grace_period);

  }

  // Step 7: Display score to player
  if (win) {
    playAudioFile("you_win.mp3");
  }
  else {
    playAudioFile("you_lose.mp3");
  }

  playAudioFile("your_score.mp3");
 
}

void playSimonMode() {
  // Step 1: Randomly choose an action

  // Step 2: Repeat all the subsequent action to the player

  // Step 3: Read all inputs, wait until one is read as true

  // Step 4: Check if the input is next in the sequence, if so continue until all correct actions are played.
  // Any failure results in a gameover

}



#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include <audio_files.h>

// WIFI ADDED
#include <WiFi.h>
#include <WebServer.h>

// Pins

#define IR_PIN 14
#define MIC_PIN 33
#define JOYSTICK_X_PIN 32
#define JOYSTICK_Y_PIN 35
#define SDA_PIN 21
#define SCL_PIN 22
#define BOPIT_PIN 34
#define PULLIT_PIN 15

// Gyroscope States

#define GYRO_CENTER 0
#define GYRO_RIGHT 1
#define GYRO_LEFT -1

// Action Enum

#define BOP_IT 0
#define FLICK_IT 1
#define SLICE_IT 2
#define TILT_IT 3
#define SHOUT_IT 4
#define PULL_IT 5

#define NUM_PARTS 6
#define MAX_ROUNDS 100

String VOICE_COMMAND[NUM_PARTS] = {
  MP3_BOP_IT,
  MP3_FLICK_IT,
  MP3_SLICE_IT,
  MP3_TILT_IT,
  MP3_SHOUT_IT,
  MP3_PULL_IT
};

String SOUND_EFFECT_ACTIONS[NUM_PARTS] = {
  MP3_EFFECT_BOP, 
  MP3_EFFECT_FLICK, 
  MP3_EFFECT_SLICE, 
  MP3_EFFECT_TILT, 
  MP3_EFFECT_SHOUT,
  MP3_EFFECT_PULL
};

String LOSE_PHRASES[10] = {
  MP3_LOSE_1,
  MP3_LOSE_2,     
  MP3_LOSE_3,      
  MP3_LOSE_4,     
  MP3_LOSE_5,      
  MP3_LOSE_6,      
  MP3_LOSE_7,     
  MP3_LOSE_8,      
  MP3_LOSE_9,      
  MP3_LOSE_10    
};

// For Gyroscope
MPU6050 mpu6050;


// WIFI ADDED
String current_action = "Idle";
const char* ssid = "Bop_It";
const char* pass = "cics_256";
WebServer server(80);



void setup() { 
  Serial.begin(115200);              // Serial Output setup
  delay(2000);     // WAIT for Processing to open USB bridge
  Serial.println("ESP32 READY");

  analogReadResolution(12);          // 0–4095 range
  analogSetAttenuation(ADC_11db);    // Good for microphones
  randomSeed(esp_random());         // Random number generator

  // Gyroscope Setup
  Wire.begin(SDA_PIN, SCL_PIN);  // SDA, SCL on ESP32
  mpu6050.initialize();

  if (!mpu6050.testConnection()) {  
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  // Other Component Setup
  pinMode(IR_PIN, INPUT); // IR Sensor
  pinMode(BOPIT_PIN, INPUT_PULLUP);
  pinMode(PULLIT_PIN, INPUT_PULLUP);

   ///// WIFI ADDED /////
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);
  server.on("/", on_home);
  server.begin();
  Serial.println("WiFi AP started: http://192.168.4.1");
  ///// END WIFI /////
}

unsigned long timer_millis; // Default is 2 seconds
unsigned long grace_period; // Delay between actions performed
unsigned long action_delay = 1000; // 1 sec extra 

int current_round = 0;
int current_score = 0;
bool in_normal_mode = true;
String soundtrack_file_path = MP3_SOUNDTRACK_1; // Soundtrack for normal mode

// Scores
int highscore_normal = 0;
int highscore_simon = 0;


void loop() {
  server.handleClient();  // WIFI ADDED
  int flicked = flickIt();  // Select the mode by flicking the joystick
  int bopped = bopIt(); // Start game in selected mode

  if (flicked) {  // Switches mode
    in_normal_mode = !in_normal_mode;
    
    // Play audio for current mode
    if (in_normal_mode) {
      playAudioFile(MP3_NORMAL_MODE);
      soundtrack_file_path = MP3_SOUNDTRACK_1;
    }
    else {
      playAudioFile(MP3_SIMON_MODE);
      soundtrack_file_path = MP3_SOUNDTRACK_2;
    }
  }

  // Push the BOP-IT button to start the game
  if (bopped) {
    gameSetup();
    sayHighscore();

    if (in_normal_mode) {
      countdown();
      startSoundtrack();
      playNormalMode();
    }
    else {
      readySetSimon();
      startSoundtrack();
      playSimonMode();
    }

    playAudioFile(MP3_PLAY_AGAIN);
  }

}

void on_home() {
  if (in_normal_mode) {
    server.send(200, "text/html",
      "<meta http-equiv='refresh' content='1'>"
      "<h1 style='font-size:60px; text-align:center;'>"
      "Mode: Normal<br>"
      "Highscore: " + String(highscore_normal) + "<br>"
      "</h1>"
    );
  } else {
    server.send(200, "text/html",
      "<meta http-equiv='refresh' content='1'>"
      "<h1 style='font-size:60px; text-align:center;'>"
      "Mode: Simon<br>"
      "Highscore: " + String(highscore_simon) + "<br>"
      "</h1>"
    );
  }
}


void playAudioFile(String file) {
  Serial.println("PLAY:" + file);
}

void announceScore(String score) {
  playAudioFile(MP3_SCORE);
  delay(1000);
  Serial.println("SCORE:" + score);
  delay(2000);
}

void announceHighscore(String score) {
  playAudioFile(MP3_HIGHSCORE);
  delay(1200);
  Serial.println("SCORE:" + score);
  delay(2000);
}

void startSoundtrack() {
  String track = soundtrack_file_path;
  Serial.println("START:" + track);
}

void stopSoundtrack() {
  String track = soundtrack_file_path;
  Serial.println("STOP:" + track);
}

void increaseSpeed() {
  String track = soundtrack_file_path;
  Serial.println("INC:" + track);
}

void displayAction(int a) {
  playAudioFile(VOICE_COMMAND[a]);
}

void playLosePhrase() {
  playAudioFile(LOSE_PHRASES[random(0, 10)]);
  delay(3000); // Ensures audio clip finishes
}


/**
 * Randomly the next action for the player to perform.
 * If in normal mode, the first 6 actions are always the same
 */
int selectAction() {
  int action;
    if (in_normal_mode && current_round <= NUM_PARTS) {
      action = current_round - 1; // Actions are 0 indexed
    }
    else {
      action = random(0, NUM_PARTS);
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
  int bop_value = bopIt();
  int flick_value = flickIt();
  int pull_value = pullIt();
  int shout_value = shoutIt();
  int slice_value = sliceIt();
  int tilt_value = tiltIt();

  if (flick_value) {
    return checkInput(FLICK_IT, correct_input);
  }

  if (slice_value) {
    return checkInput(SLICE_IT, correct_input);
  }

  if (bop_value) {
    return checkInput(BOP_IT, correct_input);
  }

  if (pull_value) {
    return checkInput(PULL_IT, correct_input);
  }

  if (tilt_value) {
    return checkInput(TILT_IT, correct_input);
  }

  if (shout_value && correct_input == SHOUT_IT) {
    return checkInput(SHOUT_IT, correct_input);
  }

  // No input recorded
  return -1;
}

// Countdown before starting a game
void countdown() {
  playAudioFile(MP3_COUNTDOWN_3);
  delay(1000);
  playAudioFile(MP3_COUNTDOWN_2);
  delay(1000);
  playAudioFile(MP3_COUNTDOWN_1);
  delay(1000);
  playAudioFile(MP3_GO);
  delay(1000);
}

void readySetSimon() {
  playAudioFile(MP3_READY);
  delay(1000);
  playAudioFile(MP3_FOLLOW);
  delay(1000);
}

void sayHighscore() {
  if (in_normal_mode && highscore_normal > 0) {
    announceHighscore(String(highscore_normal));
    delay(2000);
  }
  else if (!in_normal_mode && highscore_simon > 0) {
    announceHighscore(String(highscore_simon));
    delay(2000);
  }
}

void updateHighscore() {
  // Updates highscore if needed
  if (in_normal_mode) {
    highscore_normal = max(highscore_normal, current_score);
  }
  else {
    highscore_simon = max(highscore_simon, current_score);
  }
}

void gameover(bool win) {
  stopSoundtrack();

  if (win) {
 

    announceScore(String(current_score));
    delay(1000);
    playAudioFile(MP3_WIN);
    delay(2500);
  }
  else {
    playLosePhrase();
    if (current_score > 0) {
      announceScore(String(current_score));
    }
    delay(1000);
  }

  updateHighscore();
  
}

void gameSetup() {
  timer_millis = 2500; 
  grace_period = 2000; 
  current_round = 0;
  current_score = 0;
}

void playNormalMode() {
  bool win = true;
  while (++current_round <= MAX_ROUNDS) {
    // After 10 rounds, reduce timer window by a quarter second (lowest time is half a second)
    if (current_round % 10 == 0) {
      timer_millis = max(timer_millis - 300UL, 250UL);
      grace_period = max(grace_period - 200UL, 100UL);

      increaseSpeed();
    }

    // Step 1: Randomly choose an action (the first 6 have a set pattern)
    int action = selectAction();

    // Step 2: Communicate the command to the player 
    displayAction(action);

    // Step 3: Start timer for how long the player has to act (the time limit decreases as rounds go on)
    unsigned long start_time = millis();
    
    // Step 4: Read all inputs until the timer runs out
    bool success = false;
    while ((millis() - start_time) < (timer_millis + action_delay)) {
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
      win = false;
      break;
    }
    else {
      current_score++;
      playAudioFile(SOUND_EFFECT_ACTIONS[action]);
    }

    // Step 6: Add in a recovery period before next action call
    delay(grace_period);

  }

  // Step 7: Display score to player
  gameover(win);
 
}

void playSimonMode() { 
  int sequence[MAX_ROUNDS];
  int seq_len = 0;

  bool playing = true;

  while (playing && seq_len < MAX_ROUNDS) {
    current_round++;

    // Step 1: Add new random action to the sequence
    sequence[seq_len] = selectAction();
    seq_len++;

    // Step 2: Play back the full sequence to the player
    for (int i = 0; i < seq_len; i++) {
      displayAction(sequence[i]);
      delay(1000);  // Delay between sequence steps (editable)
    }

    playAudioFile(MP3_REPEAT);

    // Step 3: Wait for player to repeat the sequence
    for (int i = 0; i < seq_len; i++) {
      int expected = sequence[i];

      unsigned long start = millis();
      bool correct = false;

      // Wait for player to input the correct action (no time limit)
      while (true) {
        int input_result = readInputs(expected);

        if (input_result == 0) {
          // Correct action
          correct = true;
          break;
        }
        else if (input_result > 0) {
          // Wrong action
          correct = false;
          break;
        }

      }

      // Check result
      if (!correct) {
        // FAILURE → Game Over
        playing = false;
        break;
      }

      // Play audio effect 
      playAudioFile(SOUND_EFFECT_ACTIONS[expected]);
      
      // Little gap before next expected input
      delay(500);
    }

    // Pause before next round (sequence grows)
    if (playing) {
      // Performed the correct 
      current_score++;
      delay(1000);
      Serial.println();
      playAudioFile(MP3_GOOD_JOB);
      Serial.println();
      delay(1500);
    }
  }
  
  // Step 4: End of game
  gameover(playing);

}


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
  int pulled = (digitalRead(PULLIT_PIN) == HIGH);

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
  static int last_state = GYRO_CENTER;
  static bool waiting_for_center = false;

  // ---- NEW smoothing variables ----
  static float smoothedAngle = 0;
  const float smoothFactor = 0.85;

  // ---- NEW debounce variables ----
  static unsigned long tiltStart = 0;
  const unsigned long tiltHoldTime = 120;

  const float DEADZONE = 50.0;

  // Read gyro/accel data
  int16_t ax, ay, az, gx, gy, gz;
  mpu6050.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw values to g's
  float ay_g = ay / 16384.0;
  float az_g = az / 16384.0;

  // ---- FLIP Z AXIS (gyroscope is upside down) ----
  az_g = -az_g;

  // Raw tilt angle
  float rawAngle = atan2(ay_g, az_g) * 180.0 / PI;

  // Smooth it
  smoothedAngle = (smoothFactor * smoothedAngle) + ((1.0 - smoothFactor) * rawAngle);
  float angle = smoothedAngle;

  // Determine tilt state
  int state = GYRO_CENTER;
  if (angle > DEADZONE)       state = GYRO_RIGHT;
  else if (angle < -DEADZONE) state = GYRO_LEFT;

  // Require return to center after tilt
  if (waiting_for_center) {
    if (state == GYRO_CENTER) {
      waiting_for_center = false;
      last_state = GYRO_CENTER;
    }
    return GYRO_CENTER;
  }

  // Start potential tilt
  if (last_state == GYRO_CENTER && state != GYRO_CENTER) {
    tiltStart = millis();
    last_state = state;
    return GYRO_CENTER;
  }

  // Confirm tilt
  if (state != GYRO_CENTER && (millis() - tiltStart) > tiltHoldTime) {
    waiting_for_center = true;
    last_state = state;
    return state;
  }

  return GYRO_CENTER;
}


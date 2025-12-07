import processing.serial.*;
import processing.sound.*;

Serial myPort;

// SFX (keeps your original approach)
SoundFile sfx;

// Soundtrack using processing.sound so we can call rate()
SoundFile soundtrack;
float soundtrackRate = 1.0; // 1.0 = normal speed
String sound_directory = "audio/";

void setup() {
  size(200, 400);
  println(Serial.list());
  String serialName = "/dev/cu.usbserial-0001";
  myPort = new Serial(this, serialName, 115200);

  // Don't preload soundtrack here — we'll load when START command arrives
}

void playAudio(String file) {
     if (sfx != null) sfx.stop();
     sfx = new SoundFile(this, sound_directory + file);
     sfx.play();
}

// Waits until previous audio is done playing
void playAudioWait(String file) {
    
  // If a sound is already playing, wait for it to finish
  if (sfx != null) {
    while (sfx.position() < sfx.duration() - 0.01) {
      delay(5);   // prevents freezing CPU, but keeps waiting
    }
    sfx.stop();
  }

  // Now play the new sound
  sfx = new SoundFile(this, sound_directory + file);
  sfx.play();
}


void playEffect(String line) {
    String file = line.substring(5).trim();          // yields e.g. ":BOP-IT.mp3"
    playAudio(file);
}


void startSoundtrack(String line) {
    String file = line.substring("START:".length()).trim(); // "SOUNDTRACK-1.mp3"
    // unload previous
    if (soundtrack != null) {
      soundtrack.stop();
      soundtrack = null;
    }
    println("Loading soundtrack: " + file);
    soundtrack = new SoundFile(this, sound_directory + file);
    soundtrack.rate(soundtrackRate); // ensure current rate applies
    soundtrack.loop();
}

void tellScore(String line) { // yields e.g "16"
  // Convert to int for simplier conditional statements
  int score = Integer.parseInt(line.substring(6).trim());
  
  // Single audio-file (1-19 and ten places)
  if (score < 20 || score % 10 == 0) {
    playAudio(score + ".mp3");
  }
  else {
    int tens_digit = score / 10 * 10;
    int ones_digit = score % 10;
    
    playAudio(tens_digit + ".mp3");
    delay(800);
    playAudio(ones_digit + ".mp3");
    delay(800);
  }
  
  
}

void stopSoundtrack() {
    if (soundtrack != null) {
      soundtrack.pause();
      soundtrackRate = 1.0; // Resets rate
    }
}

void increaseRateSoundtrack() {
    soundtrackRate += 0.05;               // increase by 5%
    if (soundtrackRate > 3.0) soundtrackRate = 3.0; // clamp if desired
    if (soundtrack != null) {
      soundtrack.rate(soundtrackRate);
    }
    //println("Soundtrack rate = " + nf(soundtrackRate,1,2));
}




void draw() {
  while (myPort.available() > 0) {
    
    // Reads line from serial buffer
    String line = myPort.readStringUntil('\n');
    if (line == null) continue;
    line = trim(line);
    if (line.length() == 0) continue;

    println("Received: " + line);

    // PLAY effect (keeps your existing behavior)
    if (line.startsWith("PLAY:")) {
      playEffect(line);
    }

    // START:soundtrack
    else if (line.startsWith("START:")) {
      startSoundtrack(line);
    }

    // STOP:soundtrack
    else if (line.startsWith("STOP:")) {
      stopSoundtrack();
    }

    // INC:soundtrack -> increase speed
    else if (line.startsWith("INC:")) {
      increaseRateSoundtrack();
    }
    
    else if (line.startsWith("SCORE:")) {
      tellScore(line);
    }
      
    
  }
}

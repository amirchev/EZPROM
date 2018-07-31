#include <EZPROM.h>

char phrases = 0;
unsigned long nextPrint;
const unsigned long interval = 10 * 1000; //every 10 seconds
const int phrase_size_max = 64;

void setup() {
  //initialize Serial
  Serial.begin(9600);

  //resets the EEPROM before use, all saved objects erased
  ezprom.reset();

  //set the timer for prints
  nextPrint = millis() + interval;
}

void loop() {
  if (Serial.available() > 0) {
    char phrase[phrase_size_max];
    int len;
    len = Serial.readBytesUntil('\n', phrase, phrase_size_max);
    phrase[len] = '\0';

    bool saved = ezprom.save(phrases, *phrase, len);
    if (saved) {
      Serial.println("Save successful.");
      phrases++;
    } else {
      Serial.println("Unable to save!");
    }
  }

  if (millis() > nextPrint) {
    nextPrint += interval;

    for (char i = 0; i < phrases; i++) {
      char phrase[phrase_size_max];
      bool loaded = ezprom.load(i, *phrase);

      if (loaded) {
        Serial.print("Phrase ");
        Serial.print((int) i);
        Serial.print(": ");
        Serial.println(phrase);
      } else {
        Serial.print("Unable to load phrase ");
        Serial.print((int) i);
        Serial.println(".");
      }
    }
  }
}


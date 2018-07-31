#include <EZPROM.h>

char phrases = 0;
const int phrase_size_max = 128;

void setup() {
  //initialize Serial
  Serial.begin(9600);

  //resets the EEPROM before use, all saved objects erased
  ezprom.reset();
}

void loop() {
  if (Serial.available() > 0) {
    bool saved = false;
    while (Serial.available() > 0) {
      char str[phrase_size_max];
      int len;
      len = Serial.readBytesUntil('\n', str, phrase_size_max - 1);
      str[len] = '\0';

      saved = ezprom.save(phrases, *str, len + 1);
      if (saved) {
        phrases++;
      }
    }

    //print all saved strings
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

    if (saved) {
      Serial.println("\nLast save was successful!\n");
    } else {
      Serial.println("\nUNABLE TO SAVE - OUT OF MEMORY!\n");
    }
  }
}


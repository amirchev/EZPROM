#include <EZPROM.h>

char phrases = 0;
const int phrase_size_max = 128;

void setup() {
  //initialize Serial
  Serial.begin(9600);

  //resets the EEPROM before use, all saved objects erased
  ezprom.reset();
  ezprom.setOverwriteIfSizeDifferent(false);
}

void loop() {
  //check if phrase is available in Serial
  if (Serial.available() > 0) {
    bool saved = false;
    while (Serial.available() > 0) {
      //a string to hold the string
      char str[phrase_size_max];
      int len = Serial.readBytesUntil('\n', str, phrase_size_max - 1); //read the string
      str[len] = '\0'; //null-terminate it

      //save the string into ezprom with its own ID
      saved = ezprom.save(phrases, *str, len + 1);
      //increase string index if save was successful
      if (saved) {
        phrases++;
      }
    }

    //print all saved strings
    for (char i = 0; i < phrases; i++) {
      //load the string
      char phrase[phrase_size_max];
      bool loaded = ezprom.load(i, *phrase);

      //print the phrase
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

    //let user know if save was successful
    if (saved) {
      Serial.println("\nLast save was successful!\n");
    } else {
      Serial.println("\nUNABLE TO SAVE - OUT OF MEMORY!\n");
    }
  }
}


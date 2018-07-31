#include <EZPROM.h>

char strings = 0;
const int string_size_max = 128;

void setup() {
  //initialize Serial
  Serial.begin(9600);

  //resets the EEPROM before use, all saved objects erased
  ezprom.reset();
  ezprom.setOverwriteIfSizeDifferent(false);
}

void loop() {
  //check if string is available in Serial
  if (Serial.available() > 0) {
    bool saved = false;
    while (Serial.available() > 0) {
      //a string to hold the string
      char buf[string_size_max];
      int len = Serial.readBytesUntil('\n', buf, string_size_max - 1); //read the string
      buf[len] = '\0'; //null-terminate it

      //save the string into ezprom with its own ID
      saved = ezprom.save(strings, *buf, len + 1);
      //increase string index if save was successful
      if (saved) {
        strings++;
      }
    }

    //print all saved strings
    for (char i = 0; i < strings; i++) {
      //load the string
      char loadedString[string_size_max];
      bool loaded = ezprom.load(i, *loadedString);

      //print the string
      if (loaded) {
        Serial.print("String ");
        Serial.print((int) i);
        Serial.print(": ");
        Serial.println(loadedString);
      } else {
        Serial.print("Unable to load string ");
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


#include <EZPROM.h>

struct Reading {
public:
    long time;
    int value;
};

class History : public EZPROM::Serializable {
public:
    static const uint8_t history_length = 16;
    Reading * readings[history_length];
    
    History() {
        for (uint8_t i = 0; i < history_length; i++) {
            readings[i] = new Reading();
        }
    }

    ~History() {
        for (uint8_t i = 0; i < history_length; i++) {
            delete readings[i];
        }
    }

    virtual void serialize(uint8_t* stream, uint16_t & index) {
        for (int i = 0; i < history_length; i++) {
            //saves each reading object
            putObject(* readings[i], stream, index);
        }
    }

    virtual void deserialize(uint8_t* stream, uint16_t & index) {
        for (int i = 0; i < history_length; i++) {
            //loads each reading object
            getObject(* readings[i], stream, index);
        }
    }

    virtual uint16_t size() {
        //the size of this class is equal to sum of the reading objects
        return sizeof(Reading) * history_length;
    }
};

void setup() {
    Serial.begin(9600);

    ezprom.reset();

    History original = History();
    //iterate through all of the Reading objects
    Serial.println("Saved values: ");
    for (int i = 0; i < History::history_length; i++) {
        //set the reading value
        original.readings[i]->value = i + 1;
        //print the value
        Serial.println(original.readings[i]->value);
    }
    //save the History object
    ezprom.saveSerial(0, & original);

    History copy = History();
    //load the History object
    ezprom.loadSerial(0, & copy);
    //iterate through all the Reading objects
    Serial.println("Loaded values: ");
    for (int i = 0; i < History::history_length; i++) {
        //print the value
        Serial.println(copy.readings[i]->value);
    }
}

void loop() {
}

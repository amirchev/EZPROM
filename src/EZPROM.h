#ifndef EZPROM_H
#define EZPROM_H

#include <Arduino.h>
#include <EEPROM.h>

class EZPROM {
private:
    bool overwriteDiffSize = false;
public:
    //TODO: figure out way to save whether init, and check when starting

    void reset() {
        EEPROM.put(EEPROM.length() - sizeof (uint8_t), (uint8_t) 0);
    }

    struct ObjectData {
        uint8_t id;
        uint16_t size;
    };

    template<typename T> bool save(uint8_t id, T const &object, unsigned int elements = 1) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);

        //check if id exists
        uint8_t index = 0;
        bool hasId = false;
        bool hasSpace = false;
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                index = i;
                hasId = true;
                break;
            }
        }

        if (hasId) {
            if (objects[index].size == sizeof (T) * elements) {
                //overwrite object
                ramToEEPROM(getAddress(objects, index), object, objects[index].size);
                return true;
            } else if (overwriteDiffSize) {
                //calculate space totalSize
                unsigned int totalSize = 0;
                for (uint8_t i = 0; i < objectAmount; i++) { //add all objects
                    if (i != index) {
                        totalSize += objects[i].size;
                    }
                }
                totalSize += sizeof (uint8_t) + sizeof (ObjectData) * objectAmount; //add ObjectData array & length number
                totalSize += sizeof (T) * elements;
                if (totalSize <= EEPROM.length()) {
                    hasSpace = true;
                    remove(id);
                    //update objects array
                    for (uint8_t i = index; i < objectAmount - 1; i++) {
                        objects[i] = objects[i + 1];
                    }
                    objectAmount--;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

        if (!hasSpace) {
            //calculate space totalSize
            unsigned int totalSize = 0;
            for (uint8_t i = 0; i < objectAmount; i++) { //add all objects
                totalSize != objects[i].size;
            }
            totalSize += sizeof (uint8_t) + sizeof (ObjectData) * objectAmount; //add ObjectData array & length number
            totalSize += sizeof (T) * elements;
            hasSpace = totalSize <= EEPROM.length();
            Serial.print("Total size: ");
            Serial.println(totalSize);
        }

        if (hasSpace) {
            ObjectData updatedObjects[objectAmount + 1];
            for (uint8_t i = 0; i < objectAmount; i++) {
                updatedObjects[i] = objects[i];
            }
            index = objectAmount;
            ObjectData thisObjectData;
            thisObjectData.id = id;
            thisObjectData.size = sizeof (T) * elements;
            updatedObjects[index] = thisObjectData;

            for (uint8_t i = 0; i < objectAmount + 1; i++) {
                Serial.print("Object ");
                Serial.print(i);
                Serial.print(" id ");
                Serial.print(updatedObjects[i].id);
                Serial.print(": size ");
                Serial.print(updatedObjects[i].size);
                Serial.print(" address ");
                Serial.println(getAddress(updatedObjects, i));
            }
            //save
            ramToEEPROM(getAddress(updatedObjects, index), object, updatedObjects[index].size);
            saveObjectData(updatedObjects, objectAmount + 1);
            return true;
        } else {
            return false;
        }
    }

    template<typename T> bool load(uint8_t id, T &object) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);

        //check if id exists
        uint8_t index = 0;
        bool hasId = false;
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                index = i;
                hasId = true;
                break;
            }
        }

        if (hasId) {
            unsigned int address = getAddress(objects, index);
            uint8_t * ram = (uint8_t *) & object;
            for (unsigned int i = 0; i < objects[index].size; i++) {
                ram[i] = EEPROM.read(address + i);
            }
            return true;
        }
        return false;
    }

    void remove(uint8_t id) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);

        //check if id exists
        uint8_t index = 0;
        bool hasId = false;
        bool hasSpace = false;
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                index = i;
                hasId = true;
                break;
            }
        }

        if (hasId) {
            ObjectData updatedObjects[objectAmount - 1];
            for (uint8_t i = 0; i < objectAmount; i++) {
                if (i < index) {
                    updatedObjects[i] = objects[i];
                } else if (i > index) {
                    updatedObjects[i - 1] = objects[i];
                }
            }
            for (uint8_t i = index; i < objectAmount - 1; i++) {
                unsigned int oldAddress = getAddress(objects, i + 1);
                unsigned int newAddress = getAddress(updatedObjects, i);
                for (unsigned int j = 0; j < updatedObjects[i].size; j++) {
                    EEPROM.update(newAddress + j, EEPROM.read(oldAddress + j));
                }
            }
            saveObjectData(updatedObjects, objectAmount - 1);
        }
    }

    void setOverwriteIfSizeDifferent(bool b) {
        overwriteDiffSize = b;
    }

    ObjectData getObjectData(uint8_t id) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                return objects[i];
            }
        }
        ObjectData badObject;
        badObject.id = id;
        badObject.size = 0;
        return badObject;
    }

private:

    void saveObjectData(ObjectData * objectData, uint8_t objectAmount) {
        //calculate starting address
        unsigned int startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
        //save object data
        for (uint8_t i = 0; i < objectAmount; i++) {
            EEPROM.put(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
        }
        //save length of array
        EEPROM.put(EEPROM.length() - sizeof (uint8_t), objectAmount);
    }

    void loadObjectData(ObjectData * objectData, uint8_t objectAmount) {
        //load all objects
        unsigned int startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
        for (uint8_t i = 0; i < objectAmount; i++) {
            EEPROM.get(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
        }
    }

    uint8_t getObjectAmount() {
        //read amount from last address on EEPROM
        uint8_t objectAmt = 0;
        EEPROM.get(EEPROM.length() - sizeof (uint8_t), objectAmt);
        return objectAmt;
    }

    unsigned int getAddress(ObjectData * objects, uint8_t index) {
        unsigned int address = 0;
        for (uint8_t i = 0; i < index; i++) {
            address += objects[i].size;
        }
        return address;
    }

    template<typename T> void ramToEEPROM(unsigned int address, T const &object, unsigned int size) {
        uint8_t * ram = (uint8_t *) & object;
        for (unsigned int i = 0; i < size; i++) {
            EEPROM.update(address + i, ram[i]);
        }
    }
};

#endif /* EZPROM */


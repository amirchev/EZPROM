#ifndef EZPROM_H
#define EZPROM_H

#include <Arduino.h>
#include <EEPROM.h>

/**
 * EZPROM allows for easy manipulation of EEPROM memory. It allows for objects
 * to be stored to and retrieved from EEPROM with an ID number instead of an address.
 * Any type of object can be stored, including pointers and multidimensional arrays.
 * 
 * Each objects ID number and size are saved into EEPROM as well as the object.
 * This adds an additional 3 bytes into EEPROM with each object saved. This means
 * an array of objects will be less costly to save compared to individual objects. 
 * 
 * Each saved object can be overwritten in size. For example, if a char[32] is
 * saved at some point and a char[64] is saved into the same ID at a later pointer,
 * the size of the object will be updated and the char[64] object will be accommodated.
 * This functionality requires that setOverwriteIfSizeDifferent is set to true.
 * 
 * The last byte of EEPROM is used to store the amount of objects currently saved
 * by EZPROM.
 */
class EZPROM {
private:
    bool overwriteDiffSize = false;
public:

    /**
     * Clears all objects from EZPROM. Data is not actually modified except for
     * the last byte which is set to 0. The last byte of EEPROM stores the current
     * amount of objects being managed by EZPROM.
     */
    void reset() {
        EEPROM.put(EEPROM.length() - sizeof (uint8_t), (uint8_t) 0);
    }

    /**
     * Holds information regarding objects stored to EEPROM.
     */
    struct ObjectData {
        uint8_t id;
        uint16_t size;
    };

    /**
     * Stores an object and assigns it the given ID. Any object is stored as follows:
     * int i = 5;
     * save(0, i);
     * 
     * Any array can be saved as follows:
     * int i[5];
     * int j[5][5];
     * int k[5][5][5];
     * save(0, *i, 5);
     * save(1, **j, 5 * 5);
     * save(1, ***k, 5 * 5 * 5);
     * 
     * @param id The ID assigned to the object which can be used to retrieve
     * the object later using #load
     * @param src The object to be stored.
     * @param elements The number of elements if the object is an array.
     * @return True if the save was successful, false if there was no space on
     * EEPROM or overwrite of same IDs is not allowed if the size is different.
     */
    template<typename T> bool save(uint8_t id, T const &src, uint16_t elements = 1) {
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
                ramToEEPROM(getAddress(objects, index), src, objects[index].size);
                return true;
            } else if (overwriteDiffSize) {
                //calculate space totalSize
                uint16_t totalSize = 0;
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
            uint16_t totalSize = 0;
            for (uint8_t i = 0; i < objectAmount; i++) { //add all objects
                totalSize += objects[i].size;
            }
            totalSize += sizeof (uint8_t) + sizeof (ObjectData) * objectAmount; //add ObjectData array & length number
            totalSize += sizeof (T) * elements + sizeof (ObjectData); //add new object with its ObjectData
            hasSpace = totalSize <= EEPROM.length();
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
            //save
            ramToEEPROM(getAddress(updatedObjects, index), src, updatedObjects[index].size);
            saveObjectData(updatedObjects, objectAmount + 1);
            return true;
        } else {
            return false;
        }
    }

    /**
     * Loads the object with the specified ID.
     * 
     * @param id The ID of the object to be retrieved.
     * @param dest The object which will hold the retrieved object.
     * @return True if the object was retrieved, false if the ID does not exist.
     */
    template<typename T> bool load(uint8_t id, T &dest) {
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
            uint16_t address = getAddress(objects, index);
            uint8_t * ram = (uint8_t *) & dest;
            for (uint16_t i = 0; i < objects[index].size; i++) {
                ram[i] = EEPROM.read(address + i);
            }
            return true;
        }
        return false;
    }

    /**
     * Removes the object with the specified ID.
     * @param id The ID of the object to be removed.
     */
    void remove(uint8_t id) {
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
            ObjectData updatedObjects[objectAmount - 1];
            for (uint8_t i = 0; i < objectAmount; i++) {
                if (i < index) {
                    updatedObjects[i] = objects[i];
                } else if (i > index) {
                    updatedObjects[i - 1] = objects[i];
                }
            }
            for (uint8_t i = index; i < objectAmount - 1; i++) {
                uint16_t oldAddress = getAddress(objects, i + 1);
                uint16_t newAddress = getAddress(updatedObjects, i);
                for (uint16_t j = 0; j < updatedObjects[i].size; j++) {
                    EEPROM.update(newAddress + j, EEPROM.read(oldAddress + j));
                }
            }
            saveObjectData(updatedObjects, objectAmount - 1);
        }
    }

    /**
     * Specifies if overwriting the same with an object that is a different
     * size than the original is okay. Although it can be convenient, frequently
     * overwriting the same ID with objects of different sizes can increase the
     * wear on EEPROM as objects behind the one whose size is changing must also
     * be rewritten to EEPROM.
     * 
     * @param b Whether the previously saved object at a specific ID can be 
     * overwritten by a new object with a different size.
     */
    void setOverwriteIfSizeDifferent(bool b) {
        overwriteDiffSize = b;
    }

    /**
     * Retrieves the data of the object at a specified ID.
     * 
     * @param id The ID of the object whose data is to be retrieved.
     * @return An #ObjectData object with the ID of the object and its size in EEPROM.
     */
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
    
    bool exists(uint8_t id) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);
        
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * Retrieves the amount of objects currently managed by EZPROM.
     * @return The amount of objects in EZPROM.
     */
    uint8_t getObjectAmount() {
        //read amount from last address on EEPROM
        uint8_t objectAmt = 0;
        EEPROM.get(EEPROM.length() - sizeof (uint8_t), objectAmt);
        return objectAmt;
    }
    
    /**
     * Retrieves the address in EEPROM of the object with the specified ID.
     * @param id The ID of the object whose address is to be retrieved.
     * @return The address of the object in EEPROM or the length of EEPROM if 
     * an object with that ID does not exist.
     */
    uint16_t getAddress(uint8_t id) {
        //load object data
        uint8_t objectAmount = getObjectAmount();
        ObjectData objects[objectAmount];
        loadObjectData(objects, objectAmount);
        
        for (uint8_t i = 0; i < objectAmount; i++) {
            if (objects[i].id == id) {
                return getAddress(objects, i);
            }
        }
        return EEPROM.length();
    }

private:

    void saveObjectData(ObjectData * objectData, uint8_t objectAmount) {
        //calculate starting address
        uint16_t startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
        //save object data
        for (uint8_t i = 0; i < objectAmount; i++) {
            EEPROM.put(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
        }
        //save length of array
        EEPROM.put(EEPROM.length() - sizeof (uint8_t), objectAmount);
    }   

    void loadObjectData(ObjectData * objectData, uint8_t objectAmount) {
        //load all objects
        uint16_t startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
        for (uint8_t i = 0; i < objectAmount; i++) {
            EEPROM.get(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
        }
    }

    uint16_t getAddress(ObjectData * objects, uint8_t index) {
        uint16_t address = 0;
        for (uint8_t i = 0; i < index; i++) {
            address += objects[i].size;
        }
        return address;
    }

    template<typename T> void ramToEEPROM(uint16_t address, T const &object, uint16_t size) {
        uint8_t * ram = (uint8_t *) & object;
        for (uint16_t i = 0; i < size; i++) {
            EEPROM.update(address + i, ram[i]);
        }
    }
} ezprom;

#endif /* EZPROM_H */


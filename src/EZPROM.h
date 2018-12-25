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
     * This abstract class can be extended to provide serialization functionality,
     * allowing more control over how derived classes are saved into and retrieved
     * from EEPROM. The deriving class must implement #serialize, #deserialize,
     * and #size. To save to EEPROM, one can use EZPROM::saveSerial and to load,
     * one can use EEPROM::loadSerial.
     */
    class Serializable {
    public:
        /**
         * Called by #saveSerial when saving a Serializable class to EEPROM.
         * @param stream the byte stream used for saving the data; it's size is
         * determined by the #size function
         * @param index this is the index in the stream to which objects are being
         * written; generally, the derived class should not be modifying it. It is
         * passed to #putObject calls and calls to other #serialize methods
         */
        virtual void serialize(uint8_t * stream, uint16_t & index) = 0;
        /**
         * Called by #loadSerial when loading a Serializable class from EEPROM.
         * @param stream the byte stream used for retrieving data; it's size was 
         * determined on save, and will be the same
         * @param index the index in the stream from which objects are being retrieved;
         * generally,the derived class should not be modifying it. It is passed to
         * #getObject and #deserialize where it is incremented appropriately.
         */
        virtual void deserialize(uint8_t * stream, uint16_t & index) = 0;
        /**
         * Called by #saveSerial to determine the size of the byte stream necessary
         * to hold the contents of the Serializable class.
         * @return the size of the Serializable class in bytes.
         */
        virtual uint16_t size() = 0;

        /**
         * Writes an object into the byte stream, incrementing the index appropriately.
         * @param object the object to be written to the stream
         * @param stream the stream which will hold the object
         * @param index the index at which to begin writing to the stream, which
         * is incremented equal to the size of the object that is written
         */
        template<typename T> void putObject(const T &src, uint8_t * stream, uint16_t &index) {
            uint16_t size = sizeof (T);
            uint8_t * ram = (uint8_t *) & src;
            for (uint16_t i = 0; i < size; i++) {
                stream[index++] = ram[i];
            }
        }

        /**
         * Reads an object from the byte stream, incrementing the index appropriately.
         * @param dest the object to which the retrieved value will be written into
         * @param stream the stream which holds the object
         * @param index the index at which to begin reading from the stream, which
         * is incremented equal to the size of the object that is read
         */
        template<typename T> void getObject(T & dest, uint8_t * stream, uint16_t &index) {
            uint16_t size = sizeof (T);
            uint8_t * ram = (uint8_t *) & dest;
            for (uint16_t i = 0; i < size; i++) {
                ram[i] = stream[index++];
            }
        }
    };

    /**
     * Clears all objects from EZPROM. Data is not actually modified except for
     * the last byte which is set to 0. The last byte of EEPROM stores the current
     * amount of objects being managed by EZPROM.
     */
    void reset();

    /**
     * Stores the id and size of objects stored into EEPROM.
     */
    struct ObjectData {
        uint8_t id;
        uint16_t size;
    };

    /**
     * Stores an object and assigns it the given ID. Any object is stored as follows:
     * int i = 5;
     * ezprom.save(id, i);
     * 
     * Any array can be saved as follows:
     * int i[5];
     * int j[5][5];
     * int k[5][5][5];
     * ezprom.save(i_id, *i, 5);
     * ezprom.save(j_id, **j, 5 * 5);
     * ezprom.save(k_id, ***k, 5 * 5 * 5);
     * 
     * @param id The ID assigned to the object which can be used to retrieve
     * the object later using #load
     * @param src The object to be stored.
     * @param elements The number of elements if the object is an array.
     * @return True if the save was successful, false if there was no space on
     * EEPROM or overwrite of same IDs is not allowed if the size is different.
     */
    template<typename T>
    bool save(uint8_t id, const T& src, uint16_t elements = 1) {
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
     * Loads the object with the specified ID. Any object can be loaded as follows:
     * int dest;
     * ezprom.load(id, dest);
     * 
     * Any array can be loaded as follows:
     * int i[5];
     * int j[5][5];
     * int k[5][5][5];
     * ezprom.load(i_id, *i);
     * ezprom.load(j_id, **j);
     * ezprom.load(k_id, ***k);
     * 
     * @param id The ID of the object to be retrieved.
     * @param dest The object which will hold the retrieved object.
     * @return True if the object was retrieved, false if the ID does not exist.
     */
    template<typename T> bool load(uint8_t id, T& dest) {
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

    bool saveSerial(uint8_t id, const Serializable * src);

    bool loadSerial(uint8_t id, Serializable * dest);

    /**
     * Removes the object with the specified ID.
     * @param id The ID of the object to be removed.
     */
    void remove(uint8_t id);

    /**
     * Specifies if overwriting the same with an object that is a different
     * size than the original is okay. Although it can be convenient, frequently
     * overwriting the same ID with objects of different sizes can increase the
     * wear on EEPROM as objects behind the one whose size is changing must also
     * be rewritten to EEPROM. This value is false by default.
     * 
     * @param b Whether the previously saved object at a specific ID can be 
     * overwritten by a new object with a different size.
     */
    void setOverwriteIfSizeDifferent(bool b);

    /**
     * Retrieves the data of the object at a specified ID.
     * 
     * @param id The ID of the object whose data is to be retrieved.
     * @return An #ObjectData object with the ID of the object and its size in EEPROM.
     */
    ObjectData getObjectData(uint8_t id);

    bool exists(uint8_t id);

    /**
     * Retrieves the amount of objects currently managed by EZPROM.
     * @return The amount of objects in EZPROM.
     */
    uint8_t getObjectAmount();

    /**
     * Retrieves the address in EEPROM of the object with the specified ID.
     * @param id The ID of the object whose address is to be retrieved.
     * @return The address of the object in EEPROM or the length of EEPROM if 
     * an object with that ID does not exist.
     */
    uint16_t getAddress(uint8_t id);

private:

    void saveObjectData(ObjectData * objectData, uint8_t objectAmount);

    void loadObjectData(ObjectData * objectData, uint8_t objectAmount);

    uint16_t getAddress(ObjectData * objects, uint8_t index);

    template<typename T>
    void ramToEEPROM(uint16_t address, const T& object, uint16_t size) {
        uint8_t * ram = (uint8_t *) & object;
        for (uint16_t i = 0; i < size; i++) {
            EEPROM.update(address + i, ram[i]);
        }
    }
};

extern EZPROM ezprom;
#endif /* EZPROM_H */


#include "EZPROM.h"

EZPROM ezprom;

void EZPROM::reset() {
    EEPROM.put(EEPROM.length() - sizeof (uint8_t), (uint8_t) 0);
}

bool EZPROM::setup(uint16_t uniqueInt, uint8_t id = UNIQUE_INT_ID) {
	if (!isValid(uniqueInt, id)) {
		reset();
		setUniqueId(uniqueInt, id);
		return true;
	}
	return false;
}

bool EZPROM::isValid(uint16_t uniqueInt, uint8_t id = UNIQUE_INT_ID) {
	uint16_t curInt = 0;
	if (ezprom.exists(id)
			&& ezprom.getObjectData(id).size == sizeof(uint16_t)) {
		ezprom.load(id, curInt);
	}
	return curInt == uniqueInt;
}

void EZPROM::setUniqueId(uint16_t uniqueInt, uint8_t id = UNIQUE_INT_ID) {
	ezprom.save(id, uniqueInt);
}

bool EZPROM::saveSerial(uint8_t id, const Serializable* src) {
    uint16_t size = src->size();
    uint8_t stream[size];
    uint16_t index = 0;
    src->serialize(stream, index);
    return save(id, * stream, size);
}

bool EZPROM::loadSerial(uint8_t id, Serializable* dest) {
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
        uint8_t stream[objects[index].size];
        uint16_t address = getAddress(objects, index);
        for (uint16_t i = 0; i < objects[index].size; i++) {
            stream[i] = EEPROM.read(address + i);
        }
        uint16_t serialIndex = 0;
        dest->deserialize(stream, serialIndex);
        return true;
    }
    return false;
}

bool EZPROM::exists(uint8_t id) {
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

uint16_t EZPROM::getAddress(uint8_t id) {
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

uint16_t EZPROM::getAddress(ObjectData* objects, uint8_t index) {
    uint16_t address = 0;
    for (uint8_t i = 0; i < index; i++) {
        address += objects[i].size;
    }
    return address;
}

uint8_t EZPROM::getObjectAmount() {
    //read amount from last address on EEPROM
    uint8_t objectAmt = 0;
    EEPROM.get(EEPROM.length() - sizeof (uint8_t), objectAmt);
    return objectAmt;
}

EZPROM::ObjectData EZPROM::getObjectData(uint8_t id) {
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

void EZPROM::remove(uint8_t id) {
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

void EZPROM::setOverwriteIfSizeDifferent(bool b) {
    overwriteDiffSize = b;
}

void EZPROM::saveObjectData(ObjectData* objectData, uint8_t objectAmount) {
    //calculate starting address
    uint16_t startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
    //save object data
    for (uint8_t i = 0; i < objectAmount; i++) {
        EEPROM.put(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
    }
    //save length of array
    EEPROM.put(EEPROM.length() - sizeof (uint8_t), objectAmount);
}

void EZPROM::loadObjectData(ObjectData* objectData, uint8_t objectAmount) {
    //load all objects
    uint16_t startingAddress = EEPROM.length() - (sizeof (uint8_t) + sizeof (ObjectData) * objectAmount);
    for (uint8_t i = 0; i < objectAmount; i++) {
        EEPROM.get(startingAddress + (i * sizeof (ObjectData)), objectData[i]);
    }
}


# EZPROM

1. [Introduction](#introduction)
2. [Examples](#examples)
3. [Documentation](#documentation)

## Introduction

EZPROM allows for easy manipulation of EEPROM memory. It allows for objects to be stored to and retrieved from EEPROM with an ID number instead of an address. Any type of object can be stored, including pointers and multidimensional arrays.

### How it works
Each objects ID and size are saved into EEPROM as well as the object. This adds an additional 3 bytes into EEPROM with each object saved. This means an array of objects will be less costly to save compared to individual objects. 

Each saved object can be overwritten in size. For example, if a `char[32]` is saved at some point and a `char[64]` is saved into the same ID at a later pointer, the size of the object will be updated and the `char[64]` object will be accommodated. This functionality requires that `setOverwriteIfSizeDifferent` is set to true.

The last byte of EEPROM is used to store the amount of objects currently saved by EZPROM.

## Examples
Before you use EZPROM with your program the first time, you must call `reset`. This will format EEPROM so that it can be used by EZPROM. This only needs to be done once. It can also be done every time you need to delete all objects currently managed by EZPROM. Here is an example:
```
void setup() {
  //only do once, if this is called every setup then
  //information will be lost between power cycles
  ezprom.reset();
}
```

Here is the declaration of constants used in the two examples below:
```
//the value of ids does not matter, as long as they are unique
//ids must be of type uint8_t
const uint8_t pwd_id = 0,
  port_id = 1,
  msgs_id = 2;
const uint8_t pwd_size = 32,
  msg_amt = 6,
  msg_size = 32;
```

Use the `save` method to save objects to EZPROM:
```
void setPort(long port) {
  ezprom.save(port_id, port);
}

void setPwd(char * pwd) {
  //when saving an array, add an asterisk for every dimension
  //also, the total amount of elements in the array must be passed forward
  ezprom.save(pwd_id, *pwd, pwd_size);
}

void saveMessages(char ** messages) {
  //since this is a two-dimensional array, you must use two
  //asterisks when saving it
  //also, the total elements in a two-dimensional array is
  //calculated by multiplying the width by the height of the array
  ezprom.save(msgs_id, **messages, msg_amt * msg_size);
}
```

To load an object you simply call the `load` method:
```
long getPort() {
  long port;
  ezprom.load(port_id, port);
}

void getPwd(char * dest) {  
  //just like when saving, use an asterisk for every dimension
  //of the array you are loading
  ezprom.load(pwd_id, *dest);
}

void getMessages(char ** dest) {
  //when loading a two-dimensional array, use two asterisks
  ezprom.load(msgs_id, **dest);
}
```

## Documentation

1. [void reset()](#void-reset)
2. [struct ObjectData](#struct-objectdata)
3. [bool save(uint8_t, T const &, uint16_t)](#bool-saveuint8_t-id-t-const-src-uint16_t-elements--1)
4. [bool load(uint8_t, T &)](#bool-loaduint8_t-id-t-dest)
5. [void remove(uint8_t)](#void-removeuint8_t-id)
6. [void setOverwriteIfSizeDifferent(bool)](#void-setoverwriteifsizedifferentbool-b)
7. [ObjectData getObjectData(uint8_t)](#objectdata-getobjectdatauint8_t-id)
8. [uint8_t getObjectAmount()](#uint8_t-getobjectamount)
9. [uint16_t getAddress(uint8_t)](#uint16_t-getaddressuint8_t-id)

### void reset()
Clears all objects from EZPROM. Data is not actually modified except for the last byte which is set to `0`. The last byte of EEPROM stores the current amount of objects being managed by EZPROM.

### struct ObjectData
Stores the id and size of objects stored into EEPROM.

### bool save(uint8_t id, T const &src, uint16_t elements = 1)
Stores an object and assigns it the given ID. Any object is stored as follows:

```
int i = 5;
save(id, i);
```

Any array can be saved as follows:
```
int i[5];
int j[5][5];
int k[5][5][5];
save(i_id, *i, 5);
save(j_id, **j, 5 * 5);
save(k_id, ***k, 5 * 5 * 5);
```

#### @param id 
The ID assigned to the object which can be used to retrieve the object later using `load`.
#### @param src
The object to be stored.
#### @param elements
The number of elements if the object is an array.
#### @return
`true` if the save was successful, `false` if there was no space on EEPROM or overwrite of same IDs is not allowed if the size is different.

### bool load(uint8_t id, T &dest)

Loads the object with the specified ID. Any object can be loaded as follows:
```
int dest;
ezprom.load(id, dest);
```

Any array can be loaded as follows:
```
int i[5];
int j[5][5];
int k[5][5][5];
ezprom.load(i_id, *i);
ezprom.load(j_id, **j);
ezprom.load(k_id, ***k);
```

#### @param id 
The ID of the object to be retrieved.
#### @param dest 
The object which will hold the retrieved object.
#### @return 
`true` if the object was retrieved, `false` if the ID does not exist.

### void remove(uint8_t id)
Removes the object with the specified ID.
#### @param id
The ID of the object to be removed.

### void setOverwriteIfSizeDifferent(bool b)
Specifies if overwriting the same with an object that is a different size than the original is okay. Although it can be convenient, frequently overwriting the same ID with objects of different sizes can increase the wear on EEPROM as objects behind the one whose size is changing must also be rewritten to EEPROM. This value is `false` by default.

#### @param b 
Whether the previously saved object at a specific ID can be overwritten by a new object with a different size.

### ObjectData getObjectData(uint8_t id)
Retrieves the data of the object at a specified ID.
#### @param id 
The ID of the object whose data is to be retrieved.
#### @return 
An `ObjectData` object with the ID of the object and its size in EEPROM.

### uint8_t getObjectAmount()
Retrieves the amount of objects currently managed by EZPROM.
#### @return 
The amount of objects in EZPROM.

### uint16_t getAddress(uint8_t id)
retrieves the address in EEPROM of the object with the specified ID.
#### @param id 
The ID of the object whose address is to be retrieved.
#### @return 
The address of the object in EEPROM or the length of EEPROM if an object with that ID does not exist.

#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    EEPROM.begin(4096);
    delay(1);
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
      EEPROM.write(ee++, *p++);
    }
    delay(1);
    EEPROM.commit();
    delay(1);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    EEPROM.begin(4096);
    delay(1);
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++) {
      *p++ = EEPROM.read(ee++);
    }
    delay(1);  
    EEPROM.commit();
    delay(1);
    return i;
}

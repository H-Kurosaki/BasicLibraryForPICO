#ifndef _MyMACROM_h
#define _MyMACROM_h

#include "Arduino.h"

#define EEPROM_93C46_8BITMODE 	0
#define EEPROM_93C46_16BITMODE 1

#define 	EEPROM_93C46_WRITE 		0x40
#define 	EEPROM_93C46_READ 		0x80
#define 	EEPROM_93C46_ERASE 		0xC0
#define 	EEPROM_93C46_EW_DISABLE 	0x00
#define 	EEPROM_93C46_EW_ENABLE 	0x30

class MACROM93C46 {
  public:
  	MACROM93C46(int pDI, int pDO,int pSK,int pCS);
    void begin(bool longMode=EEPROM_93C46_16BITMODE);
    word read(byte addr);
  	void writeEnable();
  	void writeDisable();
  	bool write(byte addr, word value);
  private:
  	void bitsSend(word value, int len);
  	bool wait();
  	int pinCS;
  	int pinSK;
  	int pinDI;
  	int pinDO;
  	bool _WRITE_ENABLED;
    bool _BITMODE;

};

#endif

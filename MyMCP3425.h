
#ifndef _MCP3425_H_
#define _MCP3425_H_

#include "Arduino.h"
#include "Wire.h"

#define MCP3425_address 0x68
#define MCP3425_VREF    2.048
#define MCP3425_configRegister 0b10011000 //16bit 15sps PGA x1 

class MCP3425 {
  public:
  MCP3425(TwoWire &w);
	void begin(void);
	double readVolt(void);
	int16_t readADC(void);
	bool IsReadOK();
  private:
	TwoWire *wire;
    bool readFlag;
};

#endif

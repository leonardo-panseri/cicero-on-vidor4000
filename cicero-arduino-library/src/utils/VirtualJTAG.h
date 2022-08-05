/*****************************************************************************************************
 VirtualJTAG - a library to read and write 32 and 64 bit registers through the Virtual JTAG protocol.
******************************************************************************************************/

#ifndef VirtualJTAG_h
#define VirtualJTAG_h

#include <Arduino.h>

/**
  Reads the 32 bit virtual data register associated with the given virtual instruction register value through the Virtual JTAG protocol.
  
  @param VIR the virtual instruction register value to select the virtual data register to read from
  @return a 32 bit unsigned integer representing the data contained in the virtual data register
*/
uint32_t readRegister32(uint8_t VIR);
/**
  Writes the 32 bit virtual data register associated with the given virtual instruction register value through the Virtual JTAG protocol.
  
  @param VIR the virtual instruction register value to select the virtual data register to write to
  @param data a 32 bit unsigned integer representing the data to write to the virtual data register
*/
void writeRegister32(uint8_t VIR, uint32_t data);

/**
  Reads the 64 bit virtual data register associated with the given virtual instruction register value through the Virtual JTAG protocol.
  
  @param VIR the virtual instruction register value to select the virtual data register to read from
  @return a 64 bit unsigned integer representing the data contained in the virtual data register
*/
uint64_t readRegister64(uint8_t VIR);
/**
  Writes the 64 bit virtual data register associated with the given virtual instruction register value through the Virtual JTAG protocol.
  
  @param VIR the virtual instruction register value to select the virtual data register to write to
  @param data a 64 bit unsigned integer representing the data to write to the virtual data register
*/
void writeRegister64(uint8_t VIR, uint64_t data);

#endif
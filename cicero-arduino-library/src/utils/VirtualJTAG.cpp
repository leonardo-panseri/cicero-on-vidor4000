/*****************************************************************************************************
 VirtualJTAG - a library to read and write 32 and 64 bit registers through the Virtual JTAG protocol.
******************************************************************************************************/

#include <Arduino.h>
#include "JTAG.h"

uint32_t readRegister32(uint8_t VIR) {
  uint8_t data[4];
  
  JTAG_Read_VDR_from_VIR(VIR, data, 32);

  // Since the library returns the data as a byte array, we need to convert it to a 32 bit unsigned integer
  // NB: the bytes returned are little-endian: the first is the least significant and the last is the most significant
  uint32_t res = 0;
  for (int i = 0; i < 4; i++) {
    res = res | ((uint32_t) data[i]) << 8 * i;
  }
  return res;
}

void writeRegister32(uint8_t VIR, uint32_t data) {
  uint8_t bytes[4];

  // Since the library expects the data as a byte array, we need to convert our input data
  // NB: the bytes in the array are expected to be little-endian: the first is the least significant and the last is the most significant
  for (int i = 0; i < 4; i++) {
    bytes[i] = data >> i * 8;
  }

  JTAG_Write_VDR_to_VIR(VIR, bytes, 32);
}

uint64_t readRegister64(uint8_t VIR) {
  uint8_t data[8];
  
  JTAG_Read_VDR_from_VIR(VIR, data, 64);

  // Since the library returns the data as a byte array, we need to convert it to a 64 bit unsigned integer
  // NB: the bytes returned are little-endian: the first is the least significant and the last is the most significant
  uint64_t res = 0;
  for (int i = 0; i < 8; i++) {
    res = res | ((uint64_t) data[i]) << 8 * i;
  }
  return res;
}

void writeRegister64(uint8_t VIR, uint64_t data) {
  uint8_t bytes[8];

  // Since the library expects the data as a byte array, we need to convert our input data
  // NB: the bytes in the array are expected to be little-endian: the first is the least significant and the last is the most significant
  for (int i = 0; i < 8; i++) {
    bytes[i] = data >> i * 8;
  }
  
  JTAG_Write_VDR_to_VIR(VIR, bytes, 64);
}
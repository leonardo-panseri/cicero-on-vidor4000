/*
  Cicero.h - Library for implementing and using a regular expressions coprocessor for the FPGA on the Arduino MKR Vidor 4000
  Created by Leonardo Panseri & Mattia Sironi, 2022.
  Released into the public domain.
*/

#include <Arduino.h>
#include "Cicero.h"
#include "utils/JTAG.h"
#include "utils/VirtualJTAG.h"

extern void enableFpgaClock(void);

/***************************************************
  Singleton pattern implementation, as this library 
  should only be instantiated once
****************************************************/

/**
  Gets the singleton instance of the library.
  
  @return the Cicero instance
*/
Cicero_ &Cicero_::getInstance() {
  static Cicero_ instance;
  return instance;
}

Cicero_ &Cicero = Cicero.getInstance();

/********************************************
  FPGA related constants and setup functions
*********************************************/

// FPGA Pins
const int MB_INT_PIN = 31;

// FPGA setup constants
#define no_data   0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, \
                  0x00, 0x00, 0x00, 0x00  \

#define NO_BOOTLOADER   no_data
#define NO_APP          no_data
#define NO_USER_DATA    no_data

// Bitstream signature
__attribute__ ((used, section(".fpga_bitstream_signature")))
const unsigned char signatures[4096] = {
  //#include "signature.ttf"
  NO_BOOTLOADER,

  0x00, 0x00, 0x08, 0x00,
  0xA9, 0x6F, 0x1F, 0x00,   // Don't care.
  0x20, 0x77, 0x77, 0x77, 0x2e, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x65, 0x73, 0x2d, 0x65, 0x6d, 0x62, 0x61, 0x72, 0x71, 0x75, 0x65, 0x73, 0x2e, 0x66, 0x72, 0x20, 0x00, 0x00, 0xff, 0xf0, 0x0f,
  0x01, 0x00, 0x00, 0x00,   
  0x01, 0x00, 0x00, 0x00,   // Force

  NO_USER_DATA,
};

// Bitstream data
__attribute__ ((used, section(".fpga_bitstream")))
const unsigned char bitstream[] = {
  #include "utils/app.h"
};

/**
  Loads the bitstream on the FPGA.
*/
static void setupFPGA() {
  static uint8_t data[9] = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  enableFpgaClock();
  jtag_host_setup();
  JTAG_Write_VDR_to_VIR(0x00, data, 66);
  pinMode(MB_INT_PIN, OUTPUT);
  digitalWrite(MB_INT_PIN, LOW);
  data[4] = 0x03;
  JTAG_Write_VDR_to_VIR(0x00, data, 66);
  digitalWrite(MB_INT_PIN, HIGH);
  digitalWrite(MB_INT_PIN, LOW);
  delay(1000);
}

/************************************
  Library constants and setup method
*************************************/

/**
  The maximum accessible address on the RAM of the FPGA.
*/
const int RAM_MAX_ADDRESS = 511;

/*
  Virtual Data Registers addresses.
  These are the addresses to write in the VIR to select the corresponding data register.
*/
// 32 bit registers
const uint8_t VIR_STATUS            = 0x01;
const uint8_t VIR_COMMAND           = 0x02;
const uint8_t VIR_ADDRESS           = 0x03;
const uint8_t VIR_START_CC_POINTER  = 0x04;
const uint8_t VIR_END_CC_POINTER    = 0x05;
const uint8_t VIR_DEBUG             = 0x0F;

// 64 bit registers
const uint8_t VIR_DATA_IN           = 0x06;
const uint8_t VIR_DATA_OUT          = 0x07;

// Cicero commands
const uint32_t CMD_NOP              = 0x0;
const uint32_t CMD_WRITE            = 0x1;
const uint32_t CMD_READ             = 0x2;
const uint32_t CMD_START            = 0x3;
const uint32_t CMD_RESET            = 0x4;
const uint32_t CMD_READ_ELAPSED_CLK = 0x5;
const uint32_t CMD_RESTART          = 0x6;

void Cicero_::begin() {
  setupFPGA();
  
  strStartAddr = 0;
}

/*********************************************
  Methods to interact with CICERO on the FPGA
**********************************************/

bool Cicero_::writeQWordsToRAM(int qwordsToWrite, uint64_t qwords[], uint32_t startingAddr) {
  uint32_t addr = startingAddr;
  bool first = true;
  for (int i = 0; i < qwordsToWrite; i = i + 1) {
    writeRegister32(VIR_ADDRESS, addr);
    writeRegister64(VIR_DATA_IN, qwords[i]);

    if (first) {
      // We need to send the WRITE command to Cicero, as the RAM has a write enable
      first = false;
      writeRegister32(VIR_COMMAND, CMD_WRITE);
    }

    addr = addr + 1;
  }

  if (addr > RAM_MAX_ADDRESS) {
    return false;
  }

  // When the write is finished, we need to send the NOP command to Cicero, to put it back into the idle state
  writeRegister32(VIR_COMMAND, CMD_NOP);
  return true;
}

void Cicero_::readQWordsFromRAM(int qwordsToRead, uint64_t res[], uint32_t startingAddr) {
  uint32_t addr = startingAddr;
  bool first = true;
  for(int i = 0; i < qwordsToRead; i++) {
    writeRegister32(VIR_ADDRESS, addr);

    if (first) {
      // We need to send the READ command to Cicero, as the RAM has a read enable
      first = false;
      writeRegister32(VIR_COMMAND, CMD_READ);
    }

    res[i] = readRegister64(VIR_DATA_OUT);

    addr = addr + 1;
  }

  // When the read is finished, we need to send the NOP command to Cicero, to put it back into the idle state
  writeRegister32(VIR_COMMAND, CMD_NOP);
}

uint32_t Cicero_::loadCode(int numBytes, uint8_t code[]) {
  int qwordsToWrite = ceil(numBytes / 8.0);
  uint64_t qwords[qwordsToWrite];

  uint64_t qword = 0;
  int index = 0;
  int bytesWritten = 0;
  for (int i = 0; i < numBytes - 1; i = i + 2) {
    bytesWritten = bytesWritten + 2;

    // The code needs to be written in RAM as little-endian, but the endianess is refering to words (2 bytes) and not bytes
    // So we need to write the first word in the least significant 2 bytes and the last word in the most significant 2 bytes
    qword = qword | ((uint64_t) code[i]) << 8 * (bytesWritten - 1) | ((uint64_t) code[i + 1]) << 8 * (bytesWritten - 2);

    if (bytesWritten == 8 || i == numBytes - 2) {
      // If we have composed a full qword or if we are at the last byte of code, save the qword to the array
      qwords[index] = qword;
      index = index + 1;
            
      qword = 0;
      bytesWritten = 0;
    }
  }

  writeQWordsToRAM(qwordsToWrite, qwords, 0);

  // Set the string start address as the next free address
  // Since the RAM is addressed by qwords starting from address 0, the next free RAM address is equals to the number of qwords written
  strStartAddr = qwordsToWrite;
  
  return qwordsToWrite;
}

void Cicero_::convertStringToBytes(String string, uint8_t res[]) {
  for (int i = 0; i < string.length(); i++) {
    res[i] = (uint8_t) string[i];
  }
}

uint32_t Cicero_::loadCode(String codeStr) {
  uint8_t strBytes[codeStr.length()];
  convertStringToBytes(codeStr, strBytes);
  
  return loadCode(codeStr.length(), strBytes);
}

void Cicero_::loadStringAndStart(String str) {
  uint8_t strBytes[str.length()];
  convertStringToBytes(str, strBytes);

  // Calculate the address of the first and the last byte as if the RAM was addressed by bytes
  // This is needed by Cicero to understand where to start and stop reading the string
  // Note that the start of the string will always be aligned with the start of a qword
  uint32_t strStartByteAddr = strStartAddr * 8;
  uint32_t strEndByteAddr = strStartByteAddr + str.length();

  int qwordsToWrite = ceil(str.length() / 8.0);
  uint64_t qwords[qwordsToWrite];

  for (int i = 0; i < str.length(); i = i + 8) {
    uint64_t qword = 0;
    
    for (int j = 0; j < 8; j++) {
      if (i + j >= str.length()) break;
      // The string needs to be written in RAM as little-endian refering to bytes
      // So we need to write the first byte in the least significant byte and the last byte in the most significant byte
      qword = qword | ((uint64_t) strBytes[i + j]) << 8 * j;
    }

    qwords[i / 8] = qword;
  }

  writeQWordsToRAM(qwordsToWrite, qwords, strStartAddr);

  writeRegister32(VIR_START_CC_POINTER, strStartByteAddr);
  writeRegister32(VIR_END_CC_POINTER, strEndByteAddr);

  writeRegister32(VIR_COMMAND, CMD_START);
  writeRegister32(VIR_COMMAND, CMD_NOP);
  
  execStartingTime = micros();
}

uint32_t Cicero_::getStatus() {
  return readRegister32(VIR_STATUS);
}

unsigned long Cicero_::getExecTime() {
  return micros() - execStartingTime;
}

uint64_t Cicero_::getElapsedClockCycles() {
    writeRegister32(VIR_COMMAND, CMD_READ);
    
    uint64_t elapsedCC = readRegister64(VIR_DATA_OUT);
    
    writeRegister32(VIR_COMMAND, CMD_NOP);
    
    return elapsedCC;
}

void Cicero_::reset() {
  writeRegister32(VIR_COMMAND, CMD_RESET);
  writeRegister32(VIR_COMMAND, CMD_NOP);
}



/*
  Cicero.h - Library for implementing and using a regular expressions coprocessor for the FPGA on the Arduino MKR Vidor 4000
  Created by Leonardo Panseri & Mattia Sironi, 2022.
  Released into the public domain.
*/

#ifndef Cicero_h
#define Cicero_h

// Cicero statuses
#define CICERO_STATUS_IDLE       0x0
#define CICERO_STATUS_RUNNING    0x1
#define CICERO_STATUS_ACCEPTED   0x2
#define CICERO_STATUS_REJECTED   0x3
#define CICERO_STATUS_ERROR      0x4

#include <Arduino.h>

class Cicero_ {
  /*
  Singleton pattern, as this library should only be instantiated once
  */
  private:
    Cicero_() = default;

  public:
    static Cicero_ &getInstance();

    Cicero_(const Cicero_&) = delete;
    Cicero_ &operator=(const Cicero_&) = delete;
  
  /*
  Library header
  */
  public:
    /**
      Loads CICERO bitstream on the FPGA and initializes global variables.
    */
    void begin();
    /**
      Loads machine code into the RAM of CICERO.
      
      @param numBytes the number of bytes of machine code to write
      @param code an array containing the bytes of machine code, must be of size 'numBytes'
      @return the next free address of the RAM
    */
    uint32_t loadCode(int numBytes, uint8_t code[]);
    /**
      Writes the string into RAM and starts the execution of CICERO.
      
      @param str the string to analyze
    */
    void loadStringAndStart(String str);
    /**
      Gets the current status of CICERO.
      
      @return an int representing the current status of CICERO, possible values are:
                - 0: idle
                - 1: running
                - 2: string accepted
                - 3: string rejected
                - 4: error
    */
    uint32_t getStatus();
    /**
      Gets the time elapsed since loadStringAndStart() has been called.
      
      @return the time in microseconds since CICERO has started execution
    */
    unsigned long getExecTime();
    /**
      Resets CICERO.
    */
    void reset();
    /**
      Writes the given qwords (8 bytes) to the RAM of CICERO, starting from the given address.
      
      @param qwordsToWrite the size of the array of qwords
      @param qwords the array of qwords to write to RAM
      @param startingAddr the address where to start writing the qwords (the RAM is addressed by qwords: 0 is the first qword, 1 is the second qword, ...)
      @return if the qwords have been written to RAM, this will be false only when the given array does not fit in RAM
    */
    bool writeQWordsToRAM(int qwordsToWrite, uint64_t qwords[], uint32_t startingAddr);
    /**
      Reads the given number of qwords (8 bytes) from the RAM of CICERO, starting from the given address.
      
      @param qwordsToRead the number of qwords that will be read
      @param res the array where read qwords will be stores, this must be of size 'qwordsToRead'
      @param startingAddr the address where to start reading the qwords (the RAM is addressed by qwords: 0 is the first qword, 1 is the second qword, ...)
    */
    void readQWordsFromRAM(int qwordsToRead, uint64_t res[], uint32_t startingAddr);
  
  private:
    /**
      The address of the qword where input strings can be loaded.
    */
    uint32_t strStartAddr;
    /**
      Execution starting time, for debugging purposes.
    */
    unsigned long execStartingTime;
    /**
      Converts a String into an array of bytes.
      
      @param string the string to convert
      @param res the pointer to the array of bytes where the converted string will be stored
    */
    void convertStringToBytes(String string, uint8_t res[]);
};

/**
  Global reference to the instance of the library.
*/
extern Cicero_ &Cicero;

#endif
#include <Cicero.h>

/**
  Machine code for CICERO, generated with CICERO Compiler.
  Regex: a+(c|b)+
  Since it is included in the program, a change to the machine code of Cicero always requires to recompile and reupload the sketch.
 */
uint8_t code[] = {
  #include "code.h"
};

/**
  Prints qwords (8 bytes) of the RAM to Serial, starting from address 0.

  @param qwordsToPrint the number of qwords to print
 */
void printRAMContents(int qwordsToPrint) {
  uint64_t qwordsRead[qwordsToPrint];
  Cicero.readQWordsFromRAM(qwordsToPrint, qwordsRead, 0);

  Serial.println();
  Serial.println("RAM contents (HEX):");
  Serial.println("Addr - Content");
  
  for(int i = 0; i < qwordsToPrint; i++) {  
    Serial.print(i);
    Serial.print(" - ");

    // Read every 64 bit integer as an array of chars to print it byte by byte
    unsigned char* bytes = (unsigned char*) &qwordsRead[i];
    // The SAMD CPU is little-endiand, so print the last byte first and so on
    for (int j = 7; j >= 0; j--) {
      // Print leading zero if necessary
      if (bytes[j]<0x10) Serial.print("0");
      
      Serial.print(bytes[j], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial.println();
}

/*
  Global variables
 */
uint32_t status;
String input;
bool waiting;
bool ciceroExecuting;

void setup() {
  // Reset global variables
  status = 0;
  input = "";
  waiting = false;
  ciceroExecuting = false;
  
  // Upload the bitstream of CICERO to the FPGA
  Cicero.begin();

  // The length of the code in bytes is encoded in the first two bytes of the file
  uint16_t codeNumBytes = ((uint16_t) code[1]) | (((uint16_t) code[0]) << 8);
  // Load machine code to CICERO RAM
  Cicero.loadCode(codeNumBytes, code + 2);

  // Initialize Serial
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  /*
    The program can be in one of three states:
    1) if CICERO is not executing and we are not waiting for user input, ask the user for a new string to examine and start waiting for input
    2) if we are waiting for input, append any char received through Serial to the input string until a newline is found, then start the execution of CICERO
    3) if CICERO is executing, read its status and notify the user of status change, if the execution is done reset the variables and go to state 1
   */
  if (!ciceroExecuting) {
    if (!waiting) {
      Serial.print("Input a string to examine: ");
      waiting = true;
    } else {
      while (Serial.available() > 0) {
        char inChar = Serial.read();
        // Read the string until a newline is found
        if (inChar == '\n') {
          // Add the string terminator (needed by Cicero)
          input += '\0';

          Serial.println(input);

          status = Cicero.getStatus();
          if (status != CICERO_STATUS_IDLE) {
            Serial.print("Cicero is in an unexpected state: ");
            Serial.println(status);
          }

          waiting = false;
          ciceroExecuting = true;

          // Load string to examine to CICERO RAM and begin computation
          Cicero.loadStringAndStart(input);
        
          // OPTIONAL: print the first 10 qwords of the RAM to verify that everything is working as expected
          printRAMContents(10);
        } else {
          input += inChar;
        }
      }
    }
  } else {
    uint32_t newStatus = Cicero.getStatus();
    if (newStatus != status) {
      status = newStatus;

      bool restart = false;
      switch (status) {
        case CICERO_STATUS_RUNNING:
          Serial.println("CICERO is running");
          break;
        case CICERO_STATUS_ACCEPTED:
          Serial.println("A match has been found");
          restart = true;
         break;
        case CICERO_STATUS_REJECTED:
          Serial.println("A match has not been found");
          restart = true;
          break;
        case CICERO_STATUS_ERROR:
          Serial.println("CICERO has encountered an error");
          restart = true;
          break;
        default:
          Serial.print("Unknown CICERO state: ");
          Serial.println(status);
          break;
      }

      if (restart) {
        Serial.print("Execution time : ");
        Serial.print(Cicero.getExecTime());
        Serial.println(" microseconds");
        
        Serial.println("");
        
        Cicero.reset();
        
        status = 0;
        input = "";
        ciceroExecuting = false;
      }
    }
  }
}

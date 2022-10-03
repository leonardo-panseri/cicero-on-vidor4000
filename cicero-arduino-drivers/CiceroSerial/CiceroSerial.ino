#include <Cicero.h>

#define DRIVER_STATUS_WAIT_CMD    0x00
#define DRIVER_STATUS_WAIT_REGEX  0x01
#define DRIVER_STATUS_WAIT_TEXT   0x02
#define DRIVER_STATUS_EXECUTING   0x03

#define DRIVER_CMD_REGEX  0x00
#define DRIVER_CMD_TEXT   0x01

// Need to test if this is not conflicting with any possible input value
#define DRIVER_INPUT_TERMINATOR  0xFF

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
uint32_t ciceroStatus;
uint8_t driverStatus;
char inChar;
String input;

void setup() {
  // Reset global variables
  ciceroStatus = CICERO_STATUS_IDLE;
  driverStatus = DRIVER_STATUS_WAIT_CMD;
  input = "";
  
  // Upload the bitstream of CICERO to the FPGA
  Cicero.begin();

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
  switch (driverStatus) {
    case DRIVER_STATUS_WAIT_CMD:
      if(Serial.available() <= 0) return;
      inChar = Serial.read();
      
      switch (inChar) {
        case DRIVER_CMD_REGEX:
          driverStatus = DRIVER_STATUS_WAIT_REGEX;
          input = "";
          break;
        case DRIVER_CMD_TEXT:
          driverStatus = DRIVER_STATUS_WAIT_TEXT;
          input = "";
          break;
      }
      break;
    case DRIVER_STATUS_WAIT_REGEX:
      while (Serial.available() > 0) {
        inChar = Serial.read();
        
        if (inChar == DRIVER_INPUT_TERMINATOR) {
          Cicero.loadCode(input);
          driverStatus = DRIVER_STATUS_WAIT_CMD;
        } else {
          input += inChar;
        }
      }
      break;
    case DRIVER_STATUS_WAIT_TEXT:
      while (Serial.available() > 0) {
        inChar = Serial.read();
      
        if (inChar == DRIVER_INPUT_TERMINATOR) {
          driverStatus = DRIVER_STATUS_WAIT_CMD;
        } else if (inChar == '\n') {
          // Add the string terminator (needed by Cicero)
          input += '\0';
          // Load string to examine to CICERO RAM and begin computation
          Cicero.loadStringAndStart(input);
          
          driverStatus = DRIVER_STATUS_EXECUTING;
        } else {
          input += inChar;
        }
      }
      break;
    case DRIVER_STATUS_EXECUTING:
      uint32_t newStatus = Cicero.getStatus();
      if (newStatus != ciceroStatus) {
        ciceroStatus = newStatus;
  
        bool restart = false;
        switch (ciceroStatus) {
          case CICERO_STATUS_ACCEPTED:
          case CICERO_STATUS_REJECTED:
          case CICERO_STATUS_ERROR:
            Serial.print(ciceroStatus);
            restart = true;
            break;
        }
  
        if (restart) {          
          Cicero.reset();
          
          ciceroStatus = CICERO_STATUS_IDLE;
          input = "";
          driverStatus = DRIVER_STATUS_WAIT_TEXT;
        }
      }
      break;
  }
}

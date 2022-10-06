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
long charsToRead;

void setup() {
  // Reset global variables
  ciceroStatus = CICERO_STATUS_IDLE;
  driverStatus = DRIVER_STATUS_WAIT_CMD;
  input = "";
  charsToRead = -1;
  
  // Upload the bitstream of CICERO to the FPGA
  Cicero.begin();

  // Initialize Serial
  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  /*
    The program can be in one of four states:
    1) COMMAND MODE: wait for one of the following commands: enter regex editing mode, enter text editing mode
    2) REGEX EDITING MODE: wait for the number of bytes to read followed by DRIVER_INPUT_TERMINATOR, then read the bytecode of a new regex, load it into CICERO RAM and return to command mode
    3) TEXT EDITING MODE: wait for the number of bytes to read followed by DRIVER_INPUT_TERMINATOR, then read a new string to examine, load the string into CICERO RAM and go to executing mode; when reading DRIVER_INPUT_TERMINATOR return to command mode
    4) EXECUTING MODE: CICERO is executing, check its status to detect if a match has been found or not, send the result through serial and then reset and return to text editing mode
   */   
  switch (driverStatus) {
    case DRIVER_STATUS_WAIT_CMD:
      if(Serial.available() <= 0) return;
      inChar = Serial.read();

      input = "";
      charsToRead = -1;
      
      switch (inChar) {
        case DRIVER_CMD_REGEX:
          driverStatus = DRIVER_STATUS_WAIT_REGEX;
          break;
        case DRIVER_CMD_TEXT:
          driverStatus = DRIVER_STATUS_WAIT_TEXT;
          break;
      }
      break;
    case DRIVER_STATUS_WAIT_REGEX:
      if (charsToRead == -1) {
        while (Serial.available() > 0) {
          inChar = Serial.read();

          if (inChar == DRIVER_INPUT_TERMINATOR) {
            char arr[input.length() + 1];
            input.toCharArray(arr, input.length() + 1);
            charsToRead = atoi(arr);
            input = "";
            break;
          } else {
            input += inChar;
          }
        }
      } else if (charsToRead > 0) {
        while (Serial.available() > 0 && charsToRead > 0) {
          inChar = Serial.read();
          charsToRead--;
          
          input += inChar;
        }
      } else {
        Cicero.loadCode(input);
        driverStatus = DRIVER_STATUS_WAIT_CMD;
        break;
      }
      break;
    case DRIVER_STATUS_WAIT_TEXT:
      if (charsToRead == -1) {
        while (Serial.available() > 0) {
          inChar = Serial.read();
  
          if (inChar == DRIVER_INPUT_TERMINATOR) {
            char arr[input.length() + 1];
            input.toCharArray(arr, input.length() + 1);
            charsToRead = atoi(arr);
            input = "";
            break;
          } else {
            input += inChar;
          }
        }
      } else if (charsToRead > 0) {
        while (Serial.available() > 0 && charsToRead > 0) {
          inChar = Serial.read();
          charsToRead--;
          
          input += inChar;
        }
      } else if (charsToRead < -1) {
        // Negative length means to return to command mode
        driverStatus = DRIVER_STATUS_WAIT_CMD;
      } else {
        // Add the string terminator (needed by Cicero)
        input += '\0';
        // Load string to examine to CICERO RAM and begin computation
        Cicero.loadStringAndStart(input);

        printRAMContents(10);
        
        driverStatus = DRIVER_STATUS_EXECUTING;
        break;
      }
      break;
    case DRIVER_STATUS_EXECUTING:
      uint32_t newStatus = Cicero.getStatus();
      // Run the checks only if the status has changed from the previous iteration of the loop()
      if (newStatus != ciceroStatus) {
        ciceroStatus = newStatus;
  
        bool restart = false;
        switch (ciceroStatus) {
          case CICERO_STATUS_ACCEPTED:
          case CICERO_STATUS_REJECTED:
          case CICERO_STATUS_ERROR:
            uint32_t elapsedCC = Cicero.getElapsedClockCycles();
            Serial.print(ciceroStatus);
            Serial.print(elapsedCC);
            Serial.write(DRIVER_INPUT_TERMINATOR);
            restart = true;
            break;
        }
  
        if (restart) {
          Cicero.reset();
          
          ciceroStatus = CICERO_STATUS_IDLE;
          input = "";
          charsToRead = -1;
          driverStatus = DRIVER_STATUS_WAIT_TEXT;
        }
      }
      break;
  }
}

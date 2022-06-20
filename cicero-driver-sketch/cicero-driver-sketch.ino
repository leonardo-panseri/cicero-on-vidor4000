#include <wiring_private.h>
#include "JTAG.h"

extern void enableFpgaClock(void);

/*
 * FPGA related data and setup functions
 */

// FPGA Pins
#define TDI            12
#define TDO            15
#define TCK            13
#define TMS            14
#define MB_INT         28
#define MB_INT_PIN     31
#define SIGNAL_OUT     41 //B5 L16
#define SIGNAL_IN      33 //B2 N2

#define no_data   0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
                  0xFF, 0xFF, 0xFF, 0xFF, \
                  0x00, 0x00, 0x00, 0x00  \

#define NO_BOOTLOADER   no_data
#define NO_APP          no_data
#define NO_USER_DATA    no_data

// Bitstream and signature
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
__attribute__ ((used, section(".fpga_bitstream")))
const unsigned char bitstream[] = {
  #include "app.h"
};

// Load the bitstream on the FPGA
static void  setupFPGA() {
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

/*
 * Defines for Cicero and Cicero interface constants
 */

// Virtual Instruction Register values
#define VIR_STATUS            0x01
#define VIR_COMMAND           0x02
#define VIR_ADDRESS           0x03
#define VIR_START_CC_POINTER  0x04
#define VIR_END_CC_POINTER    0x05
#define VIR_DATA_IN           0x06
#define VIR_DATA_OUT          0x07
#define VIR_DEBUG             0x0F

// Cicero commands
#define  CMD_NOP                   0x0
#define  CMD_WRITE                 0x1
#define  CMD_READ                  0x2
#define  CMD_START                 0x3
#define  CMD_RESET                 0x4
#define  CMD_READ_ELAPSED_CLOCK    0x5
#define  CMD_RESTART               0x6

// Cicero status
#define  STATUS_IDLE               0x0
#define  STATUS_RUNNING            0x1
#define  STATUS_ACCEPTED           0x2
#define  STATUS_REJECTED           0x3
#define  STATUS_ERROR              0x4

/*
 Machine code for Cicero
 */

uint8_t code[] = {
  #include "code.h"
};

/*
 * Driver for the Virtual JTAG communication interface
 */

// Reads the 32 bit data register associated with the given virtual instruction register value through the Virtual JTAG
uint32_t readRegister32(uint8_t VIR) {
  uint8_t data[4];
  JTAG_Read_VDR_from_VIR(VIR, data, 32);
  return ((uint32_t) data[0]) | (((uint32_t) data[1]) << 8) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[3]) << 24);
}

// Writes the 32 bit data register associated with the given virtual instruction register value through the Virtual JTAG
void writeRegister32(uint8_t VIR, uint32_t data) {
  uint8_t bytes[4];
  bytes[0] = data;
  bytes[1] = data >> 8;
  bytes[2] = data >> 16;
  bytes[3] = data >> 24;
  JTAG_Write_VDR_to_VIR(VIR, bytes, 32);
}

/*
 * Cicero drivers
 */

// Writes the given bytes to the RAM of Cicero, starting from the given address
// NB: the RAM is addressed by dwords (address 0 is the first dword, address 1 is the second dword, ...)
// Returns the next free address in the RAM
uint32_t writeBytesToRAM(int numbytes, uint8_t *bytes, uint32_t startingAddr) {
  uint32_t byteAddr = startingAddr;
  uint8_t dword[4];
  short first = 1;
  int bytesWritten = 1;
  for (int i = 0; i < numbytes; i = i + 4) {
    dword[3] = bytes[i];
    bytesWritten = 1;
    if (i + 1 < numbytes) {
      dword[2] = bytes[i + 1];
      bytesWritten = 2;
    } else dword[2] = 0;
    if (i + 2 < numbytes) {
      dword[1] = bytes[i + 2];
      bytesWritten = 3;
    } else dword[1] = 0;
    if (i + 3 < numbytes) {
      dword[0] = bytes[i + 3];
      bytesWritten = 4;
    } else dword[0] = 0;

    writeRegister32(VIR_ADDRESS, byteAddr / 4);
    JTAG_Write_VDR_to_VIR(VIR_DATA_IN, dword, 32);

    if (first) {
      first = 0;
      writeRegister32(VIR_COMMAND, CMD_WRITE);
    }

    byteAddr = byteAddr + bytesWritten;
  }

  writeRegister32(VIR_COMMAND, CMD_NOP);
  return byteAddr;
}

// Reads the given number of dwords (4 bytes) into the res array, starting from the given address
// NB: the RAM is addressed by dwords (address 0 is the first dword, address 1 is the second dword, ...)
void readDWordsFromRAM(int dwordsToRead, uint32_t *res, uint32_t starting_addr) {
  uint32_t addr = starting_addr;
  short first = 1;
  for(int i = 0; i < dwordsToRead; i++) {
    writeRegister32(VIR_ADDRESS, addr);

    if (first) {
      first = 0;
      writeRegister32(VIR_COMMAND, CMD_READ);
    }

    res[i] = readRegister32(VIR_DATA_OUT);

    addr = addr + 1;
  }

  writeRegister32(VIR_COMMAND, CMD_NOP);
}

// Loads machine code into Cicero RAM and return the next free RAM address
uint32_t loadCode() {
  // The length of the code in bytes is encoded in the first two bytes of the file
  uint16_t codeNumBytes = ((uint16_t) code[1]) | (((uint16_t) code[0]) << 8);
  return writeBytesToRAM(codeNumBytes, &code[2], 0);
}

// Converts a String into an array of bytes
void convertStringToBytes(String string, uint8_t *res) {
  for (int i = 0; i < string.length(); i++) {
    res[i] = (uint8_t) string[i];
  }
}

// Writes the given string into RAM and starts the execution of Cicero
void loadStringAndStart(String str, uint32_t codeEndAddr) {
  uint8_t strBytes[str.length()];
  convertStringToBytes(str, strBytes);

  uint32_t strStartAddr = ceil(codeEndAddr / 4.0) * 4;
  uint32_t strEndAddr = writeBytesToRAM(str.length(), strBytes, strStartAddr);

  writeRegister32(VIR_START_CC_POINTER, strStartAddr);
  writeRegister32(VIR_END_CC_POINTER, strEndAddr);

  writeRegister32(VIR_COMMAND, CMD_START);
  writeRegister32(VIR_COMMAND, CMD_NOP);
}

/*
 * Test functions
 */
void printRAMContents(int dwordsToRead) {
  uint32_t dwordsRead[dwordsToRead];
  readDWordsFromRAM(dwordsToRead, dwordsRead, 0);
  Serial.println("RAM contents:");
  for(int i = 0; i < dwordsToRead; i++) {
    Serial.println(dwordsRead[i], HEX);
  }
}

// Global variables
uint32_t codeEndAddr;
uint32_t status;

String input;
bool waiting;
bool ciceroExecuting;

// Runs once on reset or power to the board
void setup() {
  setupFPGA();

  // Configure Pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(6, INPUT);

  // Initialize variables
  status = 0;
  input = "";
  waiting = false;
  ciceroExecuting = false;

  // Initialize Serial
  Serial.begin(9600);
  while (!Serial);
  
  //val = digitalRead(6);
  //Serial.println(val);
  
  //uint32_t a = 0x87654321;
  //writeRegister32(VIR_DATA_IN, a);
  //val = digitalRead(6);
  //Serial.println(val);
  
  //uint32_t res = readRegister32(VIR_DEBUG);
  //Serial.println(res, HEX);

  codeEndAddr = loadCode();
}

// Runs over and over again forever
void loop() {
  if (!ciceroExecuting) {
    if (!waiting) {
      Serial.println("Input a string to examine:");
      waiting = true;
    } else {
      while (Serial.available() > 0) {
        char inChar = Serial.read();
        if (inChar == '\n') {
          Serial.println("Starting Cicero execution");

          waiting = false;
          ciceroExecuting = true;
          loadStringAndStart(input, codeEndAddr);

          printRAMContents(10);
        } else {
          input += inChar;
        }
      }
    }
  } else {
    delay(1000);
    uint32_t newStatus = readRegister32(VIR_STATUS);
    if (newStatus != status) {
      status = newStatus;
      Serial.println(status);
    }
  }
}

#include <wiring_private.h>
#include "JTAG.h"

extern void enableFpgaClock(void);

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

uint32_t readRegister32(uint8_t VIR) {
  uint8_t data[4];
  JTAG_Read_VDR_from_VIR(VIR, data, 32);
  return ((uint32_t) data[0]) | (((uint32_t) data[1]) << 8) | (((uint32_t) data[2]) << 16) | (((uint32_t) data[3]) << 24);
}

void writeRegister32(uint8_t VIR, uint32_t data) {
  uint8_t bytes[4];
  bytes[0] = data;
  bytes[1] = data >> 8;
  bytes[2] = data >> 16;
  bytes[3] = data >> 24;
  JTAG_Write_VDR_to_VIR(VIR, bytes, 32);
}

void writeBytesToRAM(int numbytes, uint8_t *bytes, uint32_t starting_addr) {
  uint32_t addr = starting_addr;
  uint8_t dword[4];
  short first = 1;
  int bytes_written = 1;
  for (int i = 0; i < numbytes; i = i + 4) {
    dword[3] = bytes[i];
    bytes_written = 1;
    if (i + 1 < numbytes) {
      dword[2] = bytes[i + 1];
      bytes_written = 2;
    } else dword[2] = 0;
    if (i + 2 < numbytes) {
      dword[1] = bytes[i + 2];
      bytes_written = 3;
    } else dword[1] = 0;
    if (i + 3 < numbytes) {
      dword[0] = bytes[i + 3];
      bytes_written = 4;
    } else dword[0] = 0;

    writeRegister32(VIR_ADDRESS, addr);
    delay(100);
    JTAG_Write_VDR_to_VIR(VIR_DATA_IN, dword, 32);

    delay(1000);

    if (first) {
      first = 0;
      writeRegister32(VIR_COMMAND, CMD_WRITE);
      delay(500);
    }

    addr = addr + bytes_written;
  }

  writeRegister32(VIR_COMMAND, CMD_NOP);
}

void readDWordsFromRAM(uint32_t starting_addr, int dwordsToRead, uint32_t *res) {
  uint32_t addr = starting_addr;
  short first = 1;
  for(int i = 0; i < dwordsToRead; i++) {
    writeRegister32(VIR_ADDRESS, addr);
    delay(100);

    if (first) {
      first = 0;
      writeRegister32(VIR_COMMAND, CMD_READ);
      delay(500);
    }

    res[i] = readRegister32(VIR_DATA_OUT);

    delay(1000);

    addr = addr + 4;
  }

  writeRegister32(VIR_COMMAND, CMD_NOP);
}

uint8_t code[] = {
  #include "code.h"
};

// Global variables
int val;

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

  // The length of the code in bytes is encoded in the first two bytes of the file
  uint16_t codeNumBytes = ((uint16_t) code[1]) | (((uint16_t) code[0]) << 8);
  writeBytesToRAM(codeNumBytes, &code[2], 0);

  int dwordsToRead = ceil(codeNumBytes / 4);
  uint32_t bytesRead[dwordsToRead];
  readDWordsFromRAM(0, dwordsToRead, bytesRead);
  Serial.println("Results");
  for(int i = 0; i < dwordsToRead; i++) {
    Serial.println(bytesRead[i], HEX);
  }
}

// the loop function runs over and over again forever
void loop() {
  
}

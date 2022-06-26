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
  #include "app.h"
};

// Global variables

// The address of the qword where input strings can be loaded
uint32_t strStartAddr;
// The current status of Cicero
uint32_t status;

// The string in input that will need to be examined by Cicero
String input;
// Flag that indicates if we are waiting for user input through serial
bool waiting;
// Flag that indicates if Cicero is currently examining a string
bool ciceroExecuting;
// variable that keeps track when execution starts
unsigned long execStartingTime;
// variable that keeps track when execution ends
unsigned long execEndingTime;

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
 * Cicero and Cicero interface constants
 */

 #define RAM_MAX_ADDRESS 511

// Virtual Data Registers addresses
// These are the addresses to write in the VIR to select the corresponding data register
// 32 bit registers
#define VIR_STATUS            0x01
#define VIR_COMMAND           0x02
#define VIR_ADDRESS           0x03
#define VIR_START_CC_POINTER  0x04
#define VIR_END_CC_POINTER    0x05
#define VIR_DEBUG             0x0F

// 64 bit registers
#define VIR_DATA_IN           0x06
#define VIR_DATA_OUT          0x07

// Cicero commands
#define  CMD_NOP                   0x0
#define  CMD_WRITE                 0x1
#define  CMD_READ                  0x2
#define  CMD_START                 0x3
#define  CMD_RESET                 0x4
#define  CMD_READ_ELAPSED_CLOCK    0x5
#define  CMD_RESTART               0x6

// Cicero statuses
#define  STATUS_IDLE               0x0
#define  STATUS_RUNNING            0x1
#define  STATUS_ACCEPTED           0x2
#define  STATUS_REJECTED           0x3
#define  STATUS_ERROR              0x4

/*
 Machine code for Cicero
 Since it is included in the program, a change to the machine code of Cicero always requires to recompile and reupload the sketch
 */

uint8_t code[] = {
  #include "code.h"
};

/*
 * Drivers for the Virtual JTAG communication interface
 */

// Reads the 32 bit data register associated with the given virtual instruction register value through the Virtual JTAG
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

// Writes the 32 bit data register associated with the given virtual instruction register value through the Virtual JTAG
void writeRegister32(uint8_t VIR, uint32_t data) {
  uint8_t bytes[4];

  // Since the library expects the data as a byte array, we need to convert our input data
  // NB: the bytes in the array are expected to be little-endian: the first is the least significant and the last is the most significant
  for (int i = 0; i < 4; i++) {
    bytes[i] = data >> i * 8;
  }

  JTAG_Write_VDR_to_VIR(VIR, bytes, 32);
}

// Reads the 64 bit data register associated with the given virtual instruction register value through the Virtual JTAG
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

// Writes the 64 bit data register associated with the given virtual instruction register value through the Virtual JTAG
void writeRegister64(uint8_t VIR, uint64_t data) {
  uint8_t bytes[8];

  // Since the library expects the data as a byte array, we need to convert our input data
  // NB: the bytes in the array are expected to be little-endian: the first is the least significant and the last is the most significant
  for (int i = 0; i < 8; i++) {
    bytes[i] = data >> i * 8;
  }
  
  JTAG_Write_VDR_to_VIR(VIR, bytes, 64);
}

/*
 * Drivers for Cicero
 */

// Writes the given qwords (8 bytes) to the RAM of Cicero, starting from the given address
// NB: the RAM is addressed by qwords (address 0 is the first qword, address 1 is the second qword, ...)
void writeQWordsToRAM(int qwordsToWrite, uint64_t *qwords, uint32_t startingAddr) {
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
    Serial.print("ERROR: Trying to write to RAM address ");
    Serial.print(addr);
    Serial.print(", but it is out of bounds");
    Serial.println("");
  }

  // When the write is finished, we need to send the NOP command to Cicero, to put it back into the idle state
  writeRegister32(VIR_COMMAND, CMD_NOP);
}

// Reads the given number of qwords (8 bytes) into the res array, starting from the given address
// NB: the RAM is addressed by qwords (address 0 is the first qword, address 1 is the second qword, ...)
void readQWordsFromRAM(int qwordsToRead, uint64_t *res, uint32_t startingAddr) {
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

// Loads machine code into Cicero RAM and return the next free RAM address
uint32_t loadCode() {
  // The length of the code in bytes is encoded in the first two bytes of the file
  uint16_t codeNumBytes = ((uint16_t) code[1]) | (((uint16_t) code[0]) << 8);

  int qwordsToWrite = ceil(codeNumBytes / 8.0);
  uint64_t qwords[qwordsToWrite];

  uint64_t qword = 0;
  int index = 0;
  int bytesWritten = 0;
  for (int i = 2; i < codeNumBytes + 2 - 1; i = i + 2) {
    bytesWritten = bytesWritten + 2;

    // The code needs to be written in RAM as little-endian, but the endianess is refering to words (2 bytes) and not bytes
    // So we need to write the first word in the least significant 2 bytes and the last word in the most significant 2 bytes
    qword = qword | ((uint64_t) code[i]) << 8 * (bytesWritten - 1) | ((uint64_t) code[i + 1]) << 8 * (bytesWritten - 2);

    if (bytesWritten == 8 || i == codeNumBytes) {
      // If we have composed a full qword or if we are at the last byte of code, save the qword to the array
      qwords[index] = qword;
      index = index + 1;
            
      qword = 0;
      bytesWritten = 0;
    }
  }

  writeQWordsToRAM(qwordsToWrite, qwords, 0);

  // Since the RAM is addressed by qwords starting from address 0, the next free RAM address is equals to the number of qwords written
  return qwordsToWrite;
}

// Converts a String into an array of bytes
void convertStringToBytes(String string, uint8_t *res) {
  for (int i = 0; i < string.length(); i++) {
    res[i] = (uint8_t) string[i];
  }
}

// Writes the given string into RAM and starts the execution of Cicero
void loadStringAndStart(String str, uint32_t strStartAddr) {
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
    
    for (int j = 0; j < 7; j++) {
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

// Resets Cicero
void reset() {
  writeRegister32(VIR_COMMAND, CMD_RESET);
  writeRegister32(VIR_COMMAND, CMD_NOP);
}

/*
 * Test functions
 */

// Reads the given number of qwords from RAM, starting from address 0, and prints them to the serial as hex numbers
void printRAMContents(int qwordsToRead) {
  uint64_t qwordsRead[qwordsToRead];
  readQWordsFromRAM(qwordsToRead, qwordsRead, 0);
  Serial.println("RAM contents:");
  for(int i = 0; i < qwordsToRead; i++) {
    // Note that leading zeros will not be printed
    Serial.println(qwordsRead[i], HEX);
  }
}



// Runs once on reset or power to the board
void setup() {
  setupFPGA();

  // Configure Pins
  // pinMode(LED_BUILTIN, OUTPUT);
  // for (int i = 0; i < 8; i++) {
  //   pinMode(i, INPUT);
  // }

  // Initialize variables
  status = 0;
  input = "";
  waiting = false;
  ciceroExecuting = false;

  // Initialize Serial
  Serial.begin(9600);
  while (!Serial);

  // Load Cicero machine code
  strStartAddr = loadCode();

  // waiting = false;
  // ciceroExecuting = true;
  // String benchmark = "LHKRDTDWVENNPLKTPAQVEMYKFLLRISRLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYFCSLDSNDWVENNPLKTPAQVEMYKFLLRISRLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETESLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYFCSLDSNDWVENNPLKTPAQVEMYKFLLRISRLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYEEPSVAARILEKGKLTITNLMKSLGFKPKLHKSDDYFCSLDSNDWVENNPLKTPAQVEMYKFLLRISRLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETTEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKPKKIQSIDRYFCSLDSNYNSEDEDFEYDSDSEDDLHKRDTDWVENNPLKTPAQVEMYKFLLRISRLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTITNLMKSLGFKPKLHKRDTDWVENNPLKTPAQVEMYKFLLRISQLNRDGTGYESDSDPENEHFDDESFSSGEEDSSDEDDPTWAPDSDDSDWETETEEEPSVAARILEKGKLTX8e[F=XXX]mGAXXlL]CXXITNLMKSLGFKPKPKKIQSIDRYFCSLDSNYNSEDEDFEYDSDSEDDDSDRSERDDC";
  // loadStringAndStart(benchmark + '\0', strStartAddr);
}

// Runs over and over again forever
void loop() {
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

          status = readRegister32(VIR_STATUS);
          if (status != STATUS_IDLE) {
            Serial.print("Cicero is in an unexpected state: ");
            Serial.println(status);
          }

          waiting = false;
          ciceroExecuting = true;
          loadStringAndStart(input, strStartAddr);

          // printRAMContents(10);
        } else {
          input += inChar;
        }
      }
    }
  } else {
    uint32_t newStatus = readRegister32(VIR_STATUS);
    if (newStatus != status) {
      status = newStatus;

      bool restart = false;
      switch (status) {
        case STATUS_RUNNING:
          Serial.println("Cicero is running");
          break;
        case STATUS_ACCEPTED:
          Serial.println("A match has been found");
          restart = true;
         break;
        case STATUS_REJECTED:
          Serial.println("A match has not been found");
          restart = true;
          break;
        case STATUS_ERROR:
          Serial.println("Cicero has encountered an error");
          restart = true;
          break;
        default:
          Serial.print("Unknown Cicero state: ");
          Serial.println(status);
          break;
      }

      if (restart) {
        execEndingTime = micros();
        Serial.print("Execution time : ");
        Serial.print((execEndingTime - execStartingTime));
        Serial.println(" microseconds");
        
        Serial.println("");
        reset();
        status = 0;
        input = "";
        ciceroExecuting = false;
      }
    }
  }
}

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

uint32_t readRegister32(uint8_t VIR) {
  uint8_t data[4];
  JTAG_Read_VDR_from_VIR(VIR, data, 32);
  uint32_t result = 0;
  for (int i = 3; i >= 0; i++) {
    result = result | (data[i] << ((3 - i) * 8));
  }
  return result;
}

// Global variables
int val;

// Runs once on reset or power to the board
void setup() {
  setupFPGA();

  // Configure onboard LED Pin as output
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  pinMode(6, INPUT);
  val = digitalRead(6);
  delay(1000);
  while (!Serial);
  Serial.println(val);
  
  uint8_t data[4] = {0x23, 0x01, 0x00, 0x00};
  JTAG_Write_VDR_to_VIR(0x06, data, 32);

  val = digitalRead(6);
  Serial.println(val);

  JTAG_Read_VDR_from_VIR(0x0F, data, 32);
  int i;
  for (i = 0; i < 4; i++) {
    Serial.println(data[i]);
  }
  
  uint32_t res = readRegister32(VIR_DEBUG);
  Serial.println(res, DEC);
}

// the loop function runs over and over again forever
void loop() {
  
}

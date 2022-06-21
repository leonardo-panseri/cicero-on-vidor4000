# Cicero On Arduino MKR Vidor 4000
Adaptation of [Cicero](https://github.com/necst/cicero), a FPGA regex coprocessor, for the Intel Cyclone 10 LP embedded in the [Arduino MKR Vidor 4000](https://store.arduino.cc/products/arduino-mkr-vidor-4000).  
The AXI communication interface has been replaced with a Intel Virtual JTAG interface.

## Directory structure
The directory structure is summarized in the following table:

Directory                 | Contents
----------                | --------
cicero-compiler           | Python program that is responsible of producing the machine code for Cicero, starting from a regular expression
cicero-driver-sketch      | Arduino sketch that loads the bitstream and drives Cicero on the FPGA
constraints               | Constraint files for the FPGA, includes pinout and timings
ip                        | Source code for IP blocks provided by Arduino
projects                  | Quartus Prime project files for the FPGA
vidor-bitstream-converter | C program (written by [wd5gnr](https://github.com/wd5gnr/VidorFPGA)) that converts Quartus FPGA bitstreams into the format required by the Arduino

## About the Quartus Prime project
We used Quartus Prime 21.1 Lite, which can be downloaded from Altera/Intel web site.  

### Project template
We used the example project that can be found in `projects/example_counter` (thanks to [wd5gnr](https://github.com/wd5gnr/VidorFPGA)) as a template.  
It already has smart-compile turned off to avoid problems and it loads automatically the constraints files present in `constraints/`. The top module is [MKRVIDOR4000_top](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/example_counter/MKRVIDOR4000_top.v), it should be left unchanged and all user code should go in [user.v](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/example_counter/user.v), which gets included in the top module. Quartus throws a lot of warnings when compiling the project, but based on our testing they can be safely ignored.

### `user.v`
We imported Cicero HDL files and modified [user.v](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/user.v) as follows:
-  to have a cleaner design we refactored it to be a module instead of an include file, whit only the reset signal and the PLL clock as inputs
-  we declared some 32 bits and 64 bits registers to hold the data needed to configure and control Cicero
-  we istantiated the [AXI_top module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/AXI/AXI_top.sv) of Cicero. The name is misleading, as it does not contain any AXI protocol logic, it is just the main interface of the design
-  we istantiated the [Virtual JTAG communication interface](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/Virtual_JTAG_Adapter.v), more about this in the section below
-  we linked the two modules using the registers mentioned above
-  we created a new reset signal that is active high, as Cicero needs it and the reset coming from the Arduino is active low

### Virtual JTAG Adapter
We wrote a [communication module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/Virtual_JTAG_Adapter.v) using the [Intel Virtual JTAG IP](https://www.intel.com/content/www/us/en/docs/programmable/683705/20-3/virtual-jtag-core-user-guide.html). This module is needed to read and write the control register for Cicero from the SAMD21 CPU that is present on the Arduino.  
The usage of this interface is simple: following the protocol described in the documentation of the IP, one writes the address of the desired virtual data register to the virtual instruction register through the JTAG TDI port, then writes the data in the register through the same port or reads the content of the register through the JTAG TDO port.  
JTAG is a serial protocol and it is not the most efficient way of communicating, as the data needs to be shifted in and out bit by bit, but it is easy to use both on the FPGA side, thanks to the Virtual JTAG IP, and on the Arduino side, thanks to the Vidor libraries (more on these below).

### Design incompatibilities
Cicero has been designed for a Xilinx Ultra FPGA and the chip present on the Arduino is an Intel Cyclone 10 LP, so we had to make some adjustements:
- there was some SystemVerilog syntax in Cicero HDL files that is incompatible with Quartus SystemVerilog standards, such as for loops outside of a generate block, Xilinx specific syntax attributes and signal declaration and initialization on the same line ([document with a list of common incompatibilities when porting SystemVerilog from Xilinx to Intel](https://marekpikula.github.io/quartus-sv-gotchas/Intel%20Quartus%20SystemVerilog%20gotchas.html))
- the syntax of [the RAM used by Cicero](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/memories/bram.sv), was not synthesizable by Quartus, so we had to rewrite it following Intel standards. We chose to implement a dual-port RAM with read and write enables and with the same width for both ports
- the I/O signals in the [AXI_top module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/AXI/AXI_top.sv) where limited in width by the AXI protocol, causing the need to have a dual-port RAM with ports of different width (Cicero expects the read port to be 64 bits). Since the JTAG protocol does not impose any restrictions on register size, we modified the signals in AXI_top responsible of the communication with RAM to make them both 64 bits wide and we refactored some logic regarding these signals to ensure compatibility with our rewritten RAM


At last, we compiled the design and obtained the `MKRVIDOR4000.ttf` bitstream file.  
In order to load the FPGA bitstream in the Arduino sketch, the `.ttf` file needs to be converted with the executable found in `vidor-bitstream-converter/`.
The resulting `app.h` should be copied to the sketch directory and it will be uploaded to the FPGA when the board is powered and on reset.

## About vidor-bitstream-converter
Arduino provides a Java program to do the same task, but it only works with Java 11. Since it is very simple (it just
flips bit order) [wd5gnr](https://github.com/wd5gnr/VidorFPGA) rewrote it in C.  
The executable provided is compiled for Windows x86, but it can be compiled on any system with: `gcc -o vidorcvt vidorcvt.c`  
There are no arguments so it is needed to redirect: `vidorcvt <binaryfile.ttf >app.h`  
A Windows batch file that automatically convert the bitstream in `projects/cicero-cyclone/output_files/` and writes the result to `cicero-driver-sketch/app.h` is provided.

## About cicero-compiler
We wrote [an utility](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-compiler/compile_for_arduino.py) that calls the Cicero compiler and writes the resulting machine code to a `code.h` file so that it is easy to import it into the Arduino sketch. For more information on how the Cicero compiler works, refer to [the official repo](https://github.com/necst/cicero_compiler).

## About cicero-driver-sketch
We imported the [JTAG](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/JTAG.h) and [jtag_host](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/jtag_host.h) libraries, part of the [official VidorBoot/VidorPeripherals/VidorGraphics library](https://github.com/vidor-libraries). These libraries implement the JTAG protocol: they allows us to load the bitstream on the FPGA and to communicate using the Virtual JTAG IP.  
The `app.h` file contains the bitstream to be loaded on the FPGA and the `code.h` file contains the machine code for Cicero. They contain a list of integers, each representing one byte, so that they can be easily imported into the sketch through an `#include`.  
The [main sketch file](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/cicero-driver-sketch.ino) imports the libraries and the binary files and implements the functions to load the bitstream and read/write from registers on the FPGA. It also implements the logic needed to drive Cicero:
- it loads the code into Cicero memory during the `setup()` function
- it waits for user input through the USB serial port
- when a string is provided in input (terminated by a `\n`), it loads the string into Cicero memory and starts the execution of Cicero
- when Cicero is done it prints if a match was found, resets Cicero and starts waiting for another string

# Cicero On Arduino MKR Vidor 4000
Adaptation of [Cicero](https://github.com/necst/cicero), a FPGA regex coprocessor, for the Intel Cyclone 10 LP embedded in the [Arduino MKR Vidor 4000](https://store.arduino.cc/products/arduino-mkr-vidor-4000).  
The AXI communication interface has been replaced with a Intel Virtual JTAG interface.

## Directory structure
The directory structure is summarized in the following table:

Directory                 | Contents
----------                | --------
cicero-compiler           | Python program that is responsible of producing the machine code for Cicero starting from a regular expression
cicero-driver-sketch      | Arduino sketch that loads the bitstream and drives Cicero on the FPGA
constraints               | Constraint files for the FPGA, includes pinout and timings
ip                        | Source code for IP blocks provided by Arduino
projects                  | Quartus Prime project files for the FPGA
vidor-bitstream-converter | C program (written by [wd5gnr](https://github.com/wd5gnr/VidorFPGA)) that converts Quartus FPGA bitstreams into the format required by the Arduino

## About the Quartus Prime project
We used Quartus Prime 21.1 Lite, which can be downloaded from Altera/Intel web site.  
- We used the example project that can be found in `projects/example_counter` (thanks to [wd5gnr](https://github.com/wd5gnr/VidorFPGA)) as a template. 
It already has smart-compile turned off to avoid problems and it loads automatically the constraints files present in `constraints/`.
The top module is [MKRVIDOR4000_top](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/example_counter/MKRVIDOR4000_top.v), it should be left untouched, all user code should go in [user.v](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/example_counter/user.v), which gets included in the top module.
- We wrote a [communication module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/Virtual_JTAG_Adapter.v) using the [Intel Virtual JTAG IP](https://www.intel.com/content/www/us/en/docs/programmable/683705/20-3/virtual-jtag-core-user-guide.html). This module is needed to read and write the control register for Cicero that are defined in [user.v](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/user.v) from the SAMD21 CPU that is present on the Arduino.
- We imported Cicero HDL files and modified [user.v](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/user.v) to istantiate the [AXI_top module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/AXI/AXI_top.sv) of Cicero and the [Virtual JTAG communication interface](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/Virtual_JTAG_Adapter.v).
- We fixed incompatible SystemVerilog syntax of Cicero, as it has been written for a Xilinx Ultra FPGA and it contained HDL code that is not valid for Quartus Prime ([document with a list of common incompatibilities when porting from Xilinx HDL to Intel HDL](https://marekpikula.github.io/quartus-sv-gotchas/Intel%20Quartus%20SystemVerilog%20gotchas.html)).
- We rewrote [the RAM used by Cicero](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/memories/bram.sv), as the previous syntax was not synthesizable on the Intel Cyclone 10 LP.
- We adapted the [AXI_top module](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/projects/cicero-cyclone/cicero-rtl/AXI/AXI_top.sv) to be compatible with the new RAM and the Intel Virtual JTAG interface.
- We compiled the design and obtained the `MKRVIDOR4000.ttf` bitstream file.

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
We imported the [JTAG](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/JTAG.h) and [jtag_host](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/jtag_host.h) libraries, part of the [official VidorBoot/VidorPeripherals/VidorGraphics library](https://github.com/vidor-libraries). These libraries implement the JTAG protocol: they allows us to load the bitstream to the FPGA and communicate using the Virtual JTAG IP.  
The `app.h` file contains the bitstream to be loaded on the FPGA and the `code.h` file contains the machine code for Cicero. They contain a list of integers, each representing one byte, so that they can be easily imported into the sketch through an `#include`.  
The [main sketch file](https://github.com/leonardo-panseri/cicero-on-vidor4000/blob/master/cicero-driver-sketch/cicero-driver-sketch.ino) imports the libraries and the binary files and implements the functions to load the bitstream and read/write from registers on the FPGA.

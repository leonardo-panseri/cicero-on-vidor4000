# Cicero On Arduino MKR Vidor 4000
Adaptation of [Cicero](https://github.com/necst/cicero) for the [Arduino MKR Vidor 4000](https://store.arduino.cc/products/arduino-mkr-vidor-4000).

## Directory structure
directory structure is summarized in the following table:

Directory  | Contents
---------- | --------
cicero-driver-sketch | Arduino sketch that loads the bitstream and drives Cicero on the FPGA
constraints | constraint file for the FPGA, includes pinout and timings
ip | source code for IP blocks provided by Arduino
projects | project files for the FPGA, the project is an adaptation of Cicero for the Cyclone 10 LP that has the AXI communication interface replaced with a Intel Virtual JTAG interface
vidor-bitstream-converter | a C program (written by [wd5gnr](https://github.com/wd5gnr/VidorFPGA)) that converts Quartus FPGA bitstreams into the format required by the Arduino

## Build
We used Quartus Prime 21.1 Lite, which can be downloaded from Altera/Intel web site, to compile the project.
Quartus will produce a set of files under the output_files directory in the project folder. 
In order to incorporate the FPGA bitstream in the Arduino code, the `.ttf` file needs to be converted with the executable found in `vidor-bitstream-converter`/.
The resulting `app.h` should be copied to the sketch directory and it will be uploaded to the FPGA when the board is powered and on reset.

## About vidor-bitstream-converter
Arduino provides a Java program to do the same task, but it only works with Java 11. Since it is very simple (it just
flips bit order) [wd5gnr](https://github.com/wd5gnr/VidorFPGA) rewrote it in C.  
The executable provided is compiled for Windo6 x86, but it can be compiled on any system with:
    gcc -o vidorcvt vidorcvt.c

There are no arguments so it is needed to redirect:
    vidorcvt <binaryfile.ttf >app.h

# How to program the Arduino MKR Vidor 4000 with our library


To program the Arduino MKR Vidor 4000 and use our library go through the following steps

## Install the required boards
In the Arduino IDE
```
Tools-->Boards-->Boards Manager;
```

Search for Vidor 4000, install it, and also install the AVR Boards

## Include the CICERO library

Open the Arduino IDE, press 
```
Sketch-->include library-->add .zip library 
```
and add the zip folder


## Programming the board

Double-check for the proper COM/serial boards
```
Tools-->Port-->(COMxx: Arduino MKR Vidor 4000)
```
where xx is the proper COM number.


Either open or copy paste the sketch
```
<root_fldr>/cicero-arduino-drivers/CiceroSerial.ino 
```
Then push the "Verify" button and then "Upload"

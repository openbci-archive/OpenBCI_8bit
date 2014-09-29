Firmware for OpenBCI 8bit system
=================

The OpenBCI 8bit system is an open-source biopotential acquisisiton system. It incorporates an ADS1299 (analog to digital converter IC), LIS3DH (tripple axis accelerometer), Micro SD card, RFduino radio module, and an ATmega328 with Arduino UNO bootloader. The Device code is for the OpenBCI Board Radio Module, and the Host code is for the OpenBCI Dongle Radio Module. 
Using the OpenBCI USB Dongle, it is possible to upload the OpenBCI_8bit_streamData_Filter to the target ATmega on the OpenBCI 8bit board. This version implements an optional high pass and 60Hz notch filter using a modified biquad library included.

Copy the Biquad folder to your Documents/Arduino/Libraries folder, and the OpenBCI_8bit_streamData_Filter folder to your Documents/Arduino folder. Select UNO from the boards menue int Arduino IDE, and the correct serial port of the Dongle. All OpenBCI hardware comes flashed with the latest firmware. Upgrading the OpenBCI Board firmware is possible over-the-air using the OpenBCI Dongle. Dongle (Host) and Board (Device) firmware is also upgradable using advanced version of the Arduino IDE and installing RFduino libraries as specified here http://www.rfduino.com/download-rfduino-library/ tutorial to be released soon.. 

Arduino code developed by Chip Audette, modified by Joel Murphy, Luke Travis, Conor Russomanno. Summer 2014

RFduino code developed by Joel Murphy, Leif Percifield, Conor Russomanno. Summer 2014

this software is provided as is. there is no attached warranty, or any claim of it's functionality. wysiwyg.
free to use and share.

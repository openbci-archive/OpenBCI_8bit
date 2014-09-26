Firmware for OpenBCI 8bit system
=================

The OpenBCI 8bit system is an open-source biopotential acquisisiton system. It incorporates an ADS1299 (analog to digital converter IC), LIS3DH (tripple axis accelerometer), Micro SD card, RFduino radio module, and an ATmega328 with Arduino UNO bootloader. The Device code is for the OpenBCI Board Radio Module, and the Host code is for the OpenBCI Dongle Radio Module. 
Using the OpenBCI USB Dongle, it is possible to upload the OpenBCI_8bit_streamData_Filter to the target ATmega on the OpenBCI 8bit board. This version implements an optional high pass and 60Hz notch filter using a modified biquad library included.

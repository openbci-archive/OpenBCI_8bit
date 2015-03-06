

#ifndef ____ADS1299__
#define ____ADS1299__

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SPI.h>
#include "pins_arduino.h"
#include "Definitions.h"

class ADS1299 {
public:
    
    void initialize();
    
    //ADS1299 SPI Command Definitions 
    //System Commands
    void WAKEUP(void);  // get out of low power mode
    void STANDBY(void); // go into low power mode
    void RESET(void);   // set all register values to default
    void START(void);   // start data acquisition
    void STOP(void);    // stop data acquisition
    
    //Data Read Commands
    void RDATAC();
    void SDATAC();
    void RDATA();
    
    //Register Read/Write Commands
    
    byte RREG(byte);                // read one register
    void RREGS(byte, byte);         // read multiple registers
    void WREG(byte, byte);          // write one register
    void WREGS(byte, byte);         // write multiple registers
	byte getDeviceID();
    void printRegisterName(byte);   // used for verbosity
    void printHex(byte);            // used for verbosity
    void updateChannelData(void);   // retrieve data from ADS
    
    byte xfer(byte);        // SPI Transfer function
    int DRDY, CS, RST; 		// pin numbers for DataRead ChipSelect Reset pins 
    int DIVIDER;		// select SPI SCK frequency
    int stat;			// used to hold the status register
    byte regData [24];	        // array is used to mirror register data
    long channelData [9];	// array used when reading channel data as ints
    byte bit24ChannelData[24];    // array to hold raw channel data
    char channelSettings[8][6];  // array to hold current channel settings
    char defaultChannelSettings[6];  // default channel settings
    boolean useInBias[8];        // used to remember if we were included in Bias before channel power down
    boolean useSRB1;             // used to keep track of if we are using SRB1
    boolean useSRB2[8];          // used to remember if we were included in SRB2 before channel power down
    char leadOffSettings[8][2];  // used to control on/off of impedance measure for P and N side of each channel
    boolean verbosity;		 // turn on/off Serial feedback
    
	
    void resetADS(void);                      //reset all the ADS1299's settings.  Call however you'd like
    void startADS(void);
    void stopADS(void);
	
    void writeChannelSettings(void);
    void writeChannelSettings(int);
    void activateChannel(int);    // activate a given channel 1-8
    void reportDefaultChannelSettings(void);
    void deactivateChannel(int);  //disable given channel 1-8
    
    void configureLeadOffDetection(byte, byte);
    void changeChannelLeadOffDetection(void);
    void configureInternalTestSignal(byte, byte);  
    
    boolean isDataAvailable(void);
    void writeADSchannelData(void);
    void printAllRegisters(void);
    void setSRB1(boolean desired_state);
    void printDeviceID(void);
	
private:

    boolean isRunning;
    void csLow();
    void csHigh();

};

#endif

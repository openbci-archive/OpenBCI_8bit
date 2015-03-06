

#ifndef _____OpenBCI_8__
#define _____OpenBCI_8__

#include <SdFat.h>
#include <SdFatUtil.h>

#include "ADS1299.h"
#include "LIS3DH.h"
#include "Definitions.h"

class OpenBCI {

  public:
  
    ADS1299 ads;
    LIS3DH accel;
    ADS1299 daisy;
  
    // VARIABLES
    char ADSchannelSettings[8][6];
    char ADSleadOffSettings[8][2];
    boolean useAccel;
    int outputType;  // we have a few output types
    int auxData[3];
    boolean useAux;

    // BOARD WIDE FUNCTIONS
    void initialize(void);
    void initialize(byte);
    void updateChannelSettings(void);
    void writeChannelSettings(void);
    void setChannelsToDefault(void);
    void setChannelsToEMG(void);
    void setChannelsToECG(void);
    void sendChannelData(byte);
    void reportDefaultChannelSettings(void);
    void startStreaming(void);
    void stopStreaming(void);
    void writeAuxData(void);

    //  ADS1299 FUNCITONS
    void initialize_ads(void);
    void printAllRegisters(void); 
    void updateADSchannelSettings(void);
    void writeADSchannelSettings(void); 
    void writeADSchannelSettings(int);
    void activateChannel(int);
    void activateChannel(int, byte, byte, boolean);
    void deactivateChannel(int);
    void configureInternalTestSignal(byte,byte);
    void configure_Zdetect(byte,byte);
    void changeChannelLeadOffDetect(void);
    void reset_ads(void);
    boolean isDataAvailable(void);
    void updateChannelData(void);
    void setSRB1(boolean);
    byte getADS_ID(void);
    byte defaultChannelBitField(void);
    long getChannel(int);
    void putChannel(int, long);
    void update24bitData(void);
    
    //  ACCELEROMETER FUNCIONS
    void initialize_accel(byte);
    void disable_accel(void);
    void enable_accel(byte);
    byte getAccelID(void);
    void updateAccelData(void);
    boolean LIS3DH_DataReady(void);
    boolean LIS3DH_DataAvailable(void);
    int getX(void);
    int getY(void);
    int getZ(void);
    void testAccel(void);
    void printAccelRegisters(void);
    
    
    
};

#endif

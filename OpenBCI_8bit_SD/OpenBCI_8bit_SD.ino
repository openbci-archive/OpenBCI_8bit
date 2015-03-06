/*
 * 
 *  >>>> THIS CODE DESIGNED FOR OBCI_8bit board <<<<
 *
 * This code is written to target an ATmega328P with UNO bootloader. 
 * Adjust as needed if you are testing on different hardware.
 *
 *
 * Made by Joel Murphy, Luke Travis, Conor Russomanno Summer, 2014. 
 * SDcard code is based on RawWrite example in SDFat library 
 * ASCII commands are received on the serial port to configure and control
 * Serial protocol uses '+' immediately before and after the command character
 * We call this the 'burger' protocol. the '+' re the buns. Example:
 * To begin streaming data, this code needs to see '+b+' on the serial port.
 * The included Dongle with OpenBCI_8bit_Host code is designed to insert the '+' characters.
 * Any PC or mobile device should send the command characters at 200Hz max. 
 * OpenBCI_8bit_Host will do the rest. You're welcome.
 *
 * This software is provided as-is with no promise of workability
 * Use at your own risk.
 *
 */ 

#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>   // not using SD. could be an option later
#include <SdFatUtil.h>
#include "OpenBCI_8.h"  


//------------------------------------------------------------------------------
//  << SD CARD BUSINESS >> has been taken out. See OBCI_SD_LOG_CMRR 
//  SD_SS on pin 7 defined in OpenBCI library
boolean use_SD = false;
char fileSize = '0';  // SD file size indicator
//------------------------------------------------------------------------------
//  << OpenBCI BUSINESS >>
boolean is_running = false;    // this flag is set in serialEvent on reciept of ascii prompt
OpenBCI OBCI; //Uses SPI bus and pins to say data is ready. 
byte sampleCounter = 0;
// these are used to change individual channel settings from PC
char currentChannel;    // keep track of what channel we're loading settings for
boolean getChannelSettings = false; // used to receive channel settings command
int channelSettingsCounter; // used to retrieve channel settings from serial port
int leadOffSettingsCounter;
boolean getLeadOffSettings = false;

// these are all subject to the radio requirements: 31byte max packet length (maxPacketLength - 1 for packet checkSum)
#define OUTPUT_NOTHING (0)  // quiet
#define OUTPUT_BINARY (1)  // normal transfer mode
#define OUTPUT_BINARY_SYNTHETIC (2)  // needs portage
int outputType;

//------------------------------------------------------------------------------
//  << LIS3DH Accelerometer Business >>
//  LIS3DH_SS on pin 5 defined in OpenBCI library
volatile boolean auxAvailable = false;
boolean useAccelOnly = false;
//------------------------------------------------------------------------------
//  << Auxiliary Data Business >>
//  useAux boolean in library used to send aux bytes with data stream
//  call it by saying OBCI.useAux = true/false
  int pushButton = A0;
	int pushButtonValue;
	int lastPushButtonValue;
//------------------------------------------------------------------------------

void setup(void) {

  Serial.begin(115200);
  
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  
  delay(1000);
  Serial.print(F("OpenBCI V3 8bit Board\nSetting ADS1299 Channel Values\n"));
  OBCI.useAccel = true;
  OBCI.initialize();  // configures channel settings on the ADS and idles the Accel
//setup the electrode impedance detection parameters. math on the PC side calculates Z of electrode/skin
  OBCI.configure_Zdetect(LOFF_MAG_6NA, LOFF_FREQ_31p2HZ);   
  Serial.print(F("ADS1299 Device ID: 0x")); Serial.println(OBCI.getADS_ID(),HEX);
  Serial.print(F("LIS3DH Device ID: 0x")); Serial.println(OBCI.getAccelID(),HEX);
  Serial.print(F("Free RAM: ")); Serial.println(FreeRam()); // how much RAM?
  sendEOT();
  
  pinMode(pushButton,INPUT);
  pushButtonValue = 0; 
  lastPushButtonValue = 0;
  
}



void loop() {
    
  if(is_running){
    
      while(!(OBCI.isDataAvailable())){}   // watch the DRDY pin

      OBCI.updateChannelData(); // retrieve the ADS channel data 8x3 bytes
      if(OBCI.LIS3DH_DataAvailable()){
        OBCI.updateAccelData();    // fresh axis data goes into the X Y Z 
        auxAvailable = true;    // pass the dataReady to SDCard, if present
      }
      if(use_SD){  
        writeDataToSDcard(sampleCounter);   // send the new data to SD card
      }
      OBCI.sendChannelData(sampleCounter);  // send the new data over radio
      
      sampleCounter++;    // get ready for next time
  }
  
    pushButtonValue = digitalRead(pushButton);    // feel the pushbutton
    if (pushButtonValue != lastPushButtonValue){  // if it's changed,
      if (pushButtonValue == HIGH){    // if it's gone from LOW to HIGH
        // 0x6220 converts to 3.14 in Processing
        OBCI.auxData[0] = OBCI.auxData[1] = OBCI.auxData[2] = 0x6220;	 
        OBCI.useAux = true;	         // set the OBCI.auxData flag
      }
      lastPushButtonValue = pushButtonValue; // keep track of the changes
    }

} // end of loop



// some variables to help find 'burger protocol' commands. don't laugh.
int plusCounter = 0;
char testChar;
unsigned long commandTimer;

void serialEvent(){
  while(Serial.available()){      
    char inChar = (char)Serial.read();  // take a gander at that!
    
    if(plusCounter == 1){  // if we have received the first 'bun'
      testChar = inChar;   // this might be the 'patty'
      plusCounter++;       // get ready to look for another 'bun'
      commandTimer = millis();  // don't wait too long! and don't laugh!
    }
  
    if(inChar == '+'){  // if we see a 'bun' on the serial
      plusCounter++;    // make a note of it
      if(plusCounter == 3){  // looks like we got a command character
        if(millis() - commandTimer < 5){  // if it's not too late,
          if(getChannelSettings){ // if we just got an 'x', expect channel setting parameters
            loadChannelSettings(testChar);  // go get channel settings prameters
          }else if(getLeadOffSettings){  // if we just got a 'z', expect impedeance detect parameters
            loadLeadOffSettings(testChar); // go get lead off settings parameters
          }else{
            getCommand(testChar);    // decode the command
          }
        }
        plusCounter = 0;  // get ready for the next one, whatever it is
      }
    }
  }
}
    
    
void getCommand(char token){
    switch (token){
// TURN CHANNELS ON/OFF COMMANDS
      case '1':
        changeChannelState_maintainRunningState(1,DEACTIVATE); break;
      case '2':
        changeChannelState_maintainRunningState(2,DEACTIVATE); break;
      case '3':
        changeChannelState_maintainRunningState(3,DEACTIVATE); break;
      case '4':
        changeChannelState_maintainRunningState(4,DEACTIVATE); break;
      case '5':
        changeChannelState_maintainRunningState(5,DEACTIVATE); break;
      case '6':
        changeChannelState_maintainRunningState(6,DEACTIVATE); break;
      case '7':
        changeChannelState_maintainRunningState(7,DEACTIVATE); break;
      case '8':
        changeChannelState_maintainRunningState(8,DEACTIVATE); break;
      case '!':
        changeChannelState_maintainRunningState(1,ACTIVATE); break;
      case '@':
        changeChannelState_maintainRunningState(2,ACTIVATE); break;
      case '#':
        changeChannelState_maintainRunningState(3,ACTIVATE); break;
      case '$':
        changeChannelState_maintainRunningState(4,ACTIVATE); break;
      case '%':
        changeChannelState_maintainRunningState(5,ACTIVATE); break;
      case '^':
        changeChannelState_maintainRunningState(6,ACTIVATE); break;
      case '&':
        changeChannelState_maintainRunningState(7,ACTIVATE); break;
      case '*':
        changeChannelState_maintainRunningState(8,ACTIVATE); break;
             
// TEST SIGNAL CONTROL COMMANDS
      case '0':
        activateAllChannelsToTestCondition(ADSINPUT_SHORTED,ADSTESTSIG_NOCHANGE,ADSTESTSIG_NOCHANGE); break;
      case '-':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_1X,ADSTESTSIG_PULSE_SLOW); break;
      case '=':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_1X,ADSTESTSIG_PULSE_FAST); break;
      case 'p':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_DCSIG); break;
      case '[':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_PULSE_SLOW); break;
      case ']':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_PULSE_FAST); break;

// SD CARD COMMANDS
      case 'A': case'S': case'F': case'G': case'H': case'J': case'K': case'L': case 'a':
        use_SD = true; fileSize = token; setupSDcard(fileSize); 
        break;
      case 'j':
        if(use_SD){  // what are the consequenses of closing the file....?
          closeSDfile(); use_SD = false;
        }
        break;

// CHANNEL SETTING COMMANDS
      case 'x':  // get ready to receive new settins for a channel
        if(!is_running) {Serial.println(F("ready to accept new channel settings"));}
        channelSettingsCounter = 0;  // initialize the channelSettingsCounter
        getChannelSettings = true; break;  // tell the serialEvent that we're doing this now
      case 'X':  // update the ADS with all new channel settings
        if(!is_running) {Serial.println(F("updating channel settings"));}
        writeChannelSettingsToADS(); break;  // push the new settings to the ADS chip
      case 'd':  // reset all channel settings to default
        if(!is_running) {Serial.println(F("updating channel settings do default"));}
        setChannelsToDefaultSetting(); break;
      case 'D':  // send the coded default channel settings to the controlling program
        sendDefaultChannelSettings(); break;  
      case 'c':
        // use 8 channel mode
        break;
      case 'C':
        // use 16 channel mode
        break;
        
// LEAD OFF IMPEDANCE DETECTION COMMANDS
      case 'z':
        if(!is_running) {Serial.println(F("ready to accept new impedance detect settings"));}
        leadOffSettingsCounter = 0;  // reset counter
        getLeadOffSettings = true;  // tell the serialEvent that we're doing this now 
        break;
      case 'Z':
        if(!is_running) {Serial.println(F("updating impedance detect settings"));}
        changeChannelLeadOffDetect_maintainRunningState();
        break;

// STREAM DATA COMMANDS
      case 'n':  
          // something useful here
        break;
      case 'b':
        if(use_SD) stampSD(ACTIVATE);
        OBCI.enable_accel(RATE_25HZ);      // fire up the accelerometer
        startRunning(OUTPUT_BINARY);       // turn on the fire hose
        break;
     case 's':
        stopRunning();
        if(use_SD) stampSD(DEACTIVATE);  // mark the SD log with millis() if it's logging
        break;
     case 'v':
       // something cool here
       break;
// QUERY THE ADS REGISTERS
     case '?':
       printRegisters();
       break;
       
// OTHER COMMANDS
     case '/':
//       something cool here
       break;
     default:
       break;
     }
  }// end of getCommand
  
void sendEOT(){
  Serial.print(F("$$$"));
}

void loadChannelSettings(char c){
//  char SRB_1;
  if(channelSettingsCounter == 0){  // if it's the first byte in this channel's array, this byte is the channel number to set
    currentChannel = c - '1'; // we just got the channel to load settings into (shift number down for array usage)
    if(!is_running) Serial.print(F("loading settings for channel ")); Serial.println(currentChannel+1,DEC);
    channelSettingsCounter++;
    return;
  }
//  setting bytes are in order: POWER_DOWN, GAIN_SET, INPUT_TYPE_SET, BIAS_SET, SRB2_SET, SRB1_SET
  if(!is_running) {
    Serial.print(F("load setting ")); Serial.print(channelSettingsCounter-1);
    Serial.print(F(" with ")); Serial.println(c);
  }
  c -= '0';
  if(channelSettingsCounter-1 == GAIN_SET){ c <<= 4; }  // shift the gain value to it's bit position
  OBCI.ADSchannelSettings[currentChannel][channelSettingsCounter-1] = c;  // assign the new value to currentChannel array
  if(channelSettingsCounter-1 == SRB1_SET){
    for(int i=0; i<8; i++){
      OBCI.ADSchannelSettings[i][SRB1_SET] = c;
    }
  }
  channelSettingsCounter++;
  if(channelSettingsCounter == 7){  // 1 currentChannel, plus 6 channelSetting parameters
    if(!is_running) Serial.print(F("done receiving settings for channel "));Serial.println(currentChannel+1,DEC);
    getChannelSettings = false;
  }
}

void writeChannelSettingsToADS(){
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType; 
  stopRunning();                   //must stop running to change channel settings
  
  OBCI.updateADSchannelSettings();    // change the channel settings
  
  if (is_running_when_called == true) {
    startRunning(cur_outputType);  //restart, if it was running before
  }
}

void setChannelsToDefaultSetting(){
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType; 
  stopRunning();  //must stop running to change channel settings
  OBCI.setChannelsToDefault();   // default channel settings
  if (is_running_when_called == true) {
    startRunning(cur_outputType);  //restart, if it was running before
  }
}

void loadLeadOffSettings(char c){
   if(leadOffSettingsCounter == 0){  // if it's the first byte in this channel's array, this byte is the channel number to set
    currentChannel = c - '1'; // we just got the channel to load settings into (shift number down for array usage)
    if(!is_running) Serial.print(F("changing LeadOff settings for channel ")); Serial.println(currentChannel+1,DEC);
    leadOffSettingsCounter++;
    return;
  }
//  setting bytes are in order: PCHAN, NCHAN
  if(!is_running) {
    Serial.print(F("load setting ")); Serial.print(leadOffSettingsCounter-1);
    Serial.print(F(" with ")); Serial.println(c);
  }
  c -= '0';
  OBCI.ADSleadOffSettings[currentChannel][leadOffSettingsCounter-1] = c;
  leadOffSettingsCounter++;
  if(leadOffSettingsCounter == 3){  // 1 currentChannel, plus 2 leadOff setting parameters
    if(!is_running) Serial.print(F("done receiving leadOff settings for channel "));Serial.println(currentChannel+1,DEC);
    getLeadOffSettings = false;
  }
}

boolean stopRunning(void) {
  if(is_running == true){
    OBCI.stopStreaming();                    // stop the data acquisition  //
    is_running = false;
    }
    return is_running;
  }

boolean startRunning(int OUT_TYPE) {
  if(is_running == false){
    outputType = OUT_TYPE;
    OBCI.startStreaming();    
    is_running = true;
  }
    return is_running;
}

int changeChannelState_maintainRunningState(int chan, int start)
{
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();
  if (start == true) {
    Serial.print(F("Activating channel "));
    Serial.println(chan);
    OBCI.activateChannel(chan);
  } else {
    Serial.print(F("Deactivating channel "));
    Serial.println(chan);
    OBCI.deactivateChannel(chan);
  }
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

// CALLED WHEN COMMAND CHARACTER IS SEEN ON THE SERIAL PORT
int activateAllChannelsToTestCondition(int testInputCode, byte amplitudeCode, byte freqCode)
{
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();
  //set the test signal to the desired state
  OBCI.configureInternalTestSignal(amplitudeCode,freqCode);    
  //loop over all channels to change their state
  for (int Ichan=1; Ichan <= 8; Ichan++) {
    OBCI.ADSchannelSettings[Ichan-1][INPUT_TYPE_SET] = testInputCode;
//    OBCI.activateChannel(Ichan,gainCode,testInputCode,false);  //Ichan must be [1 8]...it does not start counting from zero
  }
  OBCI.updateADSchannelSettings();
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

int changeChannelLeadOffDetect_maintainRunningState()
{
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();

  OBCI.changeChannelLeadOffDetect();
  
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

void sendDefaultChannelSettings(){
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  OBCI.reportDefaultChannelSettings();  // reads CH1SET 
  sendEOT();
  delay(10);
  
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

void printRegisters(){
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();
  
  if(is_running == false){
    // print the ADS and LIS3DH registers
    OBCI.printAllRegisters();
  }
  sendEOT();
  delay(20);
    //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}



// end








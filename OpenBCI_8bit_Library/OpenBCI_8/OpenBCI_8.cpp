

#include "OpenBCI_8.h"

void OpenBCI::initialize(void){
  pinMode(SD_SS,OUTPUT); digitalWrite(SD_SS,HIGH);  // de-select the SD card if it's there
  initialize_ads();
  initialize_accel(SCALE_4G);
}

void OpenBCI::initialize(byte G){
  pinMode(SD_SS,OUTPUT); digitalWrite(SD_SS,HIGH);  // de-select the SD card if it's there
  initialize_ads();
  initialize_accel(G);
}

void OpenBCI::initialize_ads(void) {
  ads.initialize();  
  for(int i=0; i<8; i++){
    for(int j=0; j<6; j++){
      ADSchannelSettings[i][j] = ads.channelSettings[i][j];
    }
  }
}

void OpenBCI::initialize_accel(byte g) {
  accel.initialize(g);
}

void OpenBCI::enable_accel(byte Hz) {
  accel.enable_accel(Hz);
}

void OpenBCI::disable_accel(void){
  accel.disable_accel();
}

byte OpenBCI::getAccelID(void){
  return accel.getDeviceID();
}

boolean OpenBCI::LIS3DH_DataReady(void){
  return accel.LIS3DH_DataReady();
}

boolean OpenBCI::LIS3DH_DataAvailable(void){
  return accel.LIS3DH_DataAvailable();
}

void OpenBCI::updateAccelData(void){
  accel.LIS3DH_updateAxisData();
}


int OpenBCI::getX(void){ return accel.getX(); }
int OpenBCI::getY(void){ return accel.getY(); }
int OpenBCI::getZ(void){ return accel.getZ(); }

void OpenBCI::printAccelRegisters(){
  accel.readAllRegs();
}


// ADS FUNCTIONS
void OpenBCI::printAllRegisters(void) {
  Serial.println("\nADS Registers:");
  ads.printAllRegisters();
  delay(100);
  Serial.println("LIS3DH Registers:");
  delay(100);
  accel.readAllRegs();
}

byte OpenBCI::getADS_ID(void){
  return ads.getDeviceID();
}

void OpenBCI::setChannelsToDefault(void){
  ADSchannelSettings[0][POWER_DOWN] = NO;        // NO = on, YES = off
  ADSchannelSettings[0][GAIN_SET] = ADS_GAIN24;     // Gain setting
  ADSchannelSettings[0][INPUT_TYPE_SET] = ADSINPUT_NORMAL;// input muxer setting
  ADSchannelSettings[0][BIAS_SET] = YES;    // add this channel to bias generation
  ADSchannelSettings[0][SRB2_SET] = YES;       // connect this P side to SRB2
  ADSchannelSettings[0][SRB1_SET] = NO;        // don't use SRB1
  for(int i=1; i<8; i++){
    for(int j=0; j<6; j++){
      ADSchannelSettings[i][j] = ADSchannelSettings[0][j];
    }
  }
  updateADSchannelSettings();
  
  for(int i=0; i<8; i++){
    ads.leadOffSettings[i][0] = ADSleadOffSettings[i][0] = OFF;
    ads.leadOffSettings[i][1] = ADSleadOffSettings[i][1] = OFF; 
  }
  ads.changeChannelLeadOffDetection();
}

void OpenBCI::setChannelsToEMG(void){
  ADSchannelSettings[0][POWER_DOWN] = NO;        // NO = on, YES = off
  ADSchannelSettings[0][GAIN_SET] = ADS_GAIN12;     // Gain setting
  ADSchannelSettings[0][INPUT_TYPE_SET] = ADSINPUT_NORMAL;// input muxer setting
  ADSchannelSettings[0][BIAS_SET] = NO;    // add this channel to bias generation
  ADSchannelSettings[0][SRB2_SET] = NO;       // connect this P side to SRB2
  ADSchannelSettings[0][SRB1_SET] = NO;        // don't use SRB1
  for(int i=1; i<8; i++){
    for(int j=0; j<6; j++){
      ADSchannelSettings[i][j] = ADSchannelSettings[0][j];
    }
  }
  updateADSchannelSettings();
}

void OpenBCI::setChannelsToECG(void){
  ADSchannelSettings[0][POWER_DOWN] = NO;        // NO = on, YES = off
  ADSchannelSettings[0][GAIN_SET] = ADS_GAIN12;     // Gain setting
  ADSchannelSettings[0][INPUT_TYPE_SET] = ADSINPUT_NORMAL;// input muxer setting
  ADSchannelSettings[0][BIAS_SET] = NO;    // add this channel to bias generation
  ADSchannelSettings[0][SRB2_SET] = NO;       // connect this P side to SRB2
  ADSchannelSettings[0][SRB1_SET] = NO;        // don't use SRB1
  for(int i=1; i<8; i++){
    for(int j=0; j<6; j++){
      ADSchannelSettings[i][j] = ADSchannelSettings[0][j];
    }
  }
  updateADSchannelSettings();
}

void OpenBCI::updateADSchannelSettings(void){
  for(int i=0; i<8; i++){
    for(int j=0; j<6; j++){
      ads.channelSettings[i][j] = ADSchannelSettings[i][j];
    }
  }
 ads.writeChannelSettings();
}

void OpenBCI::writeADSchannelSettings(void){
  ads.writeChannelSettings();
}

void OpenBCI::writeADSchannelSettings(int chan){
  for(int j=0; j<6; j++){
      ads.channelSettings[chan-1][j] = ADSchannelSettings[chan-1][j];
    }
  ads.writeChannelSettings(chan);
}

void OpenBCI::reportDefaultChannelSettings(void){
  ads.reportDefaultChannelSettings();
}

void OpenBCI::activateChannel(int chan){
  ads.activateChannel(chan);
}


void OpenBCI::deactivateChannel(int N){
  ads.deactivateChannel(N); 
}

void OpenBCI::startStreaming(void){
  if(useAccel){
    accel.enable_accel(RATE_25HZ);
  }
  ads.startADS();
}

void OpenBCI::stopStreaming(void){
  ads.stopADS();
  accel.disable_accel();
}

void OpenBCI::reset_ads(void){
  ads.resetADS();
}

boolean OpenBCI::isDataAvailable(void){
  return ads.isDataAvailable();
}

void OpenBCI::updateChannelData(void){
  ads.updateChannelData();
}

void OpenBCI::sendChannelData(byte sampleNumber){
  Serial.write(sampleNumber); // 1 byte
  ads.writeADSchannelData();  // 24 bytes
  if(useAux){ 
    writeAuxData();            // 3 16bit shorts
    useAux = false;
  }else{
    accel.writeLIS3DHdata();    // 6 bytes
  }
}

void OpenBCI::writeAuxData(){
  for(int i=0; i<3; i++){
    Serial.write(highByte(auxData[i])); // write 16 bit axis data MSB first
    Serial.write(lowByte(auxData[i]));  // axisData is array of type short (16bit)
    auxData[i] = 0;   // reset auxData bytes to 0
  }
}

long OpenBCI::getChannel(int chan){
  return ads.channelData[chan];
}

void OpenBCI::putChannel(int chan, long val){
  ads.channelData[chan] = val;
}

void OpenBCI::update24bitData(){
  int indexCounter = 0;
  for(int i=0; i<8; i++){
    for(int j=2; j>=0; j--){
      ads.bit24ChannelData[indexCounter] = byte(ads.channelData[i] >> (8*j));
      indexCounter++;
    }
  }
}

// Electrode-Off Detection Functions

void OpenBCI::changeChannelLeadOffDetect(){
  for(int i=0; i<8; i++){
    ads.leadOffSettings[i][0] = ADSleadOffSettings[i][0];
    ads.leadOffSettings[i][1] = ADSleadOffSettings[i][1];
  }
  ads.changeChannelLeadOffDetection();
}

void OpenBCI::configure_Zdetect(byte amplitudeCode, byte freqCode){
  ads.configureLeadOffDetection(amplitudeCode, freqCode);
}


//Configure the test signals that can be inernally generated by the ADS1299
void OpenBCI::configureInternalTestSignal(byte amplitudeCode, byte freqCode)
{
  ads.configureInternalTestSignal(amplitudeCode, freqCode);
}


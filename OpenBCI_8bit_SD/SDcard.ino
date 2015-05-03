//------------------------------------------------------------------------------
//  << SD CARD BUSINESS >>
//  SD_SS on pin 7 defined in OpenBCI library
//  bytes per block = 2 samplCounter+comma, 8*3+8*comma, 3*2 aux + comma

/* 
Max time expecting Accel/Aux included in each sample
Min	Sec	#samples	bytes/sample	samples/block	#blocks	req     TIME	 #blocks allocated
5	300	75000	              74	 6.918918919	10839.84375	5min	  11000
15	900	225000	              74	 6.918918919	32519.53125	15min	  33000
30	1800	450000	              74	 6.918918919	65039.0625	30min	  66000
60	3600	900000	              74	 6.918918919	130078.125	1hr	  131000
120	7200	1800000	              74	 6.918918919	260156.25	2hr	  261000
240	14400	3600000	              74	 6.918918919	520312.5	4hr	  521000
720	43200	10800000	      74	 6.918918919	1560937.5	12hr	  1561000
1440	86400	21600000	      74	 6.918918919	3121875	        24hr	  3122000
    
*/

#include <avr/pgmspace.h>

#define BLOCK_5MIN  11000
#define BLOCK_15MIN  33000
#define BLOCK_30MIN  66000
#define BLOCK_1HR  131000
#define BLOCK_2HR  261000
#define BLOCK_4HR  521000
#define BLOCK_12HR  1561000
#define BLOCK_24HR  3122000

uint32_t BLOCK_COUNT;
// time to produce a block of data not critical. used for benchmarking only
const uint32_t MICROS_PER_BLOCK = 2000; // minimum time that won't trigger overrun error
SdFat sd; // file system
SdFile file; // test file
uint32_t bgnBlock, endBlock; // file extent
#define error(s) sd.errorHalt_P(PSTR(s)) // store error strings in flash to save RAM
uint32_t b = 0;  // used to count blocks
uint8_t* pCache;  // used to cache the data before writing to SD
// benchmark stats
  uint16_t overruns = 0;
  uint32_t maxWriteTime = 0;
  uint16_t minWriteTime = 65000;
  uint32_t t;
// log of first overruns
#define OVER_DIM 20
struct {
  uint32_t block;   // holds block number that over-wrote
  uint32_t micro;  // holds the length of this of over-write
} over[OVER_DIM];

int byteCounter = 0;     // used to hold position in cache
boolean logging = false;
//unsigned int timeBetweenFiles;
//boolean betweenFiles = false;
byte fileTens, fileOnes;  // enumerate succesive files on card and store number in EEPROM 
char currentFileName[] = "OBCI_00.TXT";
const char elapsedTime[] PROGMEM = {"\n%Total time mS:\n"};  // 17
const char minTime[] PROGMEM = {"%min Write time uS:\n"};  // 20
const char maxTime[] PROGMEM = {"%max Write time uS:\n"};  // 20
const char overNum[] PROGMEM = {"%Over:\n"};              //  7
const char blockTime[] PROGMEM = {"%block, uS\n"};          // 11    74 chars + 2 32(16) + 2 16(8) = 98 + (n 32x2) up to 24 overruns...

const char stopStamp[] PROGMEM = {"%STOP AT\n"};      // used to stamp SD record when stopped by PC
const char startStamp[] PROGMEM = {"%START AT\n"};    // used to stamp SD record when started by PC


void writeDataToSDcard(byte sampleCount){ 
// convert 8 bit sampleCounter into HEX. 0000|0000 
  boolean addComma = true;
  convertToHex(long(sampleCount), 1, addComma);
         
  // convert 24 bit channelData into HEX. 0000|0000 0000|0000 0000|0000
  for (int currentChannel = 0; currentChannel < 8; currentChannel++){
    convertToHex(OBCI.ads.channelData[currentChannel], 5, addComma);
    if(currentChannel == 6){
      addComma = false;
      if(accelAvailable) {addComma = true;}  // format CSV
      if(OBCI.useAux) {addComma = true;}  
    }
  }
       
  if(accelAvailable == true){  // if we have accelerometer data to log
    // convert 16 bit accelerometer data into HEX 0000|0000 0000|0000  
    for (int currentChannel = 0; currentChannel < 3; currentChannel++){
      convertToHex(long(OBCI.accel.axisData[currentChannel]), 3, addComma);
      if(currentChannel == 1){
        addComma = false;
        if(OBCI.useAux) {addComma = true;}
      }
    }
    accelAvailable = false;  // reset accelerometer data flag
  }// end of accelerometer data log
      
  if(OBCI.useAux == true){  // if we have accelerometer data to log
    // convert 16 bit accelerometer data into HEX 0000|0000 0000|0000  
    for (int currentChannel = 0; currentChannel < 3; currentChannel++){
      convertToHex(long(OBCI.auxData[currentChannel]), 3, addComma);
      if(currentChannel == 1) {addComma = false;}
    }
    OBCI.useAux = false;  // reset accelerometer data flag
  }// end of aux data log  
  
}

void overRun(){
    // write the 512 byte block
    byteCounter = 0; // reset 512 byte counter
    uint32_t tw = micros();  // time the write
    if (!sd.card()->writeData(pCache)){ 
//      Serial.print(F("writeData failed")); 
//      Serial.println(b);
      b = b-1;  //should be b-=1?
    }
    tw = micros() - tw;
    // check for max write time
    if (tw > maxWriteTime) {maxWriteTime = tw;}
    // check for min write time
    if (tw < minWriteTime){minWriteTime = tw;}
    // check for overrun
    if (tw > MICROS_PER_BLOCK) {
      if (overruns < OVER_DIM) {
        over[overruns].block = b;
        over[overruns].micro = tw;
      }
      overruns++;
    }
    
    b++;    // increment BLOCK counter
    if(b == BLOCK_COUNT-1){
      t = millis() - t;  // measure total write time
      stopRunning();
      writeFooter();
    }
    if(b == BLOCK_COUNT){
      delay(2000);
      closeSDfile();  
    }  // we did it!
}

void setupSDcard(char limit){
  // use limit to determine file size
  switch(limit){
    case 'a': BLOCK_COUNT = 512; break;  // about 14 seconds
    case 'A': BLOCK_COUNT = BLOCK_5MIN; break;
    case 'S': BLOCK_COUNT = BLOCK_15MIN; break;
    case 'F': BLOCK_COUNT = BLOCK_30MIN; break;
    case 'G': BLOCK_COUNT = BLOCK_1HR; break;
    case 'H': BLOCK_COUNT = BLOCK_2HR; break;
    case 'J': BLOCK_COUNT = BLOCK_4HR; break;
    case 'K': BLOCK_COUNT = BLOCK_12HR; break;
    case 'L': BLOCK_COUNT = BLOCK_24HR; break;
    default: return; break;
  }
  incrementFileCounter();
  while (!sd.begin(SD_SS, SPI_FULL_SPEED)){ 
    Serial.println(F("SD begin fail... Card Insterted?"));
    sd.begin(SD_SS, SPI_FULL_SPEED);
  }
  boolean createContiguousFile = true;
  sd.remove(currentFileName);  // delete possible existing file. name must be 8.3 format
  // create a contiguous file
  if(!file.createContiguous(sd.vwd(), currentFileName, 512UL*BLOCK_COUNT)) Serial.print(F("createContiguous fail"));
  // get the location of the file's blocks
  if (!file.contiguousRange(&bgnBlock, &endBlock)) {
    Serial.print(F("contiguousRange fail"));
  }
  //*********************NOTE**************************************
  // NO SdFile calls are allowed while cache is used for raw writes
  //***************************************************************
  // clear the cache and use it as a 512 byte buffer
  pCache = (uint8_t*)sd.vol()->cacheClear();
  // tell card to setup for multiple block write with pre-erase
  if (!sd.card()->erase(bgnBlock, endBlock)) Serial.println(F("erase block fail"));
  if (!sd.card()->writeStart(bgnBlock, BLOCK_COUNT)) {
    Serial.println(F("writeStart fail"));
  }
  
  // initialize benchmark stats for write time test
    overruns = 0;  // number of overruns
    maxWriteTime = 0;  // longest block write time
    minWriteTime = 65000;  // shortest block write time
    b = 0;   // block counter
    t = millis();  // note the time to measure total time
    byteCounter = 0;
}

void writeFooter(){
  for(int i=0; i<17; i++){
      pCache[byteCounter] = pgm_read_byte_near(elapsedTime+i);
      byteCounter++;
      // if(byteCounter == 512){
      //   overRun();
      // }
    }
  convertToHex(t, 7, false);
  
  for(int i=0; i<20; i++){
    pCache[byteCounter] = pgm_read_byte_near(minTime+i);
    byteCounter++;
    // if(byteCounter == 512){
    //   overRun();
    // }
  }
  convertToHex(minWriteTime, 7, false);
  
  for(int i=0; i<20; i++){
      pCache[byteCounter] = pgm_read_byte_near(maxTime+i);
      byteCounter++;
      // if(byteCounter == 512){
      //   overRun();
      // }
    }
  convertToHex(maxWriteTime, 5, false);
  
  for(int i=0; i<7; i++){
      pCache[byteCounter] = pgm_read_byte_near(overNum+i);
      byteCounter++;
      // if(byteCounter == 512){
      //   overRun();
      // }
    }
  convertToHex(overruns, 5, false);
  for(int i=0; i<11; i++){
      pCache[byteCounter] = pgm_read_byte_near(blockTime+i);
      byteCounter++;
      // if(byteCounter == 512){
      //   overRun();
      // }
    }
  if (overruns) {
    uint8_t n = overruns > OVER_DIM ? OVER_DIM : overruns;
    for (uint8_t i = 0; i < n; i++) {
      convertToHex(over[i].block, 7, true);
      convertToHex(over[i].micro, 7, false);
    } 
  }
  overRun();
} 
  
void closeSDfile(){
  // end multiple block write mode
  if(use_SD) overRun();
  if (!sd.card()->writeStop()) Serial.println(F("writeStop fail"));
  file.close();

}


void incrementFileCounter(){
  fileTens = EEPROM.read(2);
  fileOnes = EEPROM.read(3);
  // if it's the first time writing to EEPROM, seed the file number to '00'  
  if(fileTens == 0xFF | fileOnes == 0xFF){
    fileTens = fileOnes = '0';
  }
  fileOnes++;   // enumerate the file name in HEX
  if (fileOnes == ':'){fileOnes = 'A';}
  if (fileOnes > 'F'){
    fileOnes = '0';
    fileTens++; 
    if(fileTens == ':'){fileTens = 'A';} 
    if(fileTens > 'F'){fileTens = '0';}
  }
  EEPROM.write(2,fileTens);
  EEPROM.write(3,fileOnes);
  currentFileName[5] = fileTens;
  currentFileName[6] = fileOnes;
  // send corresponding file name to controlling program
  Serial.print(F("Corresponding SD file "));Serial.print(currentFileName);sendEOT(); 
  
}



void stampSD(boolean state){
  unsigned long t = millis();
  if(state){
    for(int i=0; i<10; i++){
      pCache[byteCounter] = pgm_read_byte_near(startStamp+i);
      byteCounter++;
      if(byteCounter == 512){
        overRun();
      }
    }
  }else{
    for(int i=0; i<9; i++){
      pCache[byteCounter] = pgm_read_byte_near(stopStamp+i);
      byteCounter++;
      if(byteCounter == 512){
        overRun();
      }
    }  
  }
  convertToHex(t, 7, false);
}

unsigned int numKblocks(){
  return BLOCK_COUNT/1000;
}

void convertToHex(long rawData, int numNibbles, boolean addComma){

    for (int currentNibble = numNibbles; currentNibble >= 0; currentNibble--){
      byte nibble = (rawData >> currentNibble*4) & 0x0F;
      if (nibble > 9){
        nibble += 55;  // convert to ASCII A-F
      }else{
        nibble += 48;  // convert to ASCII 0-9
      }
      pCache[byteCounter] = nibble;
      byteCounter++;
      if(byteCounter == 512){
        overRun();
      }
    }
    if(addComma == true){
      pCache[byteCounter] = ',';
    }else{
      pCache[byteCounter] = '\n';
    }
    byteCounter++; 
    if(byteCounter == 512){
      overRun();
    }
}// end of byteToHex converter


//end

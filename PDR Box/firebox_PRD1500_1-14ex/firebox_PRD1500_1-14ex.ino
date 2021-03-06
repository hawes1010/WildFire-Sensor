//#include <Adafruit_GPS.h>



// This code is used for the Teensy 3.2
// The main purpose is to log and transfer PDR and GPS sensor data
// Version 1_12: added m/d/y to usb data, fixed pdr buad from 19400 to 19200

// ***************
// PIN Assignmets:
// ***************

// SD Card (SPI)
// pin 6 - CS
// pin 11 - MOSI (DOUT)
// pin 12 - MISO (DIN)
// pin 13 - SCK (CLK)

//Serial Ports
// PDR
// pin 1(TX1) - RX
// pin 0(RX1) - TX
// GPS
// pin 10(TX2) - RX
// pin 9(RX2) - TX
// USB
// pin 8(TX3) - RX
// pin 7(RX3) - TX

// Monitor battery voltage
// pin A8(22) - voltpin

// PDR on/offw
// pin 2

// **********************
// End of pin assignments
// **********************


// Libraries ---------------------------------------------------------------------------------
#include <SD.h>                 // SD card library
#include <Wire.h>               // IC2 library
#include <TimeLib.h>            // Used for rtc
#include <SPI.h>                // Used for rtc
#include <TinyGPS++.h>
//#include <Adafruit_GPS.h>       // GPS library

TinyGPSPlus gps;

#define PDRSerial Serial1
#define GPSSerial Serial2
#define VDIPSerial Serial3
//#define XbeeSerial Serial3
File myfile;
char filename[] = "";

//Adafruit_GPS GPS(&GPSSerial);   // GPS library

int first_time = 0;
// Declare pins ----------------------------------------------------------------------------------------
const int SDPin = 6;        // CS pin for SD card
const int PDRPin = 2;       // PDR on/off
const int VoltPin = A8;     // Monitor package voltage
String inByte = "";

// Define variables -----------------------------------------------------------------------------------

int oldsec;
int newsec;
int Volt = 0.0;                  // Package voltage (Volt)
int Altitude = 0;              // Altitude (m)
String latt = "";
String lngt = "";
int gpsyear = 0;
int gpsmonth = 0;
int gpsday = 0;
int gpshour = 0;
int gpsmin = 0;
int gpssec = 0;
int checksum = 0;
String pinByte = "";
String PDRByte = "";
String payload = "";
int noOfChars;
int first_run= 0;
//String NMEA1; //Variable for first NMEA sentence
//String NMEA2; //Variable for second NMEA sentence
char c; //to read characters coming from the GPS
int i = 0;
int oldloop = 0;
int newloop = 0;
int looptime = 0;
int lock = 0;

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void setup() {
  
  Wire.begin();
  setSyncProvider(getTeensy3Time); // Call Teensy RTC subroutine
  analogReadRes(13); // Set Teensy analog read resolution to 13 bits
  
  // Open serial communications and wait for port to open:  --------------------------------------
  Serial.begin(115200);   // computer serial I/O port
  //XbeeSerial.begin(38400);  // Xbee
  GPSSerial.begin(9600);  // GPS
  PDRSerial.begin(19200);  // PDR
  VDIPSerial.begin(9600);  // usb
  
  //setSyncProvider(getTeensy3Time); // Call Teensy RTC subroutine
  
  pinMode(VoltPin, INPUT);        // Package voltage
  pinMode(PDRPin, OUTPUT);        //PDR on/off
  digitalWrite(PDRPin, HIGH);    //Turn off PDR on pulse
  delay(1000);  
  digitalWrite(PDRPin, LOW);     //PDR on pulse
  delay(4500);
  digitalWrite(PDRPin, HIGH);    //Turn off PDR on pulse
    
  SPI.begin();

  VDIPSerial.print("IPA");        //usb stick
  VDIPSerial.print(13, BYTE);     //usb stick
  
  
   // GPS start ------------------------------------------------------------------------------------
  //GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);

  // Set the update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rat
   
  //delay(1000); 
    
 //init SD card ---------------------------------------------------------------------------------
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(SDPin, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin()) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  //end SD card init

  oldsec = gps.time.second();
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
void loop() {
  //oldloop = millis();
   // GPS time, coordinate, and altitude
   
    while(GPSSerial.available() > 0)  //Loop until you have a good NMEA sentence
     if (gps.encode(Serial2.read()))

  
  // Wait for command from base station

  inByte = "";
 
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    inByte += inChar;
    i = 1;   
  } 
//Serial.print(inByte);    
   if (i == 1 && inByte.startsWith("T")) {               //if first chars = T set clock
    inByte = inByte.substring(1);
    Serial.println(inByte); 
    time_t t = processSyncMessage(); //call processSyncMessage subroutine & return "t"
    
     if (t != 0) {
      Teensy3Clock.set(t); // set the RTC
      setTime(t);
      Serial.print("CLOCK SET TO: ");
      Serial.print(month());
      Serial.print("/");
      Serial.print(day());
      Serial.print("/");
      Serial.print(year());
      Serial.print(",");
      Serial.print(hour());
      Serial.print(":");
      Serial.print(minute());
      Serial.print(":");
      Serial.println(second());
      Serial.println();
     }
     //Serial.println(">AST1");  // send ack
     i = 0;
     inByte = "";
    }

  if (i == 1 && inByte.startsWith("D")) {   //Print Directory 
        File root = SD.open("/");
    printFiles(root,0);
  i = 0;
  inByte = "";      
  }

  if (i == 1 && inByte.startsWith("R")) {   //stream file to serial port
  inByte = inByte.substring(1); 
  inByte.toCharArray(filename,13); 
  Serial.println(filename); 
  
  myfile = SD.open(filename);
  while (myfile.available()){
    Serial.write(myfile.read());
  }
  myfile.close();
  
  i = 0;
  inByte = "";   
  }

    //delay(50);
  
  if (oldsec - gps.time.second() != 0) 
  {

      
    // PDR read requset
    
    //pdr1500 ();
    //read DPR end

                       
    oldsec =gps.time.second(); // restart timer


    // Datalogging
    if (first_time > 20){
    pdr1500 (); 
    logger();
    }
    else{
    // removed delay (causes GPS errors)
    first_time++;
    }
  }

//newloop = millis();
//looptime = newloop - oldloop;
  
}

void pdr1500()
{
 PDRSerial.println("O");  // shows that output is beginning
// if(first_run < 10){ // let bad data clear out, run for 10 samples and then start collecting data points.
// delay (1000);
// first_run++;
 
// }
//else{
 //delay(100);
 boolean data_is_valid = false;  // checking if valid coming through is correct  or not
 boolean data_received = false;  // checking for first integer variable.
 // 
 int valid_data_length = 0;
 // if data is valid, check to make sure we don't see early cutoff of data.
 int data_pointer = 0;
  while(PDRSerial.available() > 0) 
 {
  data_pointer++;
  char PDRChar = PDRSerial.read();
  int x = (int)PDRChar; //x is the ascii int code of the character PDRChar
  if (data_pointer > 8){
  if (PDRChar == '\r' || x == 13  || x == 10){ // for some reason, not all return characters are being recognized.
    if (!data_is_valid){  
    break;
    }  
    else{
    if (valid_data_length < 14){
      valid_data_length = 0;
      PDRByte ="";
      break;
    }
    else 
    valid_data_length = 0;
    break;
      }
  }
  else if (x == 110 ){ // n/a message is received, print "1" message
    PDRChar = '1';
    PDRByte = "1";
    break;
  }
  else if (x == 32 && data_received) // if we receive a space after data, make it a comma
  {
    PDRChar = ',';
     PDRByte += PDRChar;
     valid_data_length++;
  }
  else if (x == 32 && !data_received){ // if we receive a space before data, do nothing to it
    PDRByte += PDRChar;
      valid_data_length++;

  }
  else if ( x == 46  || (x>=48 && x<= 57)){ //checks for "normal" characters when the Character is loaded in
  //Normal Chars:  46 == period, 48<=x<=57 == 0-9
  //N/a points become ones
  data_is_valid = true;
  data_received = true;
  valid_data_length++;
  
  PDRByte += PDRChar;
  //  Serial.print(PDRChar);
  }
  else {
    //Serial.print("error: ");
    //Serial.println((int)PDRChar);
    //continue;
  }
 }
 }
 //PDRByte = PDRByte.substring(7); // Basically read in OUTPUT and then after 7 is your data that you actually record 
// }
}


// logging data
void logger()
{
  looptime = looptime + 1;
  if (looptime > 14400){
    GPSSerial.clear();
    looptime = 0;   
  }
  //Serial.println(looptime);
 

    Altitude = (gps.altitude.meters());
    lngt = String(gps.location.lng(), 6);
    latt = String(gps.location.lat(), 6);
    gpsyear = (gps.date.year());
    gpsmonth = (gps.date.month());
    gpsday = (gps.date.day());
    gpshour = (gps.time.hour());
    gpsmin = (gps.time.minute());
    gpssec = (gps.time.second());
    checksum = (gps.failedChecksum());    

    if (gps.location.isValid())
  {
    lock = 1;
  }
    else
  {
    lock = 0;
  }
                       
  //Begin Write data to file *************************************
  // open file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
    char filename[] = "00000000.CSV"; //default file name
    getFilename(filename); //daily file name
    //Serial.println("Logger called");
    //Serial.println(PDRByte);
    //Serial.println(filename);
    payload = ""; 
    payload += month();
    payload += "/";
    payload += day();
    payload += "/";
    payload += year(); 
    payload += ",";   
    payload += hour(); 
    payload += ":"; 
    payload += minute();
    payload += ":";
    payload += second();
    payload += ",";
    payload += gps.date.month(); 
    payload += "/";
    payload += gps.date.day(); 
    payload += "/";
    payload += gps.date.year();             
    payload += ",";
    payload += gps.time.hour(); 
    payload += ":";
    payload += gps.time.minute();
    payload += ":";
    payload += gps.time.second();
    payload += ",";
    payload += latt;
    payload += ",";
    payload += lngt;
    payload += ",";
    payload += gps.altitude.meters();
    payload += ",";
    payload += checksum;    
    payload += ",";
    payload += lock;
    payload += ",";
    payload += PDRByte;
    //payload += ",";
    //payload += looptime;        
    noOfChars = payload.length() + 2;

    Serial.println(payload);
    
   // Serial.println(payload);
   if(filename){
    VDIPSerial.print("OPW ");
    VDIPSerial.print(filename);    
    VDIPSerial.print(13, BYTE);
    VDIPSerial.print("WRF ");
    VDIPSerial.print(noOfChars);
    VDIPSerial.print(13, BYTE);
    VDIPSerial.println(payload);
   // VDIPSerial.print(month());
   // VDIPSerial.print("/");
   // VDIPSerial.print(day());
    //VDIPSerial.print("/");
    //VDIPSerial.print(year());
   // VDIPSerial.print(",");
   // VDIPSerial.print(hour());
    //VDIPSerial.print(":");
    //VDIPSerial.print(minute());
    //VDIPSerial.print(":");
    //VDIPSerial.print(second());
    //VDIPSerial.print(",");
    //VDIPSerial.print(gps.date.month()); VDIPSerial.print("/");
   // VDIPSerial.print(gps.date.day()); VDIPSerial.print("/");
   // VDIPSerial.print(gps.date.year()); VDIPSerial.print(",");      
   // VDIPSerial.print(gps.time.hour()); VDIPSerial.print(':');
   // VDIPSerial.print(gps.time.minute()); VDIPSerial.print(':');
   // VDIPSerial.print(gps.time.second()); VDIPSerial.print(',');            
   // VDIPSerial.print(gps.location.lat(), 6); VDIPSerial.print(',');
  // VDIPSerial.print(gps.location.lng(), 6); VDIPSerial.print(",");
   // VDIPSerial.print(gps.altitude.meters()); VDIPSerial.print(",");
   // VDIPSerial.print(gps.failedChecksum()); VDIPSerial.print(",");
    //VDIPSerial.println(PDRByte);      
          
    VDIPSerial.print(13, BYTE);
    VDIPSerial.print("CLF ");
    VDIPSerial.print(filename);
    VDIPSerial.print(13, BYTE);
   }
   else{
    Serial.println("error opening USB file");
    PDRByte = "";
   }
      
    
    payload = "";
    
    File dataFile = SD.open(filename, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    
    //dataFile.print(looptime);
   // dataFile.print(",");
    dataFile.print(month());
    dataFile.print("/");
    dataFile.print(day());
    dataFile.print("/");
    dataFile.print(year());
    dataFile.print(",");
    dataFile.print(hour());
    dataFile.print(":");
    dataFile.print(minute());
    dataFile.print(":");
    dataFile.print(second());
    dataFile.print(",");
    
    // GPS time, coordinate, and altitude
    dataFile.print(gps.date.month()); dataFile.print("/");
    dataFile.print(gps.date.day()); dataFile.print("/");
    dataFile.print(gps.date.year()); dataFile.print(",");      
    dataFile.print(gps.time.hour()); dataFile.print(":");
    dataFile.print(gps.time.minute()); dataFile.print(":");
    dataFile.print(gps.time.second()); dataFile.print(",");            
    dataFile.print(gps.location.lat(), 6); dataFile.print(",");
    dataFile.print(gps.location.lng(), 6); dataFile.print(",");
    dataFile.print(gps.altitude.meters()); dataFile.print(",");
    dataFile.print(gps.failedChecksum()); dataFile.print(",");
    dataFile.print(lock); dataFile.print(","); 
    // PDR data:
    dataFile.println(PDRByte);
    dataFile.close(); //close file  
    PDRByte = "";
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog file");
    PDRByte = "";
  }
//while(GPSSerial.read() >= 0) ; // flush the GPS receive buffer
//NMEA1 = "";  

 //Serial.println(looptime);
  //oldloop = 0;
 // newloop = 0;
  //looptime = 0;
  //GPSSerial.clear();
  //PDRSerial.clear();

}

//RTC subroutine
time_t getTeensy3Time() 
{
  return Teensy3Clock.get();
}
/*  code to process time messages from the serial port   */
//#define TIME_HEADER  "T"   // Header tag for serial time sync message

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  
     pctime = inByte.toInt();  
 
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
    return pctime; 
  }
  
    //file date / name generator:
void getFilename(char *filename) {
    filename[0] = '2';
    filename[1] = '0';
    filename[2] = int(year()/10)%10 + '0';
    filename[3] = year()%10 + '0';
    filename[4] = month()/10 + '0';
    filename[5] = month()%10 + '0';
    filename[6] = day()/10 + '0';
    filename[7] = day()%10 + '0';
    filename[8] = '.';
    filename[9] = 'C';
    filename[10] = 'S';
    filename[11] = 'V';
    
}

void printFiles(File dir, int numTabs)
{
  while (true)
  {
    File entry =  dir.openNextFile();
    if (! entry)
    {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++)
    {
      Serial.print('\t');
    }
    Serial.println(entry.name());
    entry.close();
  }
}

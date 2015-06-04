
// Arduino 1.0.6 ATMega 328             
// Versioan 1.0.BETA 23.12.2014       
// Made GitHub RFID Ver 1.0  OK       
// Update by Romik 2.06.2015

#include <LiquidCrystalRus.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <RTClib.h>

#define TIMECTL_MAXTICS 4294967295L
#define TIMECTL_INIT          0

#define UTC 3 //  UTC+3 = Moscow

#define SS_PIN   10
#define RST_PIN  5
#define DS1307_ADDRESS   0x68
#define ADDR_EEPROM        0x50

TinyGPSPlus gps;                                    // Attach GPS Library
SoftwareSerial bt(7,6);                             // (7)RX,(6)TX => BlueToth HC-05
SoftwareSerial ss(3,255);                         // 4 - Не подключен = GPS EM-406A
LiquidCrystalRus lcd(8,2,A0,A1,A2,A3);   // Display
MFRC522 mfrc522(SS_PIN, RST_PIN);	 // Create MFRC522 instance.
RTC_DS1307 rtc;                                     // DS1307 Real Time Clock

unsigned long gpsTimeMark = 0;
unsigned long gpsTimeInterval = 1000;

unsigned long SetgpsTimeMark     = 0;
unsigned long SetgpsTimeInterval = 1000*60*10; // 10 Минут

//byte mode = 2;

int ADCvalue;     
byte knopka = 0;  // Номер нажатой кнопки

long previousMillis = 0;

struct nfc_t    
{
  double lat,lng;                // GPS Coordinates     
  byte nfcid[10];                // NFC ID
  unsigned long datetime; // UnixTimeStamp

} 
nfc_id;

// char NMEA;
// char buff[83]; 


//int ii = 0;
//int jj = 0;
//boolean gps_stat = false;
//int dis = 2;
//boolean mode_status = true;

void setup() {

  SPI.begin();
  Wire.begin();

  mfrc522.PCD_Init();    // Init MFRC522 card

  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("RFID Ver 1.0");
  lcd.setCursor(0,1);
  lcd.print("Romik 04.06.2015");
  delay(2000);

  bt.begin(9600);  // Bletooth
  ss.begin(4800); // GPS

  ss.listen();

  lcd.clear();
  i2c_scanner(); // Check I2C Devices
  delay(2000);

  lcd.clear();



  if (! rtc.isrunning()) {
    lcd.println(F("RTC Error."));
  } 
  else {

    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    DateTime now = rtc.now();
    print_time(0);
  }    

  delay(1000);

}

void loop() {

  unsigned long currentMillis = millis();


  if (isTime(&SetgpsTimeMark,SetgpsTimeInterval)) 
    set_GPS_DateTime();

  if (knopka == 2 || knopka == 22) {
    get_nfc();
  }

  if (knopka == 3) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Wait GPS ..."));
    knopka = 33;
  }

  if (knopka == 33) {
    while (ss.available() > 0) gps.encode(ss.read());
    if (isTime(&gpsTimeMark,gpsTimeInterval)) {
      if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
        lcd.clear();
        lcd.setCursor(0,1); 
        lcd.print(gps.location.lat(), 6); // Lat
        lcd.print(F(" "));
        lcd.print(gps.location.lng(), 6); // Lng
        lcd.setCursor(0,0);                   
        lcd.print(gps.time.hour()); 
        lcd.print(":");
        lcd.print(gps.time.minute()); 
        lcd.print(":"); 
        lcd.print(gps.time.second()); 
        lcd.print(" ");
        lcd.println(gps.date.year()); 
        lcd.print("/");
        lcd.println(gps.date.month()); 
        lcd.print("/");
        lcd.println(gps.date.day()); 
      }  
      else {               
        lcd.setCursor(0,1);
        for(byte x=0;x<16;x++) lcd.print(F(" "));
        lcd.setCursor(0,1); 
        lcd.print(gps.charsProcessed());
      }
    }
  }

  if( currentMillis - previousMillis > 1000) {
    previousMillis = currentMillis;      
    if  (knopka == 0 || knopka == 1) {
      print_time( knopka );
    }
  }

  // ----------- Чтение значение кнопок .--------------------------------

  ADCvalue = analogRead(7);

  if (ADCvalue > 719 && ADCvalue < 723) {
    knopka = 1;
  }

  if (ADCvalue > 656 && ADCvalue < 660) {
    knopka = 2;
  }

  if (ADCvalue > 628 && ADCvalue < 632) {
    knopka =3;
  }

  //if (dis == 2) { lcd.clear(); lcd.print("Поиск спутников"); }
  //if (dis == 3) { lcd.clear(); lcd.print("Ожидание карты"); }

  /*  switch (mode) {
   case 2:
   if (mode_status) mode_status = false;
   break;
   case 3:
   if (mode_status) mode_status = false;
   break;
   }
   */

  // =========================== GPS ==========================

  /* if (mode == 22) {   
   while (ss.available() > 0) {
   if (ii==82) ii = 0;
   buff[ii++] = char(ss.read());
   if (buff[(ii-1)] == '\n') { 
   buff[(ii+1)] = '\0'; 
   if (strstr(buff,"RMC")) {
   for (jj=0;jj<ii;jj++) bt.print(buff[jj]); 
   }
   if (strstr(buff,"A")) gps_stat = true; 
   else gps_stat = false;
   ii = 0; 
   }
   }
   }
   
   */

}  // --------------------- End of Loop ----------------------------------------------------

// -------------------------------------- MFRC Functions --------------------------------

void get_nfc() {

  if (knopka == 2) {
    knopka = 22;
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print(F("Ожидание карты"));
  }

  if (mfrc522.PICC_IsNewCardPresent()) {
    if ( mfrc522.PICC_ReadCardSerial()) {

      lcd.clear(); 
      lcd.setCursor(0,0);          

      for (byte i = 0; i < mfrc522.uid.size; i++)
        lcd.print(mfrc522.uid.uidByte[i], HEX);
      mfrc522.PICC_HaltA();

      if (mfrc522.uid.size > 0) {

        for(byte i=0;i<10;i++) nfc_id.nfcid[i]=0;

        lcd.setCursor(0,1);
        lcd.print(F("Запись"));
        for (byte x=6;x<16;x++) {
          lcd.setCursor(x,1);
          lcd.print(F(".")); 
          delay(100);
        } 
        knopka = 2; 
      }
    } 
  }

}
// -------------------------------------- Функуии ------------------------------------------

void i2c_scanner() {

  lcd.setCursor(0,0);

  byte error, address;
  int nDevices;

  nDevices = 0;

  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      if (address<16) lcd.print("I2C");      
      lcd.print("Нашли");
      lcd.print(": 0x"); 
      lcd.print(address,HEX);
      lcd.print(" OK");
      lcd.setCursor(0,1);        
      nDevices++;
    }
  }
}

void print_time(byte kn ) {

  DateTime now = rtc.now();

  char zero = '0';

  lcd.setCursor(0,0);       
  lcd.print(F("Date: "));
  if (now.day() < 10) lcd.print(zero);
  lcd.print(now.day(), DEC);
  lcd.print('/');
  if (now.month() < 10) lcd.print(zero);
  lcd.print(now.month(), DEC);
  lcd.print('/');
  lcd.print(now.year(), DEC);

  lcd.setCursor(0,1);  
  lcd.print(F("Time: "));     
  if (now.hour() < 10) lcd.print(zero);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  if (now.minute() < 10) lcd.print(zero);
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  if (now.second() < 10) lcd.print(zero);
  lcd.print(now.second(), DEC);

  // lcd.setCursor(15,1);
  // lcd.print(kn);
}

void displayInfo()
{
  /*
  if (gps.location.isValid())
   {
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("N: ");
   lcd.print(gps.location.lat(), 6);
   lcd.setCursor(0,1);
   lcd.print("E: ");
   lcd.println(gps.location.lng(), 6);
   } 
   else {
   lcd.clear();
   lcd.print("Поиск спутников.");
   lcd.setCursor(0,1);
   lcd.print("Спутников: ");
   lcd.print(gps.satellites.value());
   }
   */
}

int isTime(unsigned long *timeMark, unsigned long timeInterval) {
  unsigned long timeCurrent;
  unsigned long timeElapsed;
  int result=false;

  timeCurrent = millis();
  if (timeCurrent < *timeMark) {
    timeElapsed=(TIMECTL_MAXTICS-*timeMark) + timeCurrent;
  } 
  else {
    timeElapsed=timeCurrent-*timeMark;
  }

  if (timeElapsed>=timeInterval) {
    *timeMark=timeCurrent;
    result=true;
  }
  return(result);
}

// ---------------------------- Установка времени через GPS ------------------------

void set_GPS_DateTime() {

  if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {

    DateTime utc = (DateTime (gps.date.year(), 
    gps.date.month(), 
    gps.date.day(),
    gps.time.hour(),
    gps.time.minute(),
    gps.time.second()) + 60 * 60 * UTC);

    rtc.adjust(DateTime(utc.unixtime()));

  }

}


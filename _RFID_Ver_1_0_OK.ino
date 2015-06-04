
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

byte mode = 2;
int ADCvalue;  
long previousMillis = 0;
long interval_time = 800; 
char NMEA;
char buff[83]; 
int ii = 0;
int jj = 0;
boolean gps_stat = false;
int dis = 2;
boolean mode_status = true;

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
   } else {
       
     // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
       
     DateTime now = rtc.now();
     print_time();
   }    
   
  delay(3000);

}

void loop() {
    
   unsigned long currentMillis = millis();
      
    if( currentMillis - previousMillis > 1000) {
      previousMillis = currentMillis;      
      print_time();
    }
    
    
  ADCvalue = analogRead(7);

  if (ADCvalue > 719 && ADCvalue < 723) {
    dis++; 
    if (dis > 10) dis=0;
  }

  if (ADCvalue > 656 && ADCvalue < 660) {
    mode = dis;
    mode_status = true;
  }

  if (ADCvalue > 628 && ADCvalue < 632) {
    if (dis != 0) dis--;
  }

  //if (dis == 2) { lcd.clear(); lcd.print("Поиск спутников"); }
  //if (dis == 3) { lcd.clear(); lcd.print("Ожидание карты"); }
     
  switch (mode) {
  case 2:
    if (mode_status) mode_status = false;
    break;
  case 3:
    if (mode_status) mode_status = false;
    break;
  }

  // =========================== GPS ==========================

  if (mode == 2) {   
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
    
    /*
    
    if (gps_stat) { 
      lcd.setCursor(0,1); 
      lcd.print("GPS OK  "); 
    } 
    else { 
      lcd.setCursor(0,1); 
      lcd.print("GPS None"); 
    } 
    */
  } // --------------------- End of Loop ----------------------------------------------------

  // ============ Часы ========================================

  if (mode == 1) {
   
  }

  // ================= Ожидание карты ======================

  if (mode == 3) {

    if (mfrc522.PICC_IsNewCardPresent()) {
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        lcd.clear(); 
        lcd.setCursor(0,0);
        for (byte i = 0; i < mfrc522.uid.size; i++)
          lcd.print(mfrc522.uid.uidByte[i], HEX);
        mfrc522.PICC_HaltA();        
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

void print_time() {

     DateTime now = rtc.now();
      
     char zero = '0';
     
     lcd.clear();
      
     lcd.setCursor(0,0);       
     
     if (now.day() < 10) lcd.print(zero);
     lcd.print(now.day(), DEC);
     lcd.print('/');
     if (now.month() < 10) lcd.print(zero);
     lcd.print(now.month(), DEC);
     lcd.print('/');
     lcd.print(now.year(), DEC);
     
     lcd.setCursor(0,1);            
     if (now.hour() < 10) lcd.print(zero);
     lcd.print(now.hour(), DEC);
     lcd.print(':');
     if (now.minute() < 10) lcd.print(zero);
     lcd.print(now.minute(), DEC);
     lcd.print(':');
     if (now.second() < 10) lcd.print(zero);
     lcd.print(now.second(), DEC);

}


byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
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









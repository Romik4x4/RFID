
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
#include <EEPROM.h>
#include <EEPROMAnything.h>
#include <I2C_eeprom.h>

struct config_t   // Сохранение последней записи.
{
  unsigned  int ee_pos;
  byte last_knopka;
  unsigned int  card_count;
} 
configuration;

#define EEPROM_ADDRESS        (0x50)  // 27LC512

#define EE24LC512MAXBYTES 524288/8 // 65536 Байт [0-65535]
#define EE24LC256MAXBYTES 262144/8 // 32768 Байт [0-32767]

// НУЖНО В БИБЛИОТЕКЕ ПОМЕНЯТЬ uint16_t --> long

I2C_eeprom eeprom(EEPROM_ADDRESS,EE24LC512MAXBYTES); // Attach EEPROM

#define DEBUG 1

#define TIMECTL_MAXTICS 4294967295L
#define TIMECTL_INIT          0

#define UTC 3 //  UTC+3 = Moscow

#define SS_PIN   10
#define RST_PIN  5

#define DS1307_ADDRESS   0x68
#define ADDR_EEPROM        0x50  // 27LC512

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

int ADCvalue;     
byte knopka = 0;  // Номер нажатой кнопки

long previousMillis = 0;

unsigned int card_count;

struct nfc_t    
{
  double lat,lng;                // GPS Coordinates     
  byte nfcid[10];                // NFC ID
  unsigned long datetime; // UnixTimeStamp
} 
nfc_id;

struct nfc_t_tmp   
{
  double lat,lng;                // GPS Coordinates     
  byte nfcid[10];                // NFC ID
  unsigned long datetime; // UnixTimeStamp
} 
nfc_id_tmp;

void setup() {

  SPI.begin();
  Wire.begin();
  
  eeprom.begin();

  //configuration.ee_pos = 0;
  //configuration.last_knopka = 2;
  //configuration.card_count = 1;  
  //EEPROM_writeAnything(0, configuration);

  EEPROM_readAnything(0, configuration);

  knopka = configuration.last_knopka;
  card_count = configuration.card_count;

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

  if (DEBUG) {
    bt.print("Knopka: "); 
    bt.println(knopka);
    bt.print("Card Count: "); 
    bt.println(card_count);
    bt.println(eeprom.determineSize());
  }


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
    print_time();

  }    

  delay(500);

  if (knopka != 1 && knopka != 2 && knopka != 3) { // Если ошибка записи в EEPROM
    knopka = 1;
    configuration.last_knopka = 1;
    EEPROM_writeAnything(0, configuration); 
  } 

}

void loop() {

  unsigned long currentMillis = millis();

  if (isTime(&SetgpsTimeMark,SetgpsTimeInterval)) 
    set_GPS_DateTime();

  if (knopka == 2 || knopka == 22) {
    get_nfc();
  }

  if (knopka == 3) {
    ss.listen();
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
        lcd.print(gps.location.lat(), 3); // Lat
        lcd.print(F(" "));
        lcd.print(gps.location.lng(), 3); // Lng
        lcd.setCursor(0,0);               
        lcd.print(F("GPS: "));    
        lcd.print(gps.time.hour()); 
        lcd.print(":");
        lcd.print(gps.time.minute()); 
        lcd.print(":"); 
        lcd.print(gps.time.second()); 
      }  
      else {               
        lcd.setCursor(0,1);
        for(byte x=0;x<16;x++) lcd.print(F(" "));
        lcd.setCursor(0,1); 
        lcd.print(gps.charsProcessed());
      }
    }
  }

  if (knopka == 1) {
    bt.listen();
    knopka = 11;
  }

  if  (knopka == 11) {
    if( currentMillis - previousMillis > 1000) {
      previousMillis = currentMillis;      
      print_time();
    }
  }

  if (bt.available() > 0) {
    byte incoming = bt.read();
    bt.println("ok");
    menu( incoming );   

  }
  // ----------- Чтение значение кнопок .--------------------------------

  ADCvalue = analogRead(7);

  if (ADCvalue > 719 && ADCvalue < 723) {
    knopka = 1;
    configuration.last_knopka = 1;
    EEPROM_writeAnything(0, configuration);
  }

  if (ADCvalue > 656 && ADCvalue < 660) {
    knopka = 2;
    configuration.last_knopka = 2;
    EEPROM_writeAnything(0, configuration);
  }

  if (ADCvalue > 628 && ADCvalue < 632) {
    knopka =3;
    configuration.last_knopka = 3;
    EEPROM_writeAnything(0, configuration);    
  }

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

        for(byte i = 0; i < 10;i++) nfc_id.nfcid[i]=0;
        for (byte i = 0; i < mfrc522.uid.size; i++)
          nfc_id.nfcid[i] = mfrc522.uid.uidByte[i];

        save_nfc_id();

        lcd.setCursor(0,1);
        lcd.print(F("Запись"));
        for (byte x=6;x<16;x++) {
          lcd.setCursor(x,1);
          lcd.print(F(".")); 
          delay(50);
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

// ---------------------------------- Выводим дату и время на дисплей ------------------------------

void print_time( void ) {

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

// ------------------------- Save NFC_ID ---------------------------------------------------

void save_nfc_id() {

  DateTime now = rtc.now();

  nfc_id.datetime = now.unixtime();

  if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid()) {
    nfc_id.lat = gps.location.lat();
    nfc_id.lng = gps.location.lng();
  } 
  else {
    nfc_id.lat = 0.0;
    nfc_id.lng = 0.0;
  }

  EEPROM_readAnything(0, configuration); // Чтения конфигурации

  unsigned int ee_pos = configuration.ee_pos;

  // configuration.card_count; // Номер записываемой карты (последняя картв это card_count-1)

  if (cmp_nfc_id() == false) {  // Если это новая карточка

    const byte* p = (const byte*)(const void*)&nfc_id;

    for (unsigned int i = 0; i < sizeof(nfc_id); i++) 
      eeprom.writeByte(ee_pos++, *p++);

    if (ee_pos > (EE24LC512MAXBYTES - (sizeof(nfc_id)+1) ) ) {
      configuration.ee_pos = 0;
      configuration.card_count = 1; // Пишем все с самого начала
    } 
    else {
      configuration.ee_pos = ee_pos; // Следующая ячейка памяти в EEPROM
      configuration.card_count++;      // Количества карточек увеличиваем.
    }

    EEPROM_writeAnything(0, configuration);

    if (DEBUG && configuration.ee_pos != 0) { 
      bt.print((configuration.card_count-1));
      bt.print(" ");
      bt.print((configuration.ee_pos-sizeof(nfc_id)));
      bt.print(" ");
      bt.print(nfc_id.datetime);
      bt.print(" ");
      DateTime eedt (nfc_id.datetime);
      showDate(eedt);
      bt.print(" ");
      for(byte i = 0; i < 10;i++) bt.print(nfc_id.nfcid[i],HEX);
      bt.println(); 
    }
  } 
  else {

    lcd.clear();
    lcd.print("Карта существует.");
    delay(2000);
  } // Если это новая карточка


}

boolean cmp_nfc_id() {

  boolean ret_val = true;

  EEPROM_readAnything(0, configuration); // Чтения конфигурации

  unsigned int address = 0;

  for(int p=0;p<configuration.card_count;p++) {

    byte* pp = (byte*)(void*)&nfc_id_tmp;

    for (unsigned int i = 0; i < sizeof(nfc_id_tmp); i++)
      *pp++ = eeprom.readByte(address++);

    ret_val = true;

    if (DEBUG) {
       
      //for(byte i = 0; i < 10;i++) bt.print(nfc_id.nfcid[i],HEX);
      //bt.println("");
      //for(byte i = 0; i < 10;i++) bt.print(nfc_id_tmp.nfcid[i],HEX);
      //bt.println("");
      
    }

    for(int a=0;a<10;a++) {
      if (nfc_id_tmp.nfcid[a] != nfc_id.nfcid[a]) { 
        ret_val = false; 
        break; 
      }
    }
    if (ret_val) return true;
  }

  return(ret_val);

}

void showDate(const DateTime& dt) {
  bt.print(dt.day(), DEC);
  bt.print('/');
  bt.print(dt.month(), DEC);
  bt.print('/');    
  bt.print(dt.year(), DEC);
  bt.print(" ");
  bt.print(dt.hour(), DEC);
  bt.print(':');
  bt.print(dt.minute(), DEC);
  bt.print(':');
  bt.print(dt.second(), DEC);
}

void menu(byte in) {

  bt.println("1111"); 

  if(in=='1') {
    lcd.clear();
  } 
}









/*
   Virtual Modem using ESP8266 (WIFI ESP MODEM)

   This program creates a virtual modem using the ESP8266 microcontroller.
   Parts of this program were generated with the assistance of ChatGPT, a
   large language model trained by OpenAI.

   This program is intended to be used with a corresponding set of
   schematics for the devices connecting the ESP8266. The code and
   schematics will be made public for others to use and modify.

   Author: Reinaldo Torres CoCoByte Club reyco2000@gmail.com
   Date:   March 7, 2023

   libraries used:
  - SoftwareSerial: allows serial communication on pins other than the default RX/TX pins
  - ESP8266WiFi: provides access to the WiFi functionalities of the ESP8266
  - EEPROM: allows data to be stored and retrieved from the EEPROM of the ESP8266
  - WiFiClient: provides access to the client-side network functionalities of the ESP8266

  Icons from: https://icons8.com/icons/set/Phone--white
  Oled bitmap convert tool : https://javl.github.io/image2cpp/

  This code is free and open source, and can be used and modified for any purpose.

               +-----------------+
               |    WIFIANTENA   |
       ADC0 A0 |                 |D0 GPIO16-WAKE  **** Switch goes here other pin on switch goes to GND
   Reserved RSV|  +-----------+  |D1 GPIO5-SCL    **** This is used for 4pin oled dispay
   Reserved RSV|  |  ESP8266  |  |D2 GPIO4-SDA    **** This is used for 4pin oled dispay
  SDD3-GPIO10 SD3|  |           |  |D3 GPIO0-FLASH
  SDD2-GPIO9 SD2|  |           |  |D4 GPIO2-TXD1-FLASH
  SDD1-MOSI-SD1|  |           |  |3.3V3
   SDCMD-CS CMD|  +-----------+  |GND
  SDD0-MISO SD0|                 |D5 GPIO14-SCLK        *** This is used on Spark integrated oled esp8622
  SDCLK-SCLK CLK|                 |D6 GPIO12-MISO        *** This is used on Spark integrated oled esp8622
            GND|                 |D7 GPIO13-MOSI-RXD2   **** RX This foes to RS232 TX
           3.3V|                 |D8 GPIO15-CS-TXD2     **** TX This goes to RS232 RX
             EN|                 |RX GPIO03-RXD0
            RST|                 |TX GPIO01-TDX0
            GND|                 |GND
            Vin|                 |3.3V
               |rst    USB  flash|
               +-----------------+


*/



//
// 3.5.2023 - Fixed duplicated init of serial port
// added first version of telnet client - this needs ajust to iterate and have an escape sequece - still in review
// Added the lastconfig ok feature to autonnect to wifi
// 3.9.2023 Fixed ident
// 3.10.2023 Added Oled Code
// 3.11.2023 added function to display on oled ip %of wifi, Added BaudSpeed to further implement speedchange
// 3.16.2023 Added documentation and header for the program
// 3.31.2023 Added Baud rate change, and change AT+A insted of S0
// 4.1.2023 Baud speed store to Eeprom tested - Removed pulsers and instead pin9 will be used as regular switch for auto-answer - To be implemented
// 4.3.2023 Recoded for U8g2 Library
// 4.5.2023 Added comments to add the AA switch to be implemented in FW 0.10
// 4.6.2023 Added a new logoWifi Modem, added AA toggle switch on pin GIO16 using pullup, added conditios to avoid going into AA if not previously connected to wifi
// 4.7.2023 Added a line for compatibility with Ingegred OLED and 4 PIN Oled using GPIO04 and GPIO04, also disabled Telnet client connected line to make it more compatible with BBS software
// 4.14.2023 Added AT+IP function and still working on AT+IPADDRESSET ( See line 564)

#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiClient.h>
//#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>



#define OLED_RESET -1   // Since Oled display only uses 4 pins reset should be set to -1
#define SCREENW 128    // using ssd1306 widht 128 pixels
#define SCREENH 64     // using ssd1306 height 64 pixels



#define SSID_SIZE 32
#define PASS_SIZE 32
#define LASTCONNECT 65
#define BAUDSPEED 66
#define rxPin 13
#define txPin 15
#define DEFAULTBAUDRATE 9600
#define FirmwareVersion "0.11"

//Pulser GPIO Pins
const int switchPinAA = 16; //AUTO ANSWER GPIO0

int ModemSpeed = 9600;


/////// ALL OBJECTS NEEDED ///////////////////////////////////////

// Object for telenet server to emulate the autoanswer function port 23 default telenet
WiFiServer TelnetServer(23);
// Object Serial port usage GPIO 13 RX , GPIO 15 TX
SoftwareSerial mySerial(rxPin, txPin); // RX, TX

/*

   ____  _          _   _______
  / __ \| |        | | |__   __|
  | |  | | | ___  __| |    | |_   _ _ __   ___
  | |  | | |/ _ \/ _` |    | | | | | '_ \ / _ \
  | |__| | |  __/ (_| |    | | |_| | |_) |  __/
  \____/|_|\___|\__,_|    |_|\__, | .__/ \___|
                              __/ | |
                             |___/|_|

*/
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 14, /* data=*/ 12, -1); //This is for the integrated OLED Dispay
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ OLED_RESET);  //Use this for the external Adafruit SSD1302 128x64 display



/*
   ____  _          _   _____
  / __ \| |        | | |_   _|
  | |  | | | ___  __| |   | |  ___ ___  _ __  ___
  | |  | | |/ _ \/ _` |   | | / __/ _ \| '_ \/ __|
  | |__| | |  __/ (_| |  _| || (_| (_) | | | \__ \
  \____/|_|\___|\__,_| |_____\___\___/|_| |_|___/

*/
//COCOBYTE LOGO Size 91x12 Pixels
const unsigned char bitmap_cocobyte [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x1b, 0xf0,
  0x01, 0x1b, 0xfe, 0xf9, 0x7c, 0xff, 0xfd, 0x03, 0xf8, 0x83, 0x3b, 0xf8, 0x83, 0x3b, 0xfe, 0xfb,
  0x7c, 0xff, 0xfd, 0x03, 0xfc, 0xc7, 0x7b, 0xfc, 0xc7, 0x7b, 0xfe, 0xfb, 0x7c, 0xff, 0xfd, 0x03,
  0xfe, 0xef, 0xfb, 0xfe, 0xef, 0xfb, 0xf0, 0xfb, 0x7f, 0xff, 0x7d, 0x00, 0xfe, 0xe1, 0xfb, 0xfe,
  0xe1, 0xfb, 0xfe, 0xfb, 0x7f, 0x7c, 0xfc, 0x03, 0x7e, 0xe0, 0xfb, 0x7e, 0xe0, 0xfb, 0xfe, 0xf1,
  0x3f, 0x7c, 0xfc, 0x03, 0xfe, 0xe1, 0xfb, 0xfe, 0xe1, 0xfb, 0xfe, 0xe3, 0x1f, 0x7c, 0xfc, 0x03,
  0xfe, 0xef, 0xfb, 0xfe, 0xef, 0xfb, 0xe0, 0xc3, 0x0f, 0x7c, 0x7c, 0x00, 0xfc, 0xc7, 0x7b, 0xfc,
  0xc7, 0x7b, 0xfe, 0xc3, 0x0f, 0x7c, 0xfc, 0x07, 0xf8, 0x83, 0x3b, 0xf8, 0x83, 0x3b, 0xfe, 0xc3,
  0x0f, 0x7c, 0xfc, 0x07, 0xf0, 0x01, 0x1b, 0xf0, 0x01, 0x1b, 0xfe, 0xc1, 0x0f, 0x7c, 0xfc, 0x07
};


// Phone aa 24x24
// 'phone-AA', 24x24px
const unsigned char bitmap_phone_AA [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x01, 0x20, 0x0c, 0x01, 0x10, 0x04, 0x01, 0x08, 0x04,
  0x02, 0x04, 0x04, 0x02, 0x02, 0x00, 0x02, 0x01, 0x00, 0xc1, 0x00, 0x84, 0xc0, 0x00, 0x44, 0xc0,
  0x0f, 0x44, 0x00, 0x00, 0x84, 0x00, 0x00, 0x88, 0x00, 0x00, 0x08, 0x01, 0x07, 0x10, 0x82, 0x38,
  0x30, 0x4c, 0x20, 0x60, 0x30, 0x20, 0xc0, 0x00, 0x20, 0x80, 0x01, 0x20, 0x00, 0x06, 0x10, 0x00,
  0x78, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 'nanoserial', 22x11px
const unsigned char bitmap_nanoserial [] PROGMEM = {
  0xfe, 0xff, 0x1f, 0x03, 0x00, 0x30, 0x01, 0x00, 0x20, 0xc1, 0xb6, 0x21, 0xc1, 0xb6, 0x21, 0x02,
  0x00, 0x20, 0xc2, 0xcc, 0x10, 0xc2, 0xcc, 0x10, 0x02, 0x00, 0x10, 0x04, 0x00, 0x08, 0xf8, 0xff,
  0x07
};

// 'icons8-connected-25', 25x25px
const unsigned char bitmap_connected [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0xd8, 0x00,
  0x00, 0x70, 0x6c, 0x00, 0x00, 0x88, 0x36, 0x00, 0x00, 0x05, 0x1b, 0x00, 0x00, 0x03, 0x0e, 0x00,
  0xc0, 0x06, 0x04, 0x00, 0x80, 0x0d, 0x08, 0x00, 0x40, 0x1b, 0x10, 0x00, 0x20, 0x36, 0x10, 0x00,
  0x10, 0x6c, 0x10, 0x00, 0x10, 0xd8, 0x08, 0x00, 0x10, 0xb0, 0x05, 0x00, 0x20, 0x60, 0x03, 0x00,
  0x40, 0xc0, 0x06, 0x00, 0xe0, 0x80, 0x01, 0x00, 0xb0, 0x41, 0x01, 0x00, 0xd8, 0x22, 0x00, 0x00,
  0x6c, 0x1c, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// 'WifiModem', 116x22px
const unsigned char bitmap_wifimodem [] PROGMEM = {
  0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x80, 0x07,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x80, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x38, 0x80, 0x03, 0x30, 0x00,
  0x06, 0xfc, 0x01, 0x1c, 0x0c, 0x00, 0x20, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x30, 0x00, 0x66,
  0xfc, 0x0d, 0x3c, 0x0e, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x60, 0x00, 0x66, 0x0c,
  0x0c, 0x3c, 0x0f, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x60, 0x08, 0x03, 0x3c, 0x00,
  0x7c, 0x0f, 0x00, 0x20, 0x00, 0x00, 0x00, 0x87, 0x3f, 0x1c, 0x60, 0x1c, 0x63, 0x3c, 0x0c, 0xec,
  0x0d, 0x0f, 0x3f, 0x3e, 0xff, 0x07, 0x8f, 0x3f, 0x1e, 0xc0, 0x3c, 0x63, 0x0c, 0x0c, 0xcc, 0x8d,
  0x19, 0x3b, 0x33, 0xf7, 0x0e, 0x87, 0x3f, 0x1c, 0xc0, 0x36, 0x63, 0x0c, 0x0c, 0x8c, 0xcc, 0xb0,
  0x31, 0x7f, 0x63, 0x0c, 0x80, 0x1f, 0x00, 0xc0, 0xe7, 0x61, 0x0c, 0x0c, 0x0c, 0xcc, 0xb0, 0x21,
  0x3f, 0x63, 0x0c, 0x00, 0x0f, 0x00, 0xc0, 0xe3, 0x61, 0x0c, 0x0c, 0x0c, 0xcc, 0xb0, 0x31, 0x03,
  0x63, 0x0c, 0x00, 0x00, 0x00, 0x80, 0xc3, 0x61, 0x0c, 0x0c, 0x0c, 0x8c, 0x1f, 0x3f, 0x16, 0x63,
  0x0c, 0x00, 0x00, 0x00, 0x80, 0xc1, 0x61, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x3e, 0x1c, 0x63, 0x0c,
  0x3c, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c,
  0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04,
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


char ssid[SSID_SIZE];
char password[PASS_SIZE];
char lastconnect;
int toggleswitch;
char BaudSpeedCharOnEeprom; //1 to 9 characters
int connected;
const int switchPinEnter = 2;
const int switchPinEscape = 10;


///////////////////
// Modem Speed
//////////////////
int Return_Speed(char Eeprom_char_speed)
{
  int modem_speed;
  switch (Eeprom_char_speed) {
    case '0':
      modem_speed = 300;
      break;
    case '1':
      modem_speed = 600;
      break;
    case '2':
      modem_speed = 1200;
      break;
    case '3':
      modem_speed = 2400;
      break;
    case '4':
      modem_speed = 9600;
      break;
    case '5':
      modem_speed = 19200;
      break;
    case '6':
      modem_speed = 57400;
      break;
    default:
      modem_speed = 9600;
      break;
  }
  return modem_speed;
}

// Return character corresponding to the given speed
char Eeprom_Speed_Char(int speed_to_be_stored)
{
  char eeprom_char_speed;
  switch (speed_to_be_stored) {
    case 300:
      eeprom_char_speed = '0';
      break;
    case 600:
      eeprom_char_speed = '1';
      break;
    case 1200:
      eeprom_char_speed = '2';
      break;
    case 2400:
      eeprom_char_speed = '3';
      break;
    case 9600:
      eeprom_char_speed = '4';
      break;
    case 19200:
      eeprom_char_speed = '5';
      break;
    case 57400:
      eeprom_char_speed = '6';
      break;
    default:
      eeprom_char_speed = '4';
      break;
  }
  return eeprom_char_speed;
}



/////////////////////////////
// Oled display functions
//////////////////////////////


void print_wifi_info_display()
{
  String texto, IP_Address;
  int rssi;
  rssi = WiFi.RSSI();
  int percentage = (rssi + 100) / 2;
  IP_Address = "IP:" + WiFi.localIP().toString();
  texto = "WiFi:" + String(percentage) + "%";
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 35, texto.c_str());
  u8g2.drawStr(0, 45, IP_Address.c_str());
  u8g2.sendBuffer();
}

void print_nowifi()
{
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0, 32, "Wifi Failed. Check");
  u8g2.sendBuffer();
}


void intro_logo()
{
  u8g2.clearBuffer();
  u8g2.drawXBMP(18, 0, 91, 12, bitmap_cocobyte);
  u8g2.drawXBMP(0, 21, 116, 22, bitmap_wifimodem);
  u8g2.sendBuffer();
  delay(3000);
}

void coco_logo()
{
  String texto;
  //display.clearDisplay();
  u8g2.drawXBMP(16, 0, 91, 12, bitmap_cocobyte);
  u8g2.drawXBMP(106, 53, 22, 11, bitmap_nanoserial);
  u8g2.setFont(u8g2_font_5x7_tr);
  texto.concat("Wifi Modem FW:");
  texto.concat(FirmwareVersion);
  u8g2.drawStr(18, 22, texto.c_str());
  texto = String(ModemSpeed) + "+8N1";
  u8g2.drawStr(46, 63, texto.c_str());
  u8g2.sendBuffer();
}

void auto_answer_logo()
{
  String IP_Address;
  u8g2.clearBuffer();
  u8g2.drawXBMP(52, 22, 24, 24, bitmap_phone_AA);
  IP_Address = "Local IP:" + WiFi.localIP().toString();
  u8g2.drawStr(0, 64, IP_Address.c_str());
  u8g2.setFont(u8g2_font_helvR08_tr);
  //Print waitting for caller inverted and centered on top
  int textWidth = u8g2.getUTF8Width("   Waiting for Caller   ");
  int xPos = (u8g2.getDisplayWidth() - textWidth) / 2;
  u8g2.drawButtonUTF8(xPos, 10, U8G2_BTN_INV, textWidth,  5,  2, "   Waiting for Caller   " );
  u8g2.sendBuffer();
}

void connected_logo()
{
  u8g2.clearBuffer();
  u8g2.drawXBMP(102, 39, 25, 25, bitmap_connected);
  u8g2.drawStr(25, 9, "CONNECTED");
  u8g2.sendBuffer();
}

void refresh_initial_screen()
{
  u8g2.clearBuffer();
  print_wifi_info_display();
  coco_logo();
}

/////////////////////////////////////////////////////////////////////////////////////
// All that is done when powered
/////////////////////////////////////////////////////////////////////////////////////
void setup() {
  u8g2.begin();
  intro_logo();
  lastconnect = 'F'; // Asume we have never succeed to connect to wifi
  // Set switch pins as inputs with pull-up resistors
  pinMode(switchPinAA, INPUT_PULLUP);

  //ModemSpeed=DEFAULTBAUDRATE; line enabled for debugging
  ///// FETCH EEPROM DATA
  EEPROM.begin(256); // initialize EEPROM with 256 bytes of storage
  readConfigFromEEPROM(); // read stored SSID , password and lastconnect and Spped from EEPROM
  ////////
  ModemSpeed = Return_Speed(BaudSpeedCharOnEeprom); // Convert char of speed to modem speed
  mySerial.begin(ModemSpeed); // start the software serial communication
  toggleswitch = digitalRead(switchPinAA); // Read if toggle switch is on or off on startup (open off) closed on

  // Clear screen on serial port at start and print banner
  mySerial.print("\f"); // Print the "Ctrl L" character to clear the serial console
  mySerial.printf("WiFiModem fw %s\n\r", FirmwareVersion);
  mySerial.println("COCOBYTE CLUB(C)2023\n");
  //Check if lastconnect var is set from emprom with a previous wifi connect success to re-attempt connecting to wifi at startup
  if (lastconnect == 'O') {
    mySerial.println("WIFI CONFIG FOUND\n\rCONNECTING");
    connectToWiFi();
  } else {
    mySerial.println("NO WIFI CONFIG FOUND");
    mySerial.println("ADJUST SSID-PASS");
  }
  // Print the AT+ Ready banner
  if (toggleswitch == LOW) mySerial.println("AUTO ANSWER MODE");
  mySerial.println("AT+ READY");
  mySerial.println();
  // Initialize Oled display and display cocobyte logo
  u8g2.clearBuffer();
  coco_logo();
  // Check if connected to also display on oled IP Address and wifi streght
  if (lastconnect == 'O') {
    print_wifi_info_display();
  }
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function for the parser
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool validateIPAddress(String ipAddress) {
  int dotCount = 0;
  int numCount = 0;
  String num = "";
  for (int i = 0; i < ipAddress.length(); i++) {
    char c = ipAddress.charAt(i);
    if (c == '.') {
      dotCount++;
      if (numCount == 0 || numCount > 3) {
        return false;
      }
      num = "";
      numCount = 0;
    } else if (c >= '0' && c <= '9') {
      num += c;
      numCount++;
      if (num.toInt() > 255) {
        return false;
      }
    } else {
      return false;
    }
  }
  if (dotCount != 3 || numCount == 0 || numCount > 3) {
    return false;
  }
  return true;
}


void print_ip_details()
{
  // Getting the current IP address
  IPAddress ipAddress = WiFi.localIP();
  mySerial.print("IP Address: ");
  mySerial.println(ipAddress.toString());
  // Getting the current subnet mask
  IPAddress subnetMask = WiFi.subnetMask();
  mySerial.print("Subnet Mask: ");
  mySerial.println(subnetMask.toString());
  // Getting the current DNS
  IPAddress dns = WiFi.dnsIP();
  mySerial.print("DNS: ");
  mySerial.println(dns.toString());
  // Getting the current default gateway
  IPAddress gateway = WiFi.gatewayIP();
  mySerial.print("Default Gateway: ");
  mySerial.println(gateway.toString());
  // Getting the current Wi-Fi RSSI in percentage
  int rssi = WiFi.RSSI();
  int percentage = map(rssi, -100, -30, 0, 100);
  mySerial.print("Wi-Fi Strenght: ");
  mySerial.print(percentage);
  mySerial.println("%");
}


////////////////////////////////////////////////////////////////////////////////
// Main Loop event waitting for AT+COMMANDS
////////////////////////////////////////////////////////////////////////////////
void loop() {

  // Need to add here the if condition to check if AA is on and go stright into AA mode also need to check if lastconnect is O since we dont want to connect if no wifi
  if ((toggleswitch == LOW) && (lastconnect == 'O') ) {
    auto_answer_logo();
    RunTelnetServer();
  }
  else {
    static String command = ""; // define a static variable to hold the command string
    if (mySerial.available()) {
      char c = mySerial.read();
      mySerial.print(c);
      if (c == '\r') { // end of command detected
        mySerial.println();
        command.trim();
        if (command.startsWith("AT+")) {
          //
          // Handle the AT commands
          if (command.startsWith("AT+WIFICONNECT")) {
            //mySerial.println("Connecting to WIFI");
            connectToWiFi();
          } else if (command.startsWith("AT+DT")) {
            String destinationIP;
            int port = 23;
            int colonIndex = command.indexOf(':');
            if (colonIndex >= 0) {
              destinationIP = command.substring(5, colonIndex);
              port = command.substring(colonIndex + 1).toInt();
            } else {
              destinationIP = command.substring(5);
            }
            mySerial.println("Telnet to " + destinationIP + " Port: " + String(port));

            //Need to add a condition to check if lastconnect is O we dont want to dial if no wifi connection.
            telnetClient(destinationIP, port);
          } else if (command.startsWith("AT+SSID")) {
            if (command.length() == 7) {
              mySerial.println();
              mySerial.println("Current SSID: " + String(ssid));
            } else {
              String newSSID = command.substring(7);
              mySerial.println("Setting new SSID: " + newSSID);
              setSSID(newSSID);
              storeConfigToEEPROM();
              strcpy(ssid, newSSID.c_str()); // update the ssid variable with the new SSID string

            }
          } else if (command.startsWith("AT+PASSWORD")) {
            if (command.length() == 11) {
              mySerial.println();
              mySerial.println("Current password: " + String(password));
            } else {
              String newPassword = command.substring(11);
              mySerial.println("Setting new password: " + newPassword);
              setPassword(newPassword);
              storeConfigToEEPROM();
              strcpy(password, newPassword.c_str()); // update the password variable with the new Password string
            }
          } else if (command.startsWith("AT+BAUDSPEED")) {
            if (command.length() == 12) {
              // Empty command, return ModemSpeed variable
              mySerial.printf("Current Speed on EEprom: %i\n\r" , Return_Speed(BaudSpeedCharOnEeprom));
            } else {
              String baudSpeed = command.substring(12);
              int newBaud = baudSpeed.toInt();
              if (newBaud == 0) {
                mySerial.println("Invalid Baud Speed. Valid options: 300, 600, 1200, 2400, 9600, 19200, 57400");
              } else if (newBaud == 300 || newBaud == 600 || newBaud == 1200 || newBaud == 2400 || newBaud == 9600 || newBaud == 19200 || newBaud == 57400) {
                mySerial.println("Setting new Speed: " + baudSpeed);
                ModemSpeed = newBaud; // update the ModemSpeed variable with the new baud speed
                //
                BaudSpeedCharOnEeprom = Eeprom_Speed_Char(newBaud); // Convert speed to character for eeprom update
                storeLastBaudSpeedToEEPROM(); // Commit new speed to eeprom for next reboot
                // Warn user that Modem will restart
                //
                mySerial.println("Modem Will Restart!");
                delay(3000);
                mySerial.println("Restarting...\n\rAdjust Speed to new Speed");
                ESP.restart(); //Restart the ESP with new speed configuration.
              } else {
                mySerial.println("Invalid Baud Speed. Valid options: 300, 600, 1200, 2400, 9600, 19200, 57400");
              }
            }
          } else if (command.equals("AT+IP")) {
            print_ip_details();
          }  else if (command.startsWith("AT+A")) {
            if (command.length() == 4) {
              mySerial.println("Start AutoAnswer Mode");
              // Need to check if LAST connect is O we dont want AA if no wifi is setup
              auto_answer_logo();
              RunTelnetServer();
            }
          }  else if (command.startsWith("AT+IPADDRESSET")) {
            //IPAddressSet();
          }
          else if (command.startsWith("AT+NETCOMMIT")) {
            mySerial.println("Storing SSID and password to EEPROM");
            storeConfigToEEPROM();
          } else {
            mySerial.println("AT+ CMD ERROR");
          }
        }

        command = ""; // reset the command string for the next command
      } else {
        command += c; // append the received character to the command string
      }

    }
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setSSID(String newSSID) {
  newSSID.toCharArray(ssid, SSID_SIZE);
}

void setPassword(String newPassword) {
  newPassword.toCharArray(password, PASS_SIZE);
}

void connectToWiFi() {
  WiFi.disconnect(); // disconnect from WiFi in case we were previously connected
  mySerial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    mySerial.print(".");
    attempts++;
    if (attempts >= 30) {
      mySerial.println();
      mySerial.println("WiFi Failed.");
      mySerial.println("Adjust SSID and PASS.");
      lastconnect = 'F'; // set last connection to F (Failed) on eeprom to avoid reconnect on start
      storeLastConnectToEEPROM();
      return;
    }
  }
  mySerial.println("Connected to WiFi!");
  lastconnect = 'O';
  storeLastConnectToEEPROM();
  TelnetServer.begin();
  mySerial.println("LISTENING 23 OK");
}

void readConfigFromEEPROM() {
  EEPROM.get(0, ssid);
  EEPROM.get(SSID_SIZE, password);
  EEPROM.get(LASTCONNECT, lastconnect);
  EEPROM.get(BAUDSPEED, BaudSpeedCharOnEeprom);
}

void storeConfigToEEPROM() {
  EEPROM.put(0, ssid);
  EEPROM.put(SSID_SIZE, password);
  EEPROM.commit();
}

void storeLastConnectToEEPROM() {
  EEPROM.put(LASTCONNECT, lastconnect);
  EEPROM.commit();
}

void storeLastBaudSpeedToEEPROM() {
  EEPROM.put(BAUDSPEED, BaudSpeedCharOnEeprom);
  EEPROM.commit();
}

void telnetClient(String ip, uint16_t port) {
  WiFiClient client;
  if (client.connect(ip.c_str(), port)) { // connect to Telnet server
    mySerial.println("Connected to Telnet server!");
    while (client.connected()) {
      if (mySerial.available()) {
        client.write(mySerial.read());
      }
      if (client.available()) {
        mySerial.write(client.read());
      }
    }
    client.stop();
    mySerial.println("Disconnected from Telnet server.");
  } else {
    mySerial.println("Failed to connect to Telnet server.");
  }
}

void RunTelnetServer() {

  /// need to loop this function until the +++ - to be implemented
  mySerial.println("Waiting for Telnet client to connect...");
  while (!TelnetServer.hasClient()) {
    delay(1000);
  }
  //mySerial.println("Telnet client connected");
  connected_logo();
  WiFiClient TelnetClient = TelnetServer.available();
  mySerial.println("RING");
  if (TelnetClient) {
    mySerial.println("New client connected");
    TelnetClient.println("Welcome WIFI ESP Modem!");
    while (TelnetClient.connected()) {
      while (TelnetClient.available()) {
        char c = TelnetClient.read();
        mySerial.write(c);
      }
      while (mySerial.available()) {
        char c = mySerial.read();
        TelnetClient.write(c);
      }
    }
    TelnetClient.stop();
    mySerial.println("Client disconnected");
    refresh_initial_screen();
  }
}

# cocowifimodem
Another ESP8266 Wifi Modem for Retro Computer Aka Tandy Color Compuer

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
   Reserved RSV|  |  ESP8266  |  |D2 GPIO4-SDA    **** This is used for 4pin oled dispay SDD3-GPIO10 SD3|  |           |  |D3 GPIO0-FLASH
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

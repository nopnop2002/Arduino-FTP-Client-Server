# Arduino FTP Client & Server   

# Hardware requirement:   
Arduine ATMEGA2560   
W5100 Ethernet module   
SD Card module   

# Sofware requirement:    
SdFat Library   
https://github.com/jbeynon/sdfatlib   

You have to change SdFatConfig.h for Software SPI.   
 //#define USE_ARDUINO_SPI_LIBRARY 1   
 #define USE_ARDUINO_SPI_LIBRARY 0   

 //#define USE_SOFTWARE_SPI 0   
 #define USE_SOFTWARE_SPI 1   
 //uint8_t const SOFT_SPI_CS_PIN = 10;   
 uint8_t const SOFT_SPI_CS_PIN = 2;   
 //uint8_t const SOFT_SPI_MOSI_PIN = 11;   
 uint8_t const SOFT_SPI_MOSI_PIN = 3;   
 //uint8_t const SOFT_SPI_MISO_PIN = 12;   
 uint8_t const SOFT_SPI_MISO_PIN = 4;   
 //uint8_t const SOFT_SPI_SCK_PIN = 13;   
 uint8_t const SOFT_SPI_SCK_PIN = 5;   


# You have to change Ethernet library in ArduinoCore.   
Please read README.TXT   

# Wireing:   

|W5100|SD_CARD|MEGA2560
|-----|-------|--------|
|SS||Pin#10|
|MISO||Pin#50|
|MOSI||Pin#51|
|SCK||Pin#52|
|VCC||5V|
|GND||GND2|
||CS|Pin#2|
||MOSI|Pin#3|
||MISO|Pin#4|
||SCK|Pin#5|
||VCC|5V|
||GND|GND|

![ftp_server-1](https://user-images.githubusercontent.com/6020549/34322583-0f724814-e86e-11e7-91fe-b1f510ad5a88.jpg)

![ftp_server-2](https://user-images.githubusercontent.com/6020549/34322580-0350e202-e86e-11e7-8e88-e2d37c5aeac8.jpg)



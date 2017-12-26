# Arduino FTP Client & Server   

# Hardware requirement:   
Arduine ATMEGA2560   
W5100 Ethernet module   
SD Card module   

![ftp_server-1](https://user-images.githubusercontent.com/6020549/34323189-9db31f7e-e880-11e7-8403-bda8b8491158.JPG)

----

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

----

# Wireing:   

|W5100|SD_CARD|MEGA2560
|-----|-------|--------|
|SS||Pin#10|
|MISO||Pin#50|
|MOSI||Pin#51|
|SCK||Pin#52|
|VCC||5V|
|GND||GND|
||CS|Pin#2|
||MOSI|Pin#3|
||MISO|Pin#4|
||SCK|Pin#5|
||VCC|5V|
||GND|GND|

----

# CallBack Function with FTP Server:   

You can use CallBack Function with FTP Server.   
When put,delete,mkdir,rmdir,rename occurs, your CallBack function is called.   
You can know what occurred within FTP Server.   

void setCallBackDelete(void (*functionPointer)(void));   
void setCallBackStor(void (*functionPointer)(void));   
void setCallBackRmdir(void (*functionPointer)(void));   
void setCallBackMkdir(void (*functionPointer)(void));   
void setCallBackRename(void (*functionPointer)(void));   

----

Transfer speed is very slow.   

![ftp_server-2](https://user-images.githubusercontent.com/6020549/34323187-9a9dafac-e880-11e7-83fe-bc5e3dc44173.jpg)

![ftp_server-3](https://user-images.githubusercontent.com/6020549/34323186-96d9f74a-e880-11e7-9830-649f40bcce07.jpg)

/*
   FTP server for IDE v1.8.5 and w5100

   Port fron https://github.com/nailbuster/esp8266FTPServer

*/

#include <SPI.h>
#include <Ethernet.h>
#include <SdFat.h>      // https://github.com/jbeynon/sdfatlib
#include "FtpServer.h"

#define FTP_USER    "arduino"  // You can change
#define FTP_PASS    "mega"     // You can change

SdFat sd;
SdFile file;
FtpServer ftpSrv;

void setup(void){
//  byte mac[] = { 0x00, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
  byte myIP[] = { 192, 168, 10, 190 };

  // SD chip select pin
  uint8_t chipSelect = 2;
  
  Serial.begin(9600);
  Serial.print("SD begin....");
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    Serial.println("failed....");
    while(1) {}
  }
  Serial.println("ok....");
  sd.ls(&Serial,LS_DATE | LS_SIZE | LS_R);

  Serial.print("Ethernet begin....");
  Ethernet.begin(mac,myIP);
  Serial.println("ok....");
  Serial.print("localIP: ");
  Serial.println(Ethernet.localIP());
//  Serial.print("subnetMask: ");
//  Serial.println(Ethernet.subnetMask());
//  Serial.print("gatewayIP: ");
//  Serial.println(Ethernet.gatewayIP());
//  Serial.print("dnsServerIP: ");
//  Serial.println(Ethernet.dnsServerIP());

  // Initialize the FTP server
  ftpSrv.begin(FTP_USER, FTP_PASS);

}

void loop(void){
  ftpSrv.handleFTP();        //make sure in loop you call handleFTP()!!  
}


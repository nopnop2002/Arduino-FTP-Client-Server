 /*
   FTP passive client for IDE v1.8.5 and w5100

   Port from https://playground.arduino.cc/Code/FTP
   
*/
#include <SPI.h>
#include <Ethernet.h>
#include <SdFat.h>      // https://github.com/jbeynon/sdfatlib

#define FTP_PORT        21
#define FTP_SERVER      "192.168.10.112"  // You can change
#define FTP_USER        "orangepi"        // You can change
#define FTP_PASS        "orangepi"        // You can change
#define FTP_PUT         1
#define FTP_GET         2
#define RUNNING_LED     13

// this must be unique
byte mac[] = { 0x00, 0xA2, 0xDA, 0x00, 0x59, 0x67 };  

EthernetClient client;
EthernetClient dclient;

// SD chip select pin
const uint8_t chipSelect = 2;

SdFat sd;
SdFile file;

char outBuf[128];
char outCount;

// change fileName to your file (8.3 format!)
char fileName[13] = "test.txt";

void errorDisplay(char* buff) {
  int stat = 0;
  Serial.print("Error:");
  Serial.println(buff);
  while(1) {
    digitalWrite(RUNNING_LED,stat);
    stat = !stat;
    delay(100);
  }
}

void displayHelp() {
  Serial.println("\n\nReady. Press command");
  Serial.println(" g:Get from Remote File");
  Serial.println(" p:Put to Remote File");
  Serial.println(" r:Read Local File");
  Serial.println(" d:Delete Local File");
  Serial.println(" l:List Local File");
}

void setup()
{
  Serial.begin(9600);
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
    errorDisplay("Failed to begin SD Card");
  }
  Serial.println("sd begin OK");
  
  if (Ethernet.begin(mac) == 0) {
    errorDisplay("Failed to configure Ethernet using DHCP");
  }
  Serial.print("localIP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("subnetMask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("gatewayIP: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("dnsServerIP: ");
  Serial.println(Ethernet.dnsServerIP());
  displayHelp();
}

void loop()
{
  // change fileName to your file (8.3 format!)
  char fileName[13] = "test.txt";
  byte inChar;

  inChar = Serial.read();

  if(inChar == 'g') {
    if(doFTP(FTP_GET, fileName) ) Serial.println("FTP OK");
    else Serial.println("FTP FAIL");
    displayHelp();
  }

  if(inChar == 'p') {
    if(doFTP(FTP_PUT, fileName)) Serial.println("FTP OK");
    else Serial.println("FTP FAIL");
    displayHelp();
  }

  if(inChar == 'r') {
    Serial.println("\n\n");
    readFile(fileName);    
    displayHelp();
  }

  if(inChar == 'd') {
    sd.remove(fileName);
    Serial.println("*************************************");
    sd.ls(LS_DATE | LS_SIZE | LS_R);
    Serial.println("*************************************");
    displayHelp();
  }

  if(inChar == 'l') {
    Serial.println("*************************************");
    sd.ls(LS_DATE | LS_SIZE | LS_R);
    Serial.println("*************************************");
    displayHelp();
  }


}

byte doFTP(int request,char* fileName)
{
  if (request == FTP_PUT) {
    file.open(fileName,O_READ);
  } else {
    sd.remove(fileName);
    file.open(fileName,O_WRITE | O_CREAT | O_APPEND);
  }

  if(!file.isOpen())
  {
    Serial.println("File open fail");
    return 0;    
  }

  if (request == FTP_PUT) {  
    if(!file.seekSet(0))
    {
      Serial.println("File seekSet fail");
      file.close();
      return 0;    
    }
  }

  Serial.println("File opened");

//  if (client.connect(server,21)) {
  if (client.connect(FTP_SERVER,FTP_PORT)) {
    Serial.println("Command connected");
  } 
  else {
    file.close();
    Serial.println("Command connection failed");
    return 0;
  }

  if(!eRcv()) return 0;

  char buf[32];
  sprintf(buf,"USER %s",FTP_USER);
  client.println(buf);

  if(!eRcv()) return 0;
  sprintf(buf,"PASS %s",FTP_PASS);
  client.println(buf);

  if(!eRcv()) return 0;

  client.println("SYST");

  if(!eRcv()) return 0;

  client.println("Type I");

  if(!eRcv()) return 0;

  client.println("PASV");

  if(!eRcv()) return 0;

  char *tStr = strtok(outBuf,"(,");
  int array_pasv[6];
  for ( int i = 0; i < 6; i++) {
    tStr = strtok(NULL,"(,");
    array_pasv[i] = atoi(tStr);
    if(tStr == NULL)
    {
      Serial.println("Bad PASV Answer");    

    }
  }

  unsigned int hiPort,loPort;

  hiPort = array_pasv[4] << 8;
  loPort = array_pasv[5] & 255;

  Serial.print("Data port: ");
  hiPort = hiPort | loPort;
  Serial.println(hiPort);

//  if (dclient.connect(server,hiPort)) {
  if (dclient.connect(FTP_SERVER,hiPort)) {
    Serial.println("Data connected");
  } 
  else {
    Serial.println("Data connection failed");
    client.stop();
    file.close();
    return 0;
  }

  if (request == FTP_PUT) {
    client.print("STOR ");
    client.println(fileName);
  } else {
    client.print("RETR ");
    client.println(fileName);
  }

  if(!eRcv())
  {
    dclient.stop();
    return 0;
  }

  if (request == FTP_PUT) { 
    Serial.println("Writing");
  
    byte clientBuf[64];
    int clientCount = 0;
  
    while(file.available())
    {
      clientBuf[clientCount] = file.read();
      clientCount++;
  
      if(clientCount > 63)
      {
        dclient.write(clientBuf,64);
        clientCount = 0;
      }
    }
  
    if(clientCount > 0) dclient.write(clientBuf,clientCount);

  } else {
    while(dclient.connected())
    {
      while(dclient.available())
      {
        char c = dclient.read();
        file.write(c);      
//        Serial.write(c); 
      }
    }
  }

  dclient.stop();
  Serial.println("Data disconnected");

  if(!eRcv()) return 0;

  client.println("QUIT");

  if(!eRcv()) return 0;

  client.stop();
  Serial.println("Command disconnected");

  file.close();
  Serial.println("File closed");
  return 1;
}

byte eRcv()
{
  byte respCode;
  byte thisByte;

  while(!client.available()) delay(1);

  respCode = client.peek();

  outCount = 0;

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);

    if(outCount < 127)
    {
      outBuf[outCount] = thisByte;
      outCount++;      
      outBuf[outCount] = 0;
    }
  }

  if(respCode >= '4')
  {
    efail();
    return 0;  
  }

  return 1;
}


void efail()
{
  byte thisByte = 0;

  client.println("QUIT");

  while(!client.available()) delay(1);

  while(client.available())
  {  
    thisByte = client.read();    
    Serial.write(thisByte);
  }

  client.stop();
  Serial.println("Command disconnected");
  file.close();
  Serial.println("SD closed");
}

void readFile(char* fileName)
{
  int line = 0;
  int lflag;
  char c;
  
  file.open(fileName,O_READ);
  if(!file.isOpen()) {
    Serial.println("File open fail");
    return;    
  }
  lflag = 1;
  while(file.available()) {
//    Serial.write(file.read());
    c=file.read();
    if (lflag) {
      Serial.print("Line#");
      Serial.print(line++);
      Serial.print(">");
      lflag = 0;
    }
    Serial.write(c);
    if(c == 0x0a) lflag = 1;
  }

  file.close();
}


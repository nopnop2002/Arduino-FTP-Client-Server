
/*
 * FTP Serveur for ATMEGA2560
 * based on esp8266FTPServer by David Paiva
 * https://github.com/nailbuster/esp8266FTPServer
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 **                                                                            **
 **                       DEFINITIONS FOR FTP SERVER                           **
 **                                                                            **
 *******************************************************************************/

// Uncomment to print debugging info to console attached to ESP8266
//#define FTP_DEBUG

#ifndef FTP_SERVERESP_H
#define FTP_SERVERESP_H

#include <Ethernet.h>
#include <SdFat.h>      // https://github.com/jbeynon/sdfatlib

#define FTP_SERVER_VERSION "FTP-2017-12-24"

#define FTP_CTRL_PORT      21        // Command port on wich server is listening  
#define FTP_DATA_PORT_PASV 50009     // Data port in passive mode

#define FTP_TIME_OUT  5              // Disconnect client after 5 minutes of inactivity
#define FTP_CMD_SIZE  255 + 8        // max size of a command
#define FTP_CWD_SIZE  255 + 8        // max size of a directory name
#define FTP_FIL_SIZE  255            // max size of a file name
#define FTP_BUF_SIZE 1024 //512      // size of file buffer for read/write
//#define FTP_BUF_SIZE 2*1460 //512    // size of file buffer for read/write
#define BUILT_IN_LED  13             // ON Board LED port

extern SdFat sd;
extern SdFile file;

class FtpServer
{
public:
  void    begin(String uname, String pword);
  void    handleFTP();
  void    setCallBackDelete(void (*functionPointer)(void));
  void    setCallBackStor(void (*functionPointer)(void));
  void    setCallBackRmdir(void (*functionPointer)(void));
  void    setCallBackMkdir(void (*functionPointer)(void));
  void    setCallBackRename(void (*functionPointer)(void));

private:
  struct callbacks {
    short FunctionDelete;
    short FunctionStor;
    short FunctionRmdir;
    short FunctionMkdir;
    short FunctionRename;
    void (*pFunctionDelete)(const char *message);
    void (*pFunctionStor)(const char *message);
    void (*pFunctionRmdir)(const char *message);
    void (*pFunctionMkdir)(const char *message);
    void (*pFunctionRename)(const char *message1, const char *message2);
    } Callbacks;
 
  void    iniVariables();
  void    clientConnected();
  void    disconnectClient();
  boolean userIdentity();
  boolean userPassword();
  boolean processCommand();
  boolean dataConnect();
  boolean doRetrieve();
  boolean doStore();
  void    closeTransfer();
  void    abortTransfer();
  boolean makePath( char * fullname );
  boolean makePath( char * fullName, char * param );
  uint8_t getDateTime( uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                       uint8_t * phour, uint8_t * pminute, uint8_t * second );
  char *  makeDateTimeStr( char * tstr, uint16_t date, uint16_t time );
  int8_t  readChar();

  IPAddress      dataIp;              // IP address of client for data
  EthernetClient client;
  EthernetClient data;

  
  boolean  dataPassiveConn;
  uint16_t dataPort;
  char     buf[ FTP_BUF_SIZE ];       // data buffer for transfers
  char     cmdLine[ FTP_CMD_SIZE ];   // where to store incoming char from client
  char     cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char     command[ 5 ];              // command sent by client
  boolean  rnfrCmd;                   // previous command was RNFR
  char *   parameters;                // point to begin of parameters sent by client
  uint16_t iCL;                       // pointer to cmdLine next incoming char
  int8_t   cmdStatus,                 // status of ftp command connexion
           transferStatus;            // status of ftp data transfer
  uint32_t millisTimeOut,             // disconnect after 5 min of inactivity
           millisDelay,
           millisEndConnection,       // 
           millisBeginTrans,          // store time of beginning of a transaction
           bytesTransfered;           //
  String   _FTP_USER;
  String   _FTP_PASS;
  int16_t  eltm;
  

};

#endif // FTP_SERVERESP_H




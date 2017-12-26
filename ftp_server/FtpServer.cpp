/*
 * FTP Serveur for ATMEGA2560
 * based on esp8266FTPServerFTP by David Paiva
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

#include "FtpServer.h"

EthernetServer ftpServer( FTP_CTRL_PORT );
EthernetServer dataServer( FTP_DATA_PORT_PASV );

void FtpServer::begin(String uname, String pword)
{
//  sd.ls(&Serial,LS_DATE | LS_SIZE | LS_R);
  if (BUILT_IN_LED) {
    pinMode(BUILT_IN_LED,OUTPUT);
    digitalWrite(BUILT_IN_LED,LOW);
  }
  
  // Tells the ftp server to begin listening for incoming connection
  _FTP_USER = uname;
  _FTP_PASS = pword;

  ftpServer.begin();
  delay(10);
  dataServer.begin(); 
  delay(10);
  millisTimeOut = (uint32_t)FTP_TIME_OUT * 60 * 1000;
  millisDelay = 0;
  cmdStatus = 0;

  Callbacks.FunctionDelete = 0; // Disable CallBack
  Callbacks.FunctionStor = 0;   // Disable CallBack
  Callbacks.FunctionRmdir = 0;  // Disable CallBack
  Callbacks.FunctionMkdir = 0;  // Disable CallBack

  iniVariables();
}

void FtpServer::iniVariables()
{
  // Default for data port
  dataPort = FTP_DATA_PORT_PASV;
  
  // Default Data connection is Active
  dataPassiveConn = true;
  
  // Set the root directory
  strcpy( cwdName, "/" );

  rnfrCmd = false;
  transferStatus = 0;
}

void FtpServer::setCallBackDelete(void (*functionPointer)(void))
{
  Callbacks.FunctionDelete = 1; // Enable CallBack
  Callbacks.pFunctionDelete = (*functionPointer);
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionDelete=" + String(Callbacks.FunctionDelete));
  #endif
}

void FtpServer::setCallBackStor(void (*functionPointer)(void))
{
  Callbacks.FunctionStor = 1; // Enable CallBack
  Callbacks.pFunctionStor = (*functionPointer);
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionStor=" + String(Callbacks.FunctionStor));
  #endif
}

void FtpServer::setCallBackRmdir(void (*functionPointer)(void))
{
  Callbacks.FunctionRmdir = 1; // Enable CallBack
  Callbacks.pFunctionRmdir = (*functionPointer);
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionRmdir=" + String(Callbacks.FunctionRmdir));
  #endif
}

void FtpServer::setCallBackMkdir(void (*functionPointer)(void))
{
  Callbacks.FunctionMkdir = 1; // Enable CallBack
  Callbacks.pFunctionMkdir = (*functionPointer);
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionMkdir=" + String(Callbacks.FunctionMkdir));
  #endif
}

void FtpServer::setCallBackRename(void (*functionPointer)(void))
{
  Callbacks.FunctionRename = 1;
  Callbacks.pFunctionRename = (*functionPointer);
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionRename=" + String(Callbacks.FunctionRename));
  #endif
}


void FtpServer::handleFTP()
{
  if((int32_t) ( millisDelay - millis() ) > 0 )
    return;

#if 0
  if (ftpServer.hasClient()) {
    client.stop();
    client = ftpServer.available();
  }
#endif
  
  if( cmdStatus == 0 )
  {
    if( client.connected())
      disconnectClient();
    cmdStatus = 1;
  }
  else if( cmdStatus == 1 )         // Ftp server waiting for connection
  {
    abortTransfer();
    iniVariables();
    #ifdef FTP_DEBUG
    Serial.println("Ftp server waiting for connection on port "+ String(FTP_CTRL_PORT));
    #endif
    cmdStatus = 2;
  }
  else if( cmdStatus == 2 )         // Ftp server idle
  {
      
    client = ftpServer.connected();
    if( client > 0 )                // A client connected
    {
      clientConnected();      
      millisEndConnection = millis() + 10 * 1000 ; // wait client id during 10 s.
      cmdStatus = 3;
    }
  }
  else if( readChar() > 0 )         // got response
  {
    if( cmdStatus == 3 )            // Ftp server waiting for user identity
      if( userIdentity() )
        cmdStatus = 4;
      else
        cmdStatus = 0;
    else if( cmdStatus == 4 )       // Ftp server waiting for user registration
      if( userPassword() )
      {
        cmdStatus = 5;
        millisEndConnection = millis() + millisTimeOut;
      }
      else
        cmdStatus = 0;
    else if( cmdStatus == 5 )       // Ftp server waiting for user command
      if( ! processCommand())
        cmdStatus = 0;
      else
        millisEndConnection = millis() + millisTimeOut;
  }
  else if (!client.connected() || !client)
  {
    cmdStatus = 1;
    #ifdef FTP_DEBUG
    Serial.println("client disconnected");
    #endif
  }

  if( transferStatus == 1 )         // Retrieve data
  {
    if( ! doRetrieve())
      transferStatus = 0;
  }
  else if( transferStatus == 2 )    // Store data
  {
    if( ! doStore())
      transferStatus = 0;
  }
  else if( cmdStatus > 2 && ! ((int32_t) ( millisEndConnection - millis() ) > 0 ))
  {
    client.println("530 Timeout");
    millisDelay = millis() + 200;    // delay of 200 ms
    cmdStatus = 0;
  }
}

void FtpServer::clientConnected()
{
  #ifdef FTP_DEBUG
  Serial.println("Client connected!");
  #endif
  client.println( "220--- Welcome to FTP for atmega2560 ---");
  client.println( "220---   By nopnop2002   ---");
  client.println( "220 --   Version "+ String(FTP_SERVER_VERSION) +"   --");
  iCL = 0;
}

void FtpServer::disconnectClient()
{
  #ifdef FTP_DEBUG
  Serial.println("Disconnecting client");
  #endif
  abortTransfer();
  client.println("221 Goodbye.");
  client.stop();
}

boolean FtpServer::userIdentity()
{ 
  if( strcmp( command, "USER" ))
    client.println( "500 Syntax error");
  if( strcmp( parameters, _FTP_USER.c_str() ))
    client.println( "530 user not found");
  else
  {
    client.println( "331 OK. Password required");
    strcpy( cwdName, "/" );
    return true;
  }
  millisDelay = millis() + 100;  // delay of 100 ms
  return false;
}

boolean FtpServer::userPassword()
{
  if( strcmp( command, "PASS" ))
    client.println( "500 Syntax error");
  else if( strcmp( parameters, _FTP_PASS.c_str() ))
    client.println( "530 ");
  else
  {
    #ifdef FTP_DEBUG
    Serial.println( "OK. Waiting for commands.");
    #endif
    client.println( "230 OK.");
    return true;
  }
  millisDelay = millis() + 100;  // delay of 100 ms
  return false;
}

boolean FtpServer::processCommand()
{
  #ifdef FTP_DEBUG
  Serial.println("processCommand=" + String(command) + " parameters=" + String(parameters));
  #endif
  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  CDUP - Change to Parent Directory 
  //
  if( ! strcmp( command, "CDUP" ) )
  {
    char path[ FTP_CWD_SIZE ];
    if( makePath(path,"..")) {
      #ifdef FTP_DEBUG
      Serial.println("CWD path=" + String(path));
      #endif
      if (sd.chdir(path)) {
        strcpy(cwdName, path);
        client.println( "257 \"" + String(cwdName) + "\" is your current directory");
      } else {
        client.println( "550 No such file or directory" );
      }
    } else {
      client.println( "550 No such file or directory" );
    }
  }
  //
  //  CWD - Change Working Directory
  //
  else if( ! strcmp( command, "CWD" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strcmp( parameters, "." ) == 0 )  { // 'CWD .' is the same as PWD command
      client.println( "257 \"" + String(cwdName) + "\" is your current directory");
    } else {
      if( makePath(path)) {
        #ifdef FTP_DEBUG
        Serial.println("CWD path=" + String(path));
        #endif
        if (sd.chdir(path)) {
          strcpy(cwdName, path);
          client.println( "257 \"" + String(cwdName) + "\" is your current directory");
        } else {
          client.println( "550 " + String(parameters) + ": No such file or directory" );
        }
      } else {
        client.println( "550 " + String(parameters) + ": No such file or directory" );
      }
    }
    
  }
  //
  //  PWD - Print Directory
  //
  else if( ! strcmp( command, "PWD" ))
    client.println( "257 \"" + String(cwdName) + "\" is your current directory");
  //
  //  QUIT
  //
  else if( ! strcmp( command, "QUIT" ))
  {
    disconnectClient();
    return false;
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode 
  //
  else if( ! strcmp( command, "MODE" ))
  {
    if( ! strcmp( parameters, "S" ))
      client.println( "200 S Ok");
    // else if( ! strcmp( parameters, "B" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println( "504 Only S(tream) is suported");
  }
  //
  //  PASV - Passive Connection management
  //
  else if( ! strcmp( command, "PASV" ))
  {
    if (data.connected()) data.stop();
    //dataServer.begin();
    dataIp = Ethernet.localIP();    
    //dataIp = client.localIP();  
    dataPort = FTP_DATA_PORT_PASV;
    //data.connect( dataIp, dataPort );
    //data = dataServer.available();
    #ifdef FTP_DEBUG
    Serial.println("Connection management set to passive");
    Serial.println( "Data port set to " + String(dataPort));
    #endif
    client.println( "227 Entering Passive Mode ("+ String(dataIp[0]) + "," + String(dataIp[1])+","+ String(dataIp[2])+","+ String(dataIp[3])+","+String( dataPort >> 8 ) +","+String ( dataPort & 255 )+").");
    dataPassiveConn = true;
  }
  //
  //  PORT - Data Port
  //
  else if( ! strcmp( command, "PORT" ))
  {
  if (data) data.stop();
    // get IP of data client
    dataIp[ 0 ] = atoi( parameters );
    char * p = strchr( parameters, ',' );
    for( uint8_t i = 1; i < 4; i ++ )
    {
      dataIp[ i ] = atoi( ++ p );
      p = strchr( p, ',' );
    }
    // get port of data client
    dataPort = 256 * atoi( ++ p );
    p = strchr( p, ',' );
    dataPort += atoi( ++ p );
    if( p == NULL )
      client.println( "501 Can't interpret parameters");
    else
    {
      
    client.println("200 PORT command successful");
      dataPassiveConn = false;
    }
  }
  //
  //  STRU - File Structure
  //
  else if( ! strcmp( command, "STRU" ))
  {
    if( ! strcmp( parameters, "F" ))
      client.println( "200 F Ok");
    // else if( ! strcmp( parameters, "R" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println( "504 Only F(ile) is suported");
  }
  //
  //  TYPE - Data Type
  //
  else if( ! strcmp( command, "TYPE" ))
  {
    if( ! strcmp( parameters, "A" ))
      client.println( "200 TYPE is now ASII");
    else if( ! strcmp( parameters, "I" ))
      client.println( "200 TYPE is now 8-bit binary");
    else
      client.println( "504 Unknow TYPE");
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////
  //
  //  ABOR - Abort
  //
  else if( ! strcmp( command, "ABOR" ))
  {
    #ifdef FTP_DEBUG
    Serial.println("abort reques");
    #endif
    abortTransfer();
    client.println( "226 Data connection closed");
  }
  //
  //  DELE - Delete a File 
  //
  else if( ! strcmp( command, "DELE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client.println( "501 No file name");
    else if( makePath( path )) {
      if( ! sd.exists( path )) {
        client.println( "550 " + String(parameters) + ": No such file or directory");
      } else {
        file.open(path, O_READ);
        if (file.isFile()) {
          file.close();
          if( sd.remove( path )) {
            client.println( "250 Deleted " + String(parameters) );
            #ifdef FTP_DEBUG
            Serial.println("Callbacks.FunctionDelete=" + String(Callbacks.FunctionDelete));
            #endif
            if (Callbacks.FunctionDelete) Callbacks.pFunctionDelete(parameters);
          } else {
            client.println( "450 Can't delete " + String(parameters));
          }
        } else {
          file.close();
          client.println( "550 " + String(parameters) + ": Is a directory");
        }
      }
    }
  }
  //
  //  LIST - List 
  //
  else if( ! strcmp( command, "LIST" ) ||  ! strcmp( command, "NLST" ) )
  {
    if( ! dataConnect())
      client.println( "425 No data connection");
    else
    {
      int type = 0;
      if (! strcmp( command, "LIST" ) ) type = 1;
      Serial.println("type=" + String(type));
      client.println( "150 Accepted data connection");
      uint16_t nm = 0;

      uint32_t fsize;
      char buf[10];
      sd.chdir( cwdName );
      while (file.openNext(sd.vwd(), O_READ)) {
        if (file.isDir()) {
          if (type) data.print("drw-r--r--   ");
        } else {
          if (type) data.print("-rw-r--r--   ");
        }
        if (type) data.print(_FTP_USER);
        if (type) data.print(" ");
        if (type) data.print(_FTP_USER);
        if (type) fsize = file.fileSize();
        if (type) sprintf(buf,"%9ld ",fsize);
        if (type) data.print(buf);
        if (type) file.printModifyDateTime(&data);
        if (type) data.print(" ");
        #ifdef FTP_DEBUG
        file.printName(&Serial);
        Serial.println("");
        #endif
        file.printName(&data);
        data.println("");
        file.close();
        nm++;
      }
      client.println( "226 " + String(nm) + " matches total");
      data.stop();
    }
  }
  //
  //  NOOP
  //
  else if( ! strcmp( command, "NOOP" ))
  {
    // dataPort = 0;
    client.println( "200 Zzz...");
  }
  //
  //  RETR - Retrieve
  //
  else if( ! strcmp( command, "RETR" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client.println( "501 No file name");
    else if( makePath( path ))
    {
      if( !sd.exists(path))
        client.println( "550 " + String(parameters) + ": No such file or directory");
      else {
        file.open(path,O_READ);
        if( !file.isOpen() )
          client.println( "450 Can't open " +String(parameters));
        else if( ! dataConnect())
          client.println( "425 No data connection");
        else
        {
          #ifdef FTP_DEBUG
          Serial.println("Sending " + String(parameters) + "fiseSize=" + String(file.fileSize()));
          #endif
          client.println( "150-Connected to port "+ String(dataPort));
          client.println( "150 " + String(file.fileSize()) + " bytes to download");
          millisBeginTrans = millis();
          bytesTransfered = 0;
          transferStatus = 1;
        }
      }
    }
  }
  //
  //  STOR - Store
  //
  else if( ! strcmp( command, "STOR" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 )
      client.println( "501 No file name");
    else if( makePath( path ))
    {
      file.open(path, O_WRITE | O_CREAT | O_APPEND);
      if( !file.isOpen())
        client.println( "451 Can't open/create " +String(parameters) );
      else if( ! dataConnect())
      {
        client.println( "425 No data connection");
        file.close();
      }
      else
      {
        #ifdef FTP_DEBUG
        Serial.println( "Receiving " +String(parameters));
        #endif
        client.println( "150 Connected to port " + String(dataPort));
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 2;
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if( ! strcmp( command, "MKD" ))
  {
    if (sd.exists(parameters)) {
      client.println( "550 " + String(parameters)+ ": File exists");
    } else {
      if (sd.mkdir(parameters) ) {
        if (strcmp(cwdName, "/") == 0) {
          client.println( "257 \"" + String(cwdName) + String(parameters) + "\" - Directory successfully created");
        } else {
          client.println( "257 \"" + String(cwdName) + "/" + String(parameters) + "\" - Directory successfully created");
        }      
        #ifdef FTP_DEBUG
        Serial.println("Callbacks.FunctionMkdir=" + String(Callbacks.FunctionMkdir));
        #endif
        if (Callbacks.FunctionMkdir) Callbacks.pFunctionMkdir(parameters);
      } else {  
        client.println( "550 " + String(parameters) + ": Invalid directory name");
      }
    }
  }
  //
  //  RMD - Remove a Directory 
  //
  else if( ! strcmp( command, "RMD" ))
  {
    if (!sd.exists(parameters)) {
      client.println( "550 " + String(parameters) + ": No such file or directory");
    } else {
      file.open(parameters, O_READ);
      if (file.isDir()) {
        file.close();
        if (sd.rmdir(parameters)) {
          client.println( "250 RMD command successful");
          #ifdef FTP_DEBUG
          Serial.println("Callbacks.FunctionRmdir=" + String(Callbacks.FunctionRmdir));
          #endif
          if (Callbacks.FunctionRmdir) Callbacks.pFunctionRmdir(parameters);
        } else {
          client.println( "550 " + String(parameters) + ": Directory not empty");
        }
      } else {
        file.close();
        client.println( "550 " + String(parameters) + ": Not a directory");
      }
    }
  
  }
  //
  //  RNFR - Rename From 
  //
  else if( ! strcmp( command, "RNFR" ))
  {
    buf[ 0 ] = 0;
    if( strlen( parameters ) == 0 )
      client.println( "501 No file name");
    else if( makePath( buf )) {
      #ifdef FTP_DEBUG
      Serial.println("Renaming " + String(buf));
      #endif
      if( ! sd.exists( buf ))
        client.println( "550 " + String(parameters) + ": No such file or directory");
      else
      {
        client.println( "350 RNFR accepted - File or directory exists, ready for destination");     
        rnfrCmd = true;
      }
    } else {
      #ifdef FTP_DEBUG
      Serial.println("makePath fail");
      #endif
    }
  }
  //
  //  RNTO - Rename To 
  //
  else if( ! strcmp( command, "RNTO" ))
  {  
    char path[ FTP_CWD_SIZE ];
    char dir[ FTP_FIL_SIZE ];
    if( strlen( buf ) == 0 || ! rnfrCmd )
      client.println( "503 Need RNFR before RNTO");
    else if( strlen( parameters ) == 0 )
      client.println( "501 No file name");
    else if( makePath( path ))
    {
      if( sd.exists( path ))
        client.println( "553 " +String(parameters)+ ": File exists");
      else
      {          
        #ifdef FTP_DEBUG
        Serial.println("Renaming " + String(buf) + " to " + String(path));
        #endif
        if( sd.rename( buf, path )) {
          client.println( "250 File successfully renamed or moved");
          #ifdef FTP_DEBUG
          Serial.println("Callbacks.FunctionRename=" + String(Callbacks.FunctionRename));
          #endif
          if (Callbacks.FunctionRename) Callbacks.pFunctionRename(buf, path);
        } else { 
          client.println( "451 Rename/move failure");
        }                         
      }
    }
    rnfrCmd = false;
  }

  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

#if 0
  //
  //  FEAT - New Features
  //
  else if( ! strcmp( command, "FEAT" ))
  {
    client.println( "211-Extensions suported:");
    client.println( " MLSD");
    client.println( "211 End.");
  }
  //
  //  MDTM - File Modification Time (see RFC 3659)
  //
  else if (!strcmp(command, "MDTM"))
  {
    client.println("550 Unable to retrieve time");
  }
#endif

  //
  //  SIZE - Size of the file
  //
  else if( ! strcmp( command, "SIZE" ))
  {
    char path[ FTP_CWD_SIZE ];
    if( strlen( parameters ) == 0 ) {
      client.println( "501 No file name");
    } else if( makePath( path )) {
      file.open(path, O_READ);
      if(!file.isOpen()) {
         client.println( "550 " + String(parameters) + ": No such file or directory" );
      } else {
        if (file.isFile()) {
          client.println( "213 " + String(file.fileSize()));
        } else {
          client.println( "450 " +String(parameters) + ": not a regular file" );
        }
        file.close();
      }
    }
  }
  //
  //  SYST - System command
  //
  else if( ! strcmp( command, "SYST" ))
  {
    client.println( "215 Arduino ATMEGA2560");
  }
  //
  //  Unrecognized commands ...
  //
  else
    client.println( "500 Unknow command");
  
  return true;
}

boolean FtpServer::dataConnect()
{
  #ifdef FTP_DEBUG
  Serial.println("dataConnect=" + String(data.connected()));
  #endif
  if( ! data.connected() ) {
    if( dataPassiveConn ) {
      #ifdef FTP_DEBUG
      Serial.println("dataConnect connect to PASSIVE");
      #endif
//      data = dataServer.available();
      data = dataServer.connected();
      #ifdef FTP_DEBUG
      Serial.println("dataConnect=" + String(data.connected()));
      #endif
    } else { 
      #ifdef FTP_DEBUG
      Serial.println("dataConnect connect to ACTIVE");
      #endif
      data.connect( dataIp, dataPort );
      #ifdef FTP_DEBUG
      Serial.println("dataConnect=" + String(data.connected()));
      #endif
    }
  }
//  return data;
  return data.connected();
}


boolean FtpServer::doRetrieve()
{
  int16_t nb = file.read( buf, FTP_BUF_SIZE );
  if( nb > 0 )
  {
    data.write( buf, nb );
    bytesTransfered += nb;
    eltm++;
    if (BUILT_IN_LED) digitalWrite(BUILT_IN_LED,(eltm % 2));
    #ifdef FTP_DEBUG
    if((eltm % 10) == 0) {
      Serial.print("<");
    }
    if (eltm == 500) {
      eltm = 0;
      Serial.println("");
    }
    #endif

    return true;
  }
  closeTransfer();
  return false;
}


boolean FtpServer::doStore()
{
  if( data.connected() )
  {
    // Avoid blocking by never reading more bytes than are available
    int navail = data.available();
    if (navail <= 0) return true;
    // And be sure not to overflow buf.
    if (navail > FTP_BUF_SIZE) navail = FTP_BUF_SIZE;
    int16_t nb = data.read((uint8_t*) buf, navail );
    // int16_t nb = data.readBytes((uint8_t*) buf, FTP_BUF_SIZE );
    if( nb > 0 )
    {
      // Serial.println( millis() << " " << nb << endl;
      file.write((uint8_t*) buf, nb );
      bytesTransfered += nb;
    }
    eltm++;
    if (BUILT_IN_LED) digitalWrite(BUILT_IN_LED,(eltm % 2));
    #ifdef FTP_DEBUG
    if((eltm % 10) == 0) {
      Serial.print(">");
    }
    if (eltm == 500) {
      eltm = 0;
      Serial.println("");
    }
    #endif
    return true;
  }
  closeTransfer();
  #ifdef FTP_DEBUG
  Serial.println("Callbacks.FunctionStor=" + String(Callbacks.FunctionStor));
  #endif
  if (Callbacks.FunctionStor) Callbacks.pFunctionStor(parameters);
  return false;
}

void FtpServer::closeTransfer()
{
  uint32_t deltaT = (int32_t) ( millis() - millisBeginTrans );
  if( deltaT > 0 && bytesTransfered > 0 )
  {
    client.println( "226-Transfer complete");
    client.println( "226 " + String(deltaT) + " ms, "+ String(bytesTransfered / deltaT) + " kbytes/s");
  }
  else
    client.println( "226 File successfully transferred");
  
  file.close();
  millisEndConnection = millis() + millisTimeOut;
  data.stop();
  eltm = 0;
  #ifdef FTP_DEBUG
  Serial.println("");
  Serial.println("closeTransfere elapsed time=" + String((millis() - millisBeginTrans)/1000) + " Sec.");
  #endif
  if (BUILT_IN_LED) digitalWrite(BUILT_IN_LED,LOW);
}

void FtpServer::abortTransfer()
{
  if( transferStatus > 0 )
  {
    file.close();
    data.stop(); 
    client.println( "426 Transfer aborted"  );
    #ifdef FTP_DEBUG
    Serial.println( "Transfer aborted!") ;
    #endif
  }
  transferStatus = 0;
  eltm = 0;
}

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, iCL and parameters pointers
//
//  return:
//    -2 if buffer cmdLine is full
//    -1 if line not completed
//     0 if empty line received
//    length of cmdLine (positive) if no empty line received 

int8_t FtpServer::readChar()
{
  int8_t rc = -1;

  if( client.available())
  {
    char c = client.read();
   // char c;
   // client.readBytes((uint8_t*) c, 1);
    #ifdef FTP_DEBUG
    Serial.print( c);
    #endif
    if( c == '\\' )
      c = '/';
    if( c != '\r' )
      if( c != '\n' )
      {
        if( iCL < FTP_CMD_SIZE )
          cmdLine[ iCL ++ ] = c;
        else
          rc = -2; //  Line too long
      }
      else
      {
        cmdLine[ iCL ] = 0;
        command[ 0 ] = 0;
        parameters = NULL;
        // empty line?
        if( iCL == 0 )
          rc = 0;
        else
        {
          rc = iCL;
          // search for space between command and parameters
          parameters = strchr( cmdLine, ' ' );
          if( parameters != NULL )
          {
            if( parameters - cmdLine > 4 )
              rc = -2; // Syntax error
            else
            {
              strncpy( command, cmdLine, parameters - cmdLine );
              command[ parameters - cmdLine ] = 0;
              
              while( * ( ++ parameters ) == ' ' )
                ;
            }
          }
          else if( strlen( cmdLine ) > 4 )
            rc = -2; // Syntax error.
          else
            strcpy( command, cmdLine );
          iCL = 0;
        }
      }
    if( rc > 0 )
      for( uint8_t i = 0 ; i < strlen( command ); i ++ )
        command[ i ] = toupper( command[ i ] );
    if( rc == -2 )
    {
      iCL = 0;
      client.println( "500 Syntax error");
    }
  }
  return rc;
}

// Make complete path/name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath( char * fullName )
{
  return makePath( fullName, parameters );
}

boolean FtpServer::makePath( char * fullName, char * param )
{
  int pos;

  if( param == NULL )
    param = parameters;

  // Root or empty?
  if( strcmp( param, "/" ) == 0 || strlen( param ) == 0 )
  {
    strcpy( fullName, "/" );
    return true;
  }

  // If absolute path, store this
  if( param[0] == '/' ) { // cd /dir1/dir2/dir3
    strcpy( fullName, param );
  // If primary path, search primary
  } else if( strcmp(param,"..") == 0) { // cd ..
    if (strcmp(cwdName,"/") == 0) return false;
    for(pos=strlen(cwdName)-1;pos>=0;pos--) {
      if (cwdName[pos] == '/') {
        strcpy(fullName,cwdName);
        fullName[pos+1] = 0;
        break;
      }
    }
  // If relative path, concatenate with current dir
  } else { // cd dir
    strcpy( fullName, cwdName );
    if( fullName[ strlen( fullName ) - 1 ] != '/' )
      strncat( fullName, "/", FTP_CWD_SIZE );
      strncat( fullName, param, FTP_CWD_SIZE );
  }

  // If ends with '/', remove it
  uint16_t strl = strlen( fullName ) - 1;
  if( fullName[ strl ] == '/' && strl > 1 )
    fullName[ strl ] = 0;
  if( strlen( fullName ) < FTP_CWD_SIZE )
    return true;

  client.println( "500 Command line too long");
  return false;
}

#if 0
// Calculate year, month, day, hour, minute and second
//   from first parameter sent by MDTM command (YYYYMMDDHHMMSS)
//
// parameters:
//   pyear, pmonth, pday, phour, pminute and psecond: pointer of
//     variables where to store data
//
// return:
//    0 if parameter is not YYYYMMDDHHMMSS
//    length of parameter + space

uint8_t FtpServer::getDateTime( uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                                uint8_t * phour, uint8_t * pminute, uint8_t * psecond )
{
  char dt[ 15 ];

  // Date/time are expressed as a 14 digits long string
  //   terminated by a space and followed by name of file
  if( strlen( parameters ) < 15 || parameters[ 14 ] != ' ' )
    return 0;
  for( uint8_t i = 0; i < 14; i++ )
    if( ! isdigit( parameters[ i ]))
      return 0;

  strncpy( dt, parameters, 14 );
  dt[ 14 ] = 0;
  * psecond = atoi( dt + 12 ); 
  dt[ 12 ] = 0;
  * pminute = atoi( dt + 10 );
  dt[ 10 ] = 0;
  * phour = atoi( dt + 8 );
  dt[ 8 ] = 0;
  * pday = atoi( dt + 6 );
  dt[ 6 ] = 0 ;
  * pmonth = atoi( dt + 4 );
  dt[ 4 ] = 0 ;
  * pyear = atoi( dt );
  return 15;
}


// Create string YYYYMMDDHHMMSS from date and time
//
// parameters:
//    date, time 
//    tstr: where to store the string. Must be at least 15 characters long
//
// return:
//    pointer to tstr

char * FtpServer::makeDateTimeStr( char * tstr, uint16_t date, uint16_t time )
{
  sprintf( tstr, "%04u%02u%02u%02u%02u%02u",
           (( date & 0xFE00 ) >> 9 ) + 1980, ( date & 0x01E0 ) >> 5, date & 0x001F,
           ( time & 0xF800 ) >> 11, ( time & 0x07E0 ) >> 5, ( time & 0x001F ) << 1 );            
  return tstr;
}
#endif

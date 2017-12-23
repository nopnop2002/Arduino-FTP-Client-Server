You have to change Ethernet library in ArduinoCore.

C:\Program Files\Arduino\libraries\Ethernet\src\EthernetServer.h
     In EthernetServer.h add:
           EthernetClient connected();

C:\Program Files\Arduino\libraries\Ethernet\src\EthernetServer.cpp
     In EthernetServer.cpp add:
           EthernetClient EthernetServer::connected()
           {
             accept();
             for( int sock = 0; sock < MAX_SOCK_NUM; sock++ )
               if( EthernetClass::_server_port[sock] == _port )
               {
                 EthernetClient client(sock);
                 if( client.status() == SnSR::ESTABLISHED ||
                     client.status() == SnSR::CLOSE_WAIT )
                   return client;
               }
             return EthernetClient(MAX_SOCK_NUM);
           }

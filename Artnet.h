/*The MIT License (MIT)

Copyright (c) 2014 Nathanaël Lécaudé
https://github.com/natcl/Artnet, http://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef ARTNET_H
#define ARTNET_H

#include <Arduino.h>

#if defined(ARDUINO_SAMD_ZERO)
    #include <WiFi101.h>
    #include <WiFiUdp.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <WiFiUdp.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <WiFiUdp.h>
#else
    #include <Ethernet.h>
    #include <EthernetUdp.h>
#endif

// UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_POLL_REPLY 0x2100
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 18

struct artnet_reply_s {
  uint8_t  id[8];
  uint16_t opCode;
  uint32_t ipAddress;
  uint16_t port;
  uint8_t  versInfoH;
  uint8_t  versInfoL;
  uint8_t  netSwitch;
  uint8_t  subSwitch;
  uint8_t  oemHi;
  uint8_t  oem;
  uint8_t  ubeaVersion;
  uint8_t  status1;
  uint8_t  estaManLo;
  uint8_t  estaManHi;
  uint8_t  shortName[18];
  uint8_t  longName[64];
  uint8_t  nodeReport[64];
  uint8_t  numPortsHi;
  uint8_t  numPortsLo;
  uint8_t  portTypes[4];//max of 4 ports per node
  uint8_t  goodInput[4];
  uint8_t  goodOutput[4];
  uint8_t  swIn[4];
  uint8_t  swOut[4];
  uint8_t  swVideo;
  uint8_t  swMacro;
  uint8_t  swRemote;
  uint8_t  spare1;
  uint8_t  spare2;
  uint8_t  spare3;
  uint8_t  style;
  uint8_t  mac[6];
  uint32_t bindIp;
  uint8_t  bindIndex;
  uint8_t  status2;
  uint8_t  filler[26];
} __attribute__((packed));

class Artnet
{
public:
  Artnet();

  void begin(byte mac[], byte ip[]);
  void begin();
  void setBroadcastAuto(IPAddress ip, IPAddress sn);
  void setBroadcast(byte bc[]);
  void setBroadcast(IPAddress bc);
  uint16_t read();
  void printPacketHeader();
  void printPacketContent();

  // Return a pointer to the start of the DMX data
  inline uint8_t* getDmxFrame(void)
  {
    return artnetPacket + ART_DMX_START;
  }

  inline uint16_t getOpcode(void)
  {
    return opcode;
  }

  inline uint8_t getSequence(void)
  {
    return sequence;
  }

  inline uint16_t getUniverse(void)
  {
    return incomingUniverse;
  }

  inline uint16_t getLength(void)
  {
    return dmxDataLength;
  }

  inline IPAddress getRemoteIP(void)
  {
    return remoteIP;
  }

  inline void setArtDmxCallback(void (*fptr)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP))
  {
    artDmxCallback = fptr;
  }

  inline void setArtSyncCallback(void (*fptr)(IPAddress remoteIP))
  {
    artSyncCallback = fptr;
  }

private:
  uint8_t  node_ip_address[4];
  #if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
    WiFiUDP Udp;
  #else
    EthernetUDP Udp;
  #endif
  struct artnet_reply_s ArtPollReply;
  int artReplyCount;


  uint8_t artnetPacket[MAX_BUFFER_ARTNET];
  uint16_t packetSize;
  IPAddress broadcast;
  uint16_t opcode;
  uint8_t sequence;
  uint16_t incomingUniverse;
  uint16_t dmxDataLength;
  IPAddress remoteIP;
  void (*artDmxCallback)(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP);
  void (*artSyncCallback)(IPAddress remoteIP);
};

#endif

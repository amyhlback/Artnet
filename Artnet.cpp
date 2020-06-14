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

#include <Artnet.h>

Artnet::Artnet() {}

void Artnet::begin(byte mac[], byte ip[])
{
  #if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
    Ethernet.begin(mac,ip);
  #endif

  begin();
}

void Artnet::begin()
{
  Udp.begin(ART_NET_PORT);


  #if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
    IPAddress node_ip_address = Ethernet.localIP();
  #else
    IPAddress node_ip_address = WiFi.localIP();
  #endif

  //The following block populates the constant fields of the ArtPollReply packet.
  sprintf((char *)ArtPollReply.id, "Art-Net");
  ArtPollReply.opCode     = ART_POLL_REPLY;
  ArtPollReply.ipAddress  = node_ip_address;
  ArtPollReply.port       =  ART_NET_PORT;
  ArtPollReply.versInfoH  = 1;
  ArtPollReply.versInfoL  = 0;
  ArtPollReply.netSwitch  = 0;
  ArtPollReply.subSwitch  = 0;
  ArtPollReply.oemHi      = 0;
  ArtPollReply.oem        = 0xFF; //0xFF = "OEM Unknown"
  ArtPollReply.ubeaVersion= 0;
  ArtPollReply.status1    = 0b11010000;
  ArtPollReply.estaManLo  = 0;
  ArtPollReply.estaManHi  = 0;
  memset(ArtPollReply.shortName, 0, 18);
  sprintf((char *)ArtPollReply.shortName, "artnet arduino");
  memset(ArtPollReply.longName, 0, 64);
  sprintf((char *)ArtPollReply.longName, "Art-Net -> Arduino Bridge");
  //ArtPollReply.nodeReport is set at runtime
  ArtPollReply.numPortsHi = 0;
  ArtPollReply.numPortsLo = 4;
  memset(ArtPollReply.portTypes,  0b10000000, 4);
  memset(ArtPollReply.goodInput,  0b00001000, 4);
  memset(ArtPollReply.goodOutput,  0b10000000, 4);
  uint8_t swIn[4]  = {0x00,0x01,0x02,0x03};
  uint8_t swOut[4] = {0x00,0x01,0x02,0x03};
  for(uint8_t i = 0; i < 4; i++)
  {
    ArtPollReply.swIn[i] = swIn[i];
    ArtPollReply.swOut[i] = swOut[i];
  }
  ArtPollReply.swVideo    = 0;
  ArtPollReply.swMacro    = 0;
  ArtPollReply.swRemote   = 0;
  ArtPollReply.spare1     = 0;
  ArtPollReply.spare2     = 0;
  ArtPollReply.spare3     = 0;
  ArtPollReply.style      = 0;
  #if !defined(ARDUINO_SAMD_ZERO) && !defined(ESP8266) && !defined(ESP32)
    Ethernet.MACAddress(ArtPollReply.mac);
  #else
    WiFi.macAdress(ArtPollReply.mac);
  #endif
  ArtPollReply.bindIp     = node_ip_address;
  ArtPollReply.bindIndex  = 1;
  ArtPollReply.status2    = 0b00001000;
  memset(ArtPollReply.filler, 0, 26);

  artReplyCount           = 0;
}

void Artnet::setBroadcastAuto(IPAddress ip, IPAddress sn)
{
  //Cast in uint 32 to use bitwise operation of DWORD
  uint32_t ip32 = ip;
  uint32_t sn32 = sn;

  //Find the broacast Address
  uint32_t bc = (ip32 & sn32) | (~sn32);

  //sets the broadcast address
  setBroadcast(IPAddress(bc));
}

void Artnet::setBroadcast(byte bc[])
{
  //sets the broadcast address
  broadcast = bc;
}
void Artnet::setBroadcast(IPAddress bc)
{
  //sets the broadcast address
  broadcast = bc;
}

uint16_t Artnet::read()
{
  packetSize = Udp.parsePacket();

  remoteIP = Udp.remoteIP();
  if (packetSize <= MAX_BUFFER_ARTNET && packetSize > 0)
  {
      Udp.read(artnetPacket, MAX_BUFFER_ARTNET);

      // Check that packetID is "Art-Net" else ignore
      for (byte i = 0 ; i < 8 ; i++)
      {
        if (artnetPacket[i] != ART_NET_ID[i])
          return 0;
      }

      opcode = artnetPacket[8] | artnetPacket[9] << 8;

      if (opcode == ART_DMX)
      {
        sequence = artnetPacket[12];
        incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;
        dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;

        if (artDmxCallback) (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, remoteIP);
        return ART_DMX;
      }
      if (opcode == ART_POLL)
      {
        //fill the reply struct, and then send it to the network's broadcast address
        Serial.print("POLL from ");
        Serial.print(remoteIP);
        Serial.print(" broadcast addr: ");
        Serial.println(broadcast);

        memset(ArtPollReply.nodeReport, 0, 64);
        if (artReplyCount > 9999) artReplyCount = 0;
        sprintf((char *)ArtPollReply.nodeReport, "#0001 [%04i] %i DMX output universes active.", artReplyCount, ArtPollReply.numPortsLo);
        artReplyCount++;

        Udp.beginPacket(remoteIP, ART_NET_PORT);//send the packet to the Controller that sent ArtPoll
        Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
        Udp.endPacket();

        //To fully implement the standard, state1, state2, goodoutput[0-3] should be
        //exposed in some way so that the flags can be set during runtime.

        return ART_POLL;
      }
      if (opcode == ART_SYNC)
      {
        if (artSyncCallback) (*artSyncCallback)(remoteIP);
        return ART_SYNC;
      }
  }
  else
  {
    return 0;
  }
  return 0;
}

void Artnet::printPacketHeader()
{
  Serial.print("packet size = ");
  Serial.print(packetSize);
  Serial.print("\topcode = ");
  Serial.print(opcode, HEX);
  Serial.print("\tuniverse number = ");
  Serial.print(incomingUniverse);
  Serial.print("\tdata length = ");
  Serial.print(dmxDataLength);
  Serial.print("\tsequence n0. = ");
  Serial.println(sequence);
}

void Artnet::printPacketContent()
{
  for (uint16_t i = ART_DMX_START ; i < dmxDataLength ; i++){
    Serial.print(artnetPacket[i], DEC);
    Serial.print("  ");
  }
  Serial.println('\n');
}

#include <UC1701.h>
#include <SPI.h>
#include <EthernetIndustruino.h>
#include <EthernetUdp.h>
#include <EEPROM.h>
#include <Time.h>




const int backlightPin = 13; // PWM output pin that the LED backlight is attached to
const int ctrl_port_a = 4;
const int ctrl_port_b = 5;


static UC1701 lcd;

EthernetUDP Udp;                     // A UDP instance to let us send and receive packets over UDP
EthernetClient client;
EthernetServer server(80);        // always at 80, despite serv_port value in cfg_struct

#define NTP_PACKET_SIZE 48           // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets


int backlightIntensity = 4;        // default LCD backlight intesity


struct cfg_struct {
  // This is for mere detection if they are your settings
  byte struct_version;
  byte switches;        //b0 = ntp-enabled
  byte mac_addr[6];
  byte ip_addr[4];
  byte netmask[4];
  byte dns[4];
  byte gw_addr[4];
  unsigned int serv_port;
  byte ntp_ip_addr[4];
  byte domoticz_ip_addr[4];
  unsigned int domoticz_port;
  byte domoticz_sw1_idx;
  byte domoticz_sw2_idx;
  byte lcd_backlight;
  byte lcd_timeout;

};

cfg_struct cfg;





void setup() {
  Serial.begin(115200);
  Serial.print("Remotecon setup started");

  pinMode(backlightPin, OUTPUT); //set backlight pin to output
  analogWrite(backlightPin, (map(backlightIntensity, 0, 5, 255, 0))); //convert backlight intesity from a value of 0-5 to a value of 0-255 for PWM.


  lcd.begin();
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("Remotecon v0.1");


  if (EEPROM.read(0) != 0x01) {
    lcd.setCursor(0, 7);
    lcd.print("Fatal error EEPROM");
  
  }
  else {
    //load config
    byte value;
    for (unsigned int i = 0; i < sizeof(cfg_struct); i++) {
      value = EEPROM.read(i);
      *((char*)&cfg + i) = value;
    }

    Serial.print("Config loaded");

  }

  Ethernet.begin(cfg.mac_addr, cfg.ip_addr, cfg.dns, cfg.gw_addr, cfg.netmask);
 
  Udp.begin(8888);
  // setup time synchronization, this is a call to the function that contacts the teim server
  // synchronize time every 7200 seconds (2 hours)

  setSyncProvider(getNTPtime);
  setSyncInterval(7200);
  
  //while(timeStatus()== timeNotSet)
  //  delay(100); // wait until the time is set by the sync provider

  Serial.print(cfg.serv_port);
  server = EthernetServer(cfg.serv_port);
  Serial.print("Ethernet server created");


  server.begin();

  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  Serial.print("Remotecon setup ended");

  lcd.setCursor(0,7);
  lcd.print(Ethernet.localIP());
  
}


void updateDisplay() {
  lcd.setCursor(0,6);
  lcd.print(hour());
  lcd.print(':');
  lcd.print(minute());
  lcd.print(':');
  lcd.print(second());


}

void loop() {
  // Just to show the program is alive...
  static int counter = 0;

  delay(200);
  updateDisplay();

  // Write a piece of text on the first line...
  //lcd.setCursor(0, 1);
  //lcd.print(Ethernet.localIP());
  // if an incoming client connects, there will be bytes available to read:
  client = server.available();
  if (client == true) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    Serial.print("Client available");
    // read bytes from the incoming client and write them back
    // to any clients connected to the server:
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin

          for (int analogChannel = 0; analogChannel < 6; analogChannel++) {
            int sensorReading = analogRead(analogChannel);
            client.print("analog input ");
            client.print(analogChannel);
            client.print(" is ");
            client.print(sensorReading);
            client.println("<br />");
          }
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
  }
}



/******************************************
 * send an NTP request to the time server 
 * at the given address 
 ******************************************/
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(byte *address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  //Udp.sendPacket( packetBuffer,NTP_PACKET_SIZE,  address, 123); //NTP requests are to port 123
} 







/******************************************
 * get the NTP time
 ******************************************/

unsigned long getNTPtime()   
{  
  Serial.println("Time synchronization starting");  
  //IPAddress timeServer(64,147,116,229); // time.nist.gov NTP server  
  sendNTPpacket((byte *)cfg.ntp_ip_addr); // send an NTP packet to a time server  
  delay(1000);   
  if ( Udp.parsePacket() ) {   
   // We've received a packet, read the data from it  
   Udp.read(packetBuffer,NTP_PACKET_SIZE); // read the packet into the buffer  

   //the timestamp starts at byte 40 of the received packet and is four bytes,  
   // or two words, long. First, esxtract the two words:  

   unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);  
   unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);   
   // combine the four bytes (two words) into a long integer  
   // this is NTP time (seconds since Jan 1 1900):  
   unsigned long secsSince1900 = highWord << 16 | lowWord;   
   Serial.print("Seconds since Jan 1 1900 = " );  
   Serial.println(secsSince1900);          

   // now convert NTP time into everyday time:  
   Serial.print("Unix time = ");  
   // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:  
   const unsigned long seventyYears = 2208988800UL;     
   // subtract seventy years:  
   unsigned long epoch = secsSince1900 - seventyYears;   

   //add correction for GMT+1  
   epoch = epoch + 3600;  
   //todo daylight saving time  

   // print Unix time:  
   Serial.println(epoch);                  
   return epoch;  
  }  
  return 0;  
}  



#include <UC1701.h>
#include <SPI.h>
#include <EthernetIndustruino.h>
#include <EEPROM.h>



const int backlightPin = 13; // PWM output pin that the LED backlight is attached to
const int ctrl_port_a = 4;
const int ctrl_port_b = 5;


static UC1701 lcd;
EthernetClient client;

EthernetServer server(80);        // always at 80, despite serv_port value in cfg_struct


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
  //while (!Serial) {
  //  ; // wait for serial port to connect. Needed for Leonardo only
  //}


  Serial.print("Remotecon setup started");
  /*pinMode(10, OUTPUT);     // change this to 53 on a mega
  pinMode(4, OUTPUT);     // change this to 53 on a mega
  pinMode(6, OUTPUT);     // change this to 53 on a mega
  digitalWrite(10, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(4, HIGH);
  delay(5000);
  */

  //backlightIntensity = 5; //0 to 5
  pinMode(backlightPin, OUTPUT); //set backlight pin to output
  analogWrite(backlightPin, (map(backlightIntensity, 0, 5, 255, 0))); //convert backlight intesity from a value of 0-5 to a value of 0-255 for PWM.


  lcd.begin();
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("Remotecon v0.1");


  if (EEPROM.read(0) != 0x01) {
    lcd.setCursor(0, 1);
    lcd.print("Fatal error:");
    lcd.setCursor(0, 2);
    lcd.print("EEPROM mismatch!");

  }
  else {
    //load config
    byte value;
    for (unsigned int i = 0; i < sizeof(cfg_struct); i++) {
      value = EEPROM.read(i);
      //Serial.print(value, HEX);
      *((char*)&cfg + i) = value;
    }

    //lcd.setCursor(0, 0);
    //lcd.print("Config loaded");
    Serial.print("Config loaded");

  }

  /*
  Serial.print("MAC: ");
  Serial.println(cfg.mac_addr);
  Serial.print("server is at ");
  Serial.println(cfg.ip_addr);
  */

  Ethernet.begin(cfg.mac_addr, cfg.ip_addr, cfg.dns, cfg.gw_addr, cfg.netmask);
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

void loop() {
  // Just to show the program is alive...
  static int counter = 0;

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

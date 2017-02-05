//#include "U8glib.h" //http://code.google.com/p/u8glib/
//#include "M2tk.h"   //http://code.google.com/p/m2tklib
//#include "utility/m2ghu8g.h"
#include <UC1701.h>
#include <SPI.h>
#include <EEPROM.h>

static UC1701 lcd;



const int backlightPin = 13; // PWM output pin that the LED backlight is attached to
int backlightIntensity = 5;        // LCD backlight intesity

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




void setup()
{
  cfg_struct cfg =
  {
    0x01,
    0x00,
    {0, 0, 0xde, 0xad, 0xbe, 0xef},
    {192,168,2,10},
    {255,255,255,0},
    {192,168,2,1},
    {192,168,2,1},
    80,
    {5,101,105,6},
    {192,168,2,7},
    8080,
    52,
    53,
    240,
    5
  };

  analogWrite(backlightPin, (map(backlightIntensity, 0, 5, 255, 0))); //map the value (from 0-5) to a corresponding PWM value (0-255) and update the output 
  lcd.begin();

  
  for (int i = 0; i < sizeof(cfg); i++) {
    EEPROM.write(i, *((char*)&cfg + i));
  }

  
  lcd.setCursor(0,0);
  lcd.print("OK");
  lcd.setCursor(0,1);
  lcd.print(sizeof(cfg));
  //lcd.setCursor(4,1);
  lcd.print(" bytes written");
   
}

void loop()
{
}




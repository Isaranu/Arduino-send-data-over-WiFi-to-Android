// <Project SP1 module> : Smart solar power management
// Applicable module
// 1. Arduino UNO R3 board
// 2. Humidity and Temperature sensor : DHT11
// 3. Graphics Display Panel : TFT_ILI9136C
// 4. WiFi shield : ESP8266
// 5. Current sensor : ACS712-05 (Ip = 5 Amps)

//Updated coding : 9.Sep.2015, Night

#include "DHT.h"
#include "uartWIFI.h"
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

#define SSID       "ISARANU" // SSID name for WiFi connection
#define PASSWORD   "4502160000" // Password

#define DHTPIN 2     // Pin connected on DHT11
#define DHTTYPE DHT11   // Model of DHT : DHT11

#define __CS 10 // Chip select pin on TFT
#define __DC 9 // Data send to TFT
#define __RST 8 // Reset pin on TFT

// Define color code for TFT display <Easy to use>
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

WIFI wifi;
TFT_ILI9163C tft = TFT_ILI9163C(__CS, __DC, __RST);
DHT dht(DHTPIN, DHTTYPE);

//Variable for DHT
// h=Humid %, t=Celcuis deg. and f=Farenheit
float h, t, f;
String hs, ht;

//Variable for WiFi
extern int chlID;  //client id(0-4)
String s;
const int led = 12;
char buf[100];
String Sbuf;

//Variable for TFT notification
String Notice;

//Variable for ACS712 current sensor
const int CurrentIn = A0;
float CurrentRead;
String CurrentS;

// ------------------- ## Set up ## -------------------------------------------
void setup() 

{
  //Serial.begin(9600);

  tft.begin();
  dht.begin();
  wifi.begin();

// -------------------- Start connecting WiFi --------------------------

  tft.setTextColor(WHITE);
  Notice = "Connecting WiFi...";
  tftpushnoti();
  
  bool b = wifi.Initialize(STA, SSID, PASSWORD);
  delay(8000);  //Give a time for WiFi shield to connecting.
  wifi.confMux(1);
  delay(200);
  
  if(wifi.confServer(1,8888))
  { 
    // When WiFi connected, LED will display 2 sec.
    digitalWrite(led,1);
    delay(2000);
    digitalWrite(led,0);
    
    s = wifi.showIP();
    s.remove(0,14);
    s.remove(11,40);

    tft.setTextColor(MAGENTA);
    Notice = "WiFi connected ! :)";
    tftpushnoti();
    delay(1000);
    
    tft.setTextColor(GREEN);
    Notice = "WiFi IP address : \n" + s;
    tftpushnoti();
    delay(1000);    

    pinMode(led,OUTPUT);
    digitalWrite(led,0);

    tft.setTextColor(CYAN);
    Notice = "Wait order from client ... ";
    tftpushnoti();
    
   }
   else 
   {
    Notice = "WiFi connecting failed !";
    tftpushnoti();
    }

}

// ---------------------- Main looping program -----------------------

void loop() 

{

// -------------- WiFi : Get order from client -----------------
  int iLen = wifi.ReceiveMessage(buf);
  if(iLen > 0)
  {
    //For control from browser
    Sbuf = String(buf);
    Sbuf.remove(8);
    
    if (Sbuf=="GET /ON ")
    {
        unsigned long takttime = millis();
        digitalWrite(led,1);
        wifi.Send(chlID,"\n\nUnder reading sensor data...");
        
        DHTsensor();
        ACS712();

        takttime = millis()-takttime;
        
        wifi.Send(chlID, "\nRH% = ");
        wifi.Send(chlID, hs);
        wifi.Send(chlID, " %");
        wifi.Send(chlID, "\nT = ");
        wifi.Send(chlID, ht);
        wifi.Send(chlID, " deg.C");
        wifi.Send(chlID, "\nCurrent (Is) = ");
        wifi.Send(chlID, CurrentS);
        wifi.Send(chlID, " mA");
        wifi.Send(chlID, "\n\n Sensor Conversion time  ");
        wifi.Send(chlID, String(takttime));
        wifi.Send(chlID, " ms");

        // Send all data display on TFT
        tftdisplay();
            
      }
      
      else if (Sbuf=="GET /OFF")
      {
        digitalWrite(led,0);
        wifi.Send(chlID,"\n\nStopped reading sensor data");
        wifi.Send(chlID, "\n Thank you sir :) ");

        // Send messege to TFT
        tft.setTextColor(WHITE);
        Notice = "Stop read sensor \n - End Session -";
        tftpushnoti();
      
      }
      wifi.Send(chlID,"\n- End session -");

    }
}

// ---------------------- Sub function ------------------------------

unsigned long tftpushnoti() 
{
  tft.fillScreen();
  unsigned long start = micros();
  tft.setCursor(0, 0);  
  tft.setTextSize(1);
  tft.print(Notice);
  tft.println();

  Notice = "";
  return micros() - start;
}

void DHTsensor()
{
  
  h = dht.readHumidity();
  hs = String(dht.readHumidity()); 
  // Read temperature as Celsius (the default)
  t = dht.readTemperature();
  ht = String(dht.readTemperature());
  // Read temperature as Fahrenheit (isFahrenheit = true)
  f = dht.readTemperature(true);

    // Takt time for sensor response = 2 sec.
  delay(2000);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) 
  {
    //Serial.println("Failed to read from DHT sensor!");
    Notice = "DHT11 sensor reading failed :(";
    tftpushnoti();
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);  
}

unsigned long tftdisplay() 
{
  tft.fillScreen();
  unsigned long start = micros();
  tft.setCursor(0, 0);
  
  tft.setTextColor(WHITE);  
  tft.setTextSize(1);
  tft.print("\nRH ");
  tft.print(h);
  tft.print(" %");
  
  tft.setTextColor(YELLOW);
  tft.print("\nTemp ");
  tft.print(t);
  tft.print(" deg C.");

  tft.setTextColor(GREEN);
  tft.print("\nCurrent(Is) : ");
  tft.print(CurrentRead);
  tft.print(" mA");

  tft.setTextColor(CYAN);
  tft.println("\n<Command> : ");
  tft.print(Sbuf);

  return micros() - start;
}

void ACS712()
{
    CurrentRead = analogRead(CurrentIn);
    CurrentRead = map(CurrentRead, 0,1023, 0, 5);

    // Formulas of ACS712 is Is = (Vout - 2.5)/0.2 A
    CurrentRead = ((CurrentRead - 2.5)/0.2);
    
    CurrentRead = CurrentRead*1000;   
    CurrentS = String(CurrentRead);
}

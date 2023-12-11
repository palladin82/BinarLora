#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <aes/esp_aes.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include "wifi_pass.h"


WebServer httpServer(80);
HTTPUpdateServer httpUpdater;
extern RTC_DATA_ATTR unsigned long startTimers[2];
extern RTC_DATA_ATTR wTIMER waketimer[2];
extern int tick;
extern Heater MyHeater;
extern ESP32Time rtc;

extern void fStart();
extern void fStop();

const char* host = "binar";
const char* ssid = STASSID;
const char* password = STAPSK;

int WiFiTimeOut=0;
bool WiFiClient=true;


void set_timer1()
{
  String hour = httpServer.arg(String("hour"));
  String min = httpServer.arg(String("min"));
  

  waketimer[0].hh = atoi(hour.c_str());
  waketimer[0].mm = atoi(min.c_str());
  
  Serial.printf("%02d %02d \r\n",waketimer[0].hh,waketimer[0].mm);

  httpServer.sendHeader("Location", "/timers.html",true);
  httpServer.send(302, "text/plain", "");
}

void send_timers()
{
  File indexfile = SPIFFS.open("/timers.html",FILE_READ);
  String content;

  if(!indexfile)
  {
    /*content = "<!DOCTYPE html><html><head><title>ESP32 Web Server</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">
    </head><body>  <h1>ESP32 Web Server</h1>  <p>Binar state: <strong> %STATE%</strong></p><p>Binar temp: <strong> %TEMP%</strong></p><p><a href=\"/on\"><button class=\"button\">ON</button></a></p>
    <p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p></body></html>";*/
  }
  else
  {
      content=indexfile.readString();
  }
 
  char temps[255];
  char batt[10];
  char chour[2];
  char cmin[2];
  //struct tm t;
  
  //t.tm_hour = ;
  //t.tm_min = ;

  sprintf(chour,"%02d",hour(waketimer[0].hh));
  sprintf(cmin,"%02d",minute(waketimer[0].mm));

  float battery=(float)MyHeater.Battery/10;  
  sprintf(batt,"%.2fВ",battery);
  sprintf(temps,"<b>%d°С</b><br>ERROR %d",MyHeater.temp1, MyHeater.Error);
  content.replace("%STATE%",MyHeater.GetStatus());
  content.replace("%TEMP%", temps);
  content.replace("%BATT%", batt);
  content.replace("%TIME%",rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  content.replace("%HH%",chour);
  content.replace("%MM%",cmin);
  httpServer.send(200,"text/html",content);
  
  tick=0;
}

void send_root()
{
  File indexfile = SPIFFS.open("/index.html",FILE_READ);
  String content;

  if(!indexfile)
  {
    /*content = "<!DOCTYPE html><html><head><title>ESP32 Web Server</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">
    </head><body>  <h1>ESP32 Web Server</h1>  <p>Binar state: <strong> %STATE%</strong></p><p>Binar temp: <strong> %TEMP%</strong></p><p><a href=\"/on\"><button class=\"button\">ON</button></a></p>
    <p><a href=\"/off\"><button class=\"button button2\">OFF</button></a></p></body></html>";*/
  }
  else
  {
      content=indexfile.readString();
  }
 
  char temps[255];
  char batt[10];
  //char time[64];
  //time = rtc.
  float battery=(float)MyHeater.Battery/10;  
  sprintf(batt,"%.2fВ",battery);
  sprintf(temps,"<b>%d°С</b><br>ERROR %d",MyHeater.temp1, MyHeater.Error);
  content.replace("%STATE%",MyHeater.GetStatus());
  content.replace("%TEMP%", temps);
  content.replace("%BATT%", batt);
  content.replace("%TIME%",rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  httpServer.send(200,"text/html",content);
  tick=0;
}

void start_binar()
{
  fStart();
  httpServer.sendHeader("Location", "/",true);
  httpServer.send(302, "text/plain", "");
}

void stop_binar()
{
  fStop();
  httpServer.sendHeader("Location", "/",true);
  httpServer.send(302, "text/plain", "");
}

void send_css()
{
  File cssfile = SPIFFS.open("/style.css",FILE_READ);
  String content;
  if(!cssfile)
  {
    return;
  }
  else
  {
    content=cssfile.readString();
    httpServer.send(200,"text/css",content);
  }
}

void send_back()
{
  File imgfile = SPIFFS.open("/img/back.jpg",FILE_READ);
  String content;
  if(!imgfile)
  {
    return;
  }
  else
  {
    content=imgfile.readString();
    httpServer.send(200,"image/jpeg",content);
  }

}


void networkInit()
{
    httpServer.on("/", HTTP_GET, send_root);
    httpServer.on("/timers.html", HTTP_GET, send_timers);
    httpServer.on("/set1", HTTP_GET, set_timer1);
    httpServer.on("/on", HTTP_GET, start_binar);
    httpServer.on("/off", HTTP_GET, stop_binar);
    httpServer.on("/style.css", HTTP_GET, send_css);
    httpServer.on("/img/back.jpg", HTTP_GET, send_back);

    httpUpdater.setup(&httpServer);
    httpServer.begin();

    WiFi.setHostname("Binar5SHOST");
    WiFi.begin(APSSID, APPSK);
    while (WiFi.status() != WL_CONNECTED) 
    {
        if(WiFiTimeOut>10)
        {
            WiFi.disconnect();
            if (!WiFi.softAP(ssid, password)) 
            {
                Serial.println("Soft AP creation failed.");
                while(1);
            }
            WiFiClient=false;
            break;
        }
        delay(200);
        Serial.println("Соединяемся c WiFi-сетью...");
        WiFiTimeOut++;
    }

    if(!WiFiClient)
    {
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);
        
        if (MDNS.begin(host)) 
        {
        Serial.println("mDNS responder started");
        }
    }
    else
    {
        IPAddress IP = WiFi.localIP();
        Serial.print("Local IP address: ");
        Serial.println(IP);
        
        if (MDNS.begin(host)) {
        Serial.println("mDNS responder started");
        }
    }
    MDNS.addService("http", "tcp", 80);
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
    WiFi.setTxPower(WIFI_POWER_18_5dBm);
}
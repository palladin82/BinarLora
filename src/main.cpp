/*
  LoRa Binar-5s communication

  created 21 January 2023
  by NikMarch
*/
#include <Arduino.h>
#include <SPI.h>              // include libraries


#include <OneButton.h>
#include <SimpleMenu.h>
#include <EEPROM.h>
#include <HardwareSerial.h>
#include <ESP32Time.h>
#include <esp_sntp.h>
#include <TimeLib.h>
#include <SPIFFS.h>
#include "planar.h"
#include "display.h"
#include "LoRaDriver.h"
#include "deepsleep.h"
#include "network.h"


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 60*60*3;
ESP32Time rtc(10800); //gmt+3



TaskHandle_t Task1;
void loop_task(void *pvParameters);



static const uint16_t screenWidth  = 480;
static const uint16_t screenHeight = 320;


#define BUTTON_PIN_BITMASK 0x000000001

int temp = 55;
unsigned int wakeflag=0;
int tick=0;
bool crcok=false;
bool pult=false;

Heater MyHeater;
S_PACKET data;

static const unsigned long REFRESH_INTERVAL = 1000; // ms
int timetosleep=600; //60*1 sec

int commandQueue[255];
int curCommand=0;

static unsigned long lastRefreshTime = 0;

#define PIN_INPUT 0
#define PIN_LED 2 // GIOP2 pin connected to button

OneButton button(PIN_INPUT, true);


HardwareSerial mySerial(1);
HardwareSerial mySerialTX(2);


/*
in file .platformio/packages/framework-arduinoesp.../cores/esp32/hardwareserial.cpp
line 360-360 and 368-369 set tx/rx as u see

#if SOC_UART_NUM > 1                   // may save some flash bytes...
            case UART_NUM_1:
               if (rxPin < 0 && txPin < 0) {
                    rxPin = 34;
                    txPin = 17;
                }
            break;
#endif
#if SOC_UART_NUM > 2                   // may save some flash bytes...
            case UART_NUM_2:
               if (rxPin < 0 && txPin < 0) {
                    rxPin = 39;
                    txPin = 23;


*/


// Variables will change:
int lastState = HIGH; // the previous state from the input pin
int currentState;
uint8_t debug=0;
int valueA = 1, valueB = 2, mainValue = 55;
bool sntp_time_set=false;

void fStart();
void fStop();
void exitF();
void sendStatus();
void fMessage();
void setBw();
void sendPing();
void sendWelcome();
void ToggleDebug();
void setSprd(int *param);
void setbaud(long unsigned baud);
void sntp_notification(timeval*);
void print_wakeup_reason();
void check_timers();
void set_timer1();

SimpleMenu* ShowAllNext(SimpleMenu *menu, char *buf);

S_PACKET ReadMySerial();



void setbaud(long unsigned baud)
{
  mySerial.updateBaudRate(baud);
  mySerialTX.updateBaudRate(baud);
}


void sntp_notification(timeval *timecb)
{
    sntp_time_set=true;
}


int menuMaxShow=3;

SimpleMenu MenuSubSprd[] = {
  SimpleMenu("..", exitF),
  SimpleMenu("OFF", setSprd, 0),
  SimpleMenu("Start", setSprd, 1),
  SimpleMenu("RUN", setSprd, 4),
  SimpleMenu("Shut", setSprd, 5)
  };



SimpleMenu Menu[] = {
  //SimpleMenu("Cur Temp",&mainValue),
  SimpleMenu("Start", fStart),
  SimpleMenu("Stop", fStop),
  SimpleMenu("Send-OK", sendStatus)  

};

SimpleMenu TopMenu(3, Menu);





void displayMenu(SimpleMenu *_menu)
{
  
  int menucnt = 10;
  SimpleMenu *next;
  SimpleMenu *prev;
  
  u8g2->setDrawColor(0);
  u8g2->drawBox(0, 0, 63, 10*menuMaxShow);



  u8g2->setDrawColor(1);
  u8g2->drawBox(0, 0, 634, 12);
  
  char buf[256];
  snprintf(buf, sizeof(buf), "%s", _menu->name);
  //snprintf(CurMenu, sizeof(CurMenu), "%s", _menu->name);
  u8g2->setDrawColor(0);
  u8g2->drawStr(0, menucnt, buf);
  u8g2->sendBuffer();

  //next=_menu->next();
  //next = TopMenu.next();

  for(int i=1;i<=menuMaxShow;i++)
  {
    next = TopMenu.next(i);
    if (next != NULL && next != prev)
    {
      menucnt += 10;
      u8g2->setDrawColor(1);
      char buf[256];
      snprintf(buf, sizeof(buf), "%s", next->name);
      u8g2->drawStr(0, menucnt, buf);
      u8g2->sendBuffer();
    }
    prev = next;

  }
  
}

SimpleMenu* ShowAllNext(SimpleMenu *menu, char *buf)
{
    static SimpleMenu *next=NULL;
    next = menu->next();
    
    //= &menu;

    

    if (next != NULL)
    {
        //displayMsg("menu ok");
        //snprintf(buf, sizeof(buf), "%s", next->name);
        //displayMsg(buf);
    }
    return next;
}




void setSprd(int *param)
{
  int index = * (int *) param;
  MyHeater.Status = uint8_t(index);
  if(index==5&&debug) Serial.println("OK!");
  //return true;
}


void ToggleDebug()
{
    

    if(debug)
    {
      debug=0;
    }
    else
    {
      debug=1;
    }
    
    EEPROM.write(0,debug);
    EEPROM.commit();
}









void setBw()
{

}

void fMessage()
{
    //while(1) sendMessage(" Ok");

}

void fGetstatus()
{
    //while(1) sendMessage(" Ok");

}

void fStart()
{
  char buf[255];
  char message[32];
  serialize(buf,&MyHeater.start);
  mySerialTX.write(buf, MyHeater.start.len+7);
  if(MyHeater.sync==false)sendHEX(0xF8);    
  sprintf(message,"Request START");
  displayMsg(message);
  
  sendStatus();
  if(debug)
  {
    Serial.println(message);
  }
}

void fStop()
{
    char buf[255];
    char message[32];
    serialize(buf,&MyHeater.shutdown);
    mySerialTX.write(buf, MyHeater.shutdown.len+7);
    if(MyHeater.sync==false)sendHEX(0xFC);
    sprintf(message,"Request STOP");
    displayMsg(message);

    sendStatus();
    if(debug)
    {
      Serial.println(message);
    }

  
}

void fSettings()
{
    char buf[255];
    serialize(buf,&MyHeater.settingsP);
    mySerialTX.write(buf, MyHeater.settingsP.len+7);
    
    if(debug)
    {
      displayMsg(print(&MyHeater.settingsP));
      Serial.println(print(&MyHeater.settingsP));
    }
}



void exitF()
{
    TopMenu.home();
}









void doubleClick()
{

  tick=0;
  TopMenu.up();
  
}

void Click()
{

  tick=0;
  TopMenu.down();
}

void longClick()
{

  tick=0;
  TopMenu.select();
}





void setup()
{
  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  Serial.print("Hello\r\n");
  mySerial.begin(2400);
  mySerialTX.begin(2400,SERIAL_8N1,-1,-1,true,20000UL,112);

  print_wakeup_reason();
  
  EEPROM.begin(256);
  
  debug = EEPROM.read(0);  
  pult = EEPROM.read(1);
  
  MyHeater.init();
  

  pinMode(PIN_LED, OUTPUT);
  button.attachDoubleClick(doubleClick);
  button.attachClick(Click);
  button.attachLongPressStart(longClick);

  init_oled();

  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    //return;
  }

  int count=0;
  unsigned long speed=0;
  
  
  LoRa_init();
  TopMenu.begin(displayMenu, displayValue);


  while(count < 1)
  {
    speed = 2400;
    //if(count==0) speed = 1200;
    if(count==1) speed = 2400;
    if(count==2) speed = 4800;
    if(count==3) speed = 9600;
    if(count==4) speed = 19200;
    if(count==5) speed = 38400;
    if(count==6) speed = 57600;
    if(count==7) speed = 115200;
    
    setbaud(speed);
   
    char tempmsg[64];
    sprintf(tempmsg,"try %d",speed);
    displayMsgS(tempmsg);


    mySerial.setTimeout(200);
  
    char tempStr[255];
    int timeout=0;
    
    while(timeout<50) 
    {
      sendPing();

      delay(30);  
      mySerial.readBytes(tempStr,1);
      
      if(tempStr[0]==0xaa)
      {
          break;  
      }
      timeout++;
      sprintf(tempmsg,"%d",timeout);
      displayMsgS1(tempmsg);
    }

    delay(100);
    tempStr[0]=mySerial.read();

    if(tempStr[0]==0x04)
    {
      displayMsgS("speed done!");
      sprintf(tempmsg,"speed %d",speed);
      
  
      MyHeater.sync=true;
      //read for foolresync
      int size=mySerial.read();
      mySerial.readBytes(tempStr,size);
      size=mySerial.read();
      size=mySerial.read();
      
      break;
    }

    count++;
  }

  mySerial.setTimeout(1000);
  
  

  //init WiFi and set RTC

 
  networkInit();
 
  delay(100);
  
  setenv("TZ","MSK-3",1);
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0,"pool.ntp.org");
  sntp_setservername(1,"time.nist.gov");
  sntp_set_time_sync_notification_cb(sntp_notification);
  sntp_init();
  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));

// end setup
}


void loop_task(void *pvParameters)
{
  
  for(;;)
  {
    if(mySerial.available())
    {
      memset(&data,0,sizeof(data));
      data=ReadMySerial();
      
      MyHeater.Planar_response(data);
      
      if(debug)
      {        
          Serial.println(print(&data));
      }
      
    }
    
    vTaskDelay(1);
  }
}



void loop()
{
  
  httpServer.handleClient();
  onReceive(LoRa.parsePacket());


  if(mySerial.available())
    {
      memset(&data,0,sizeof(data));
      data=ReadMySerial();
      
      MyHeater.Planar_response(data);
      
      if(debug)
      {        
          Serial.println(print(&data));
      }
      
    }


  if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
	{
    //Serial.print("loop() running on core ");
    //Serial.println(xPortGetCoreID());
    if(Serial.available()>0)
    {
        char temp;
        int speed=2400;
        temp=Serial.read();
        if(temp=='1')speed=1200;
        if(temp=='2')speed=2400;
        if(temp=='3')speed=4800;
        if(temp=='4')speed=9600;
        if(temp=='5')speed=57600;
        if(temp=='d')debug=1;
        if(temp=='+')fStart();
        if(temp=='-')fStop();
        if(temp=='s')fSettings();
        mySerial.end();
        mySerial.begin(speed);
        
        Serial.println(speed);
    }

		lastRefreshTime += REFRESH_INTERVAL;
    
    if(curCommand>0)
    {
      switch(commandQueue[curCommand])
      {
        case 0: sendPing();
                curCommand--;
              break;
        case 1: fStart();
                curCommand--;
              break;
        case 2: fStop();
                curCommand--;
              break;
        case 3: sendStatus();
                curCommand--;
              break;
      }
    }
    else sendPing();

   
    if(LoraBatt>0)MyHeater.Battery=LoraBatt;
    displayBat(MyHeater.Battery);
    if(LoraStatus>0)MyHeater.Status=LoraStatus;

    displayStatus(MyHeater.GetStatus());
    char temps[30];
    if(MyHeater.temp1>0||MyHeater.temp2||MyHeater.temp3) sprintf(temps,"%d %d %d LR=%d",MyHeater.temp1,MyHeater.temp2,MyHeater.temp3,MyHeater.Error);
    else sprintf(temps,"%d*",LoraTemp);\
    displayTemp(temps);
    
    if(MyHeater.temp1>79&&(MyHeater.Mode==2||MyHeater.Mode==3))
    {
      fStop();
    }

    if(tick > 0 && wakeflag > 0)
    {      
        tick=0;
        wakeflag=0;
    }
    else if(tick > timetosleep)
    {
      //need to go sleep
      goto_deepsleep();     
     
    }
    if(MyHeater.Status==1)tick++;
	}

  
  button.tick();

  if((millis() - lastReg.lasttime > RegCheckTime) && lastReg.lastgood)
  {
    LoRa.writeRegister(lastReg.regid,lastReg.value);
    lastReg.lastgood=false;
  }  


}



void sendPing()
{
  for(int i=0;i<=6;i++)
  {
    mySerialTX.write(char_ping[i]);  
  }
}


void sendWelcome()
{
  for(int i=0;i<=6;i++)
  {
    mySerialTX.write(char_welcome[i]);  
  }
}


void sendStatus()
{
  
    char status[10];
    status[0]=0xaa;
    unsigned long curtime = rtc.getLocalEpoch();
    
    unsigned long vIn = 0;
    

    if(MyHeater.sync)
    {
      status[1]=MyHeater.Battery;
      status[2]=MyHeater.temp1;
      status[3]=MyHeater.Status;
      status[4]=curtime         & 0xFF;
      status[5]=(curtime >>  8) & 0xFF;
      status[6]=(curtime >> 16) & 0xFF;
      status[7]=(curtime >> 24) & 0xFF;
      status[8]=sntp_time_set;
      
    }
    else
    {
      status[1]=random(-127,127);
      status[2]=random(-127,127);
      status[3]=MyHeater.Status;
      status[4]=curtime         & 0xFF;
      status[5]=(curtime >>  8) & 0xFF;
      status[6]=(curtime >> 16) & 0xFF;
      status[7]=(curtime >> 24) & 0xFF;
      status[8]=sntp_time_set;
      
    }
    sendMessage(status);
      
}

S_PACKET ReadMySerial()
{
  static S_PACKET temp;

  uint16_t crc;
  uint16_t c_crc;
  char reading[255];
  uint8_t packetSize;
  temp.preamble=0;
  temp.device=0;
  temp.len=0;
  temp.msg_id1=0;
  temp.msg_id2=0;
  temp.crc=0;

  if (mySerial.available())
  {

      temp.preamble=mySerial.read();
      
      if(debug)
      {
        Serial.write(temp.preamble);
      }

      if(temp.preamble==0xaa)
      {
          mySerial.readBytes(reading,1);

          if(debug)
          {
            Serial.write(reading[0]);
          }
          temp.device = reading[0];

          

          if(temp.device==0x04)
          {
              mySerial.readBytes(reading,1);
              temp.len = reading[0];

              if(debug)
              {
                Serial.write(reading[0]);
              }

              mySerial.readBytes(reading,2);
              temp.msg_id1 = reading[0];                
              temp.msg_id2 = reading[1];

              if(debug)
              {
                Serial.write(reading[0]);
                Serial.write(reading[1]);
              }

              if(temp.len>0)
              {
                mySerial.readBytes(temp.payload,temp.len);
                mySerial.readBytes(reading,2);
                temp.crc = (reading[0] << 8 ) | reading[1];
              }              
          }
          else
          {
              mySerial.readBytes(reading,1);
              temp.len = reading[0];

              if(debug)
              {
                Serial.write(reading[0]);
              }

              mySerial.readBytes(reading,2);
              temp.msg_id1 = reading[0];                
              temp.msg_id2 = reading[1];

              if(debug)
              {
                Serial.write(reading[0]);
                Serial.write(reading[1]);
              }

              if(temp.len>0)
              {
                mySerial.readBytes(temp.payload,temp.len);
                mySerial.readBytes(reading,2);
                temp.crc = (reading[0] << 8 ) | reading[1];
              }

          }                  
      }
  }

  packetSize = serialize(reading, &temp);

  c_crc=CRC16(reading,packetSize-2);


  if (temp.crc!=c_crc) 
  {
    char msgS1[64];
    sprintf(msgS1,"%d!=%d",temp.crc,c_crc);
    displayMsgS1(msgS1);
    temp.preamble=0;
    temp.device=0;
    temp.len=0;
    temp.msg_id1=0;
    temp.msg_id2=0;
    temp.crc=0;
  }
  else
  {
    displayMsgS1("CRC-OK");
    crcok=true;
  }
  displayX();

  return temp;
}



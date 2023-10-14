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
#include "planar.h"
#include <SoftwareSerial.h>
#include "display.h"
#include "LoRaDriver.h"
#define BUTTON_PIN_BITMASK 0x000000001

int temp = 55;
unsigned int wakeflag=0;
int tick=0;
int timetosleep=600; //60*1 sec
bool crcok=false;
bool pult=false;

Heater MyHeater;
S_PACKET data;

static const unsigned long REFRESH_INTERVAL = 1000; // ms
static unsigned long lastRefreshTime = 0;

#define PIN_INPUT 0
#define PIN_LED 2 // GIOP2 pin connected to button

OneButton button(PIN_INPUT, true);

SoftwareSerial mySerial(34, -1); // RX, TX
SoftwareSerial mySerialTX(-1, 23, true); //need to be inverted

// Variables will change:
int lastState = HIGH; // the previous state from the input pin
int currentState;
uint8_t debug=0;



//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);






int valueA = 1, valueB = 2, mainValue = 55;

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
void setbaud(int *baud);

SimpleMenu* ShowAllNext(SimpleMenu *menu, char *buf);

S_PACKET ReadMySerial();

//void displayMsg(String);
//void sendHEX(char outgoing);

//String intToString(int num);
//void init_oled();


//uint8_t setRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb);

void setbaud(int *baud)
{

  mySerial.end();
  mySerialTX.end();
  mySerial.begin(*baud);
  mySerialTX.begin(*baud);
  
}



int zero=0, one=1, four=4, five=5;
int menuMaxShow=5;

SimpleMenu MenuSubSprd[] = {
  SimpleMenu("..", exitF),
  SimpleMenu("OFF", setSprd, 0),
  SimpleMenu("Start", setSprd, 1),
  SimpleMenu("RUN", setSprd, 4),
  SimpleMenu("Shut", setSprd, 5),
  //SimpleMenu("Waiting", CallbackP(setSprd, &zero)),
  //SimpleMenu("Waiting", CallbackP(setSprd, &one)),
  //SimpleMenu("Waiting", CallbackP(setSprd, &four)),
  //SimpleMenu("Waiting", CallbackP(setSprd, &five))
  
};

SimpleMenu MenuSubBw[11] = {
  SimpleMenu("..", exitF),
  SimpleMenu("1200", setbaud,1200),
  SimpleMenu("2400", setbaud,2400),
  SimpleMenu("4800", setbaud,4800),
  SimpleMenu("9600", setbaud,9600),
  SimpleMenu("4800", setbaud,4800),
  SimpleMenu("41.7E3", setBw),
  SimpleMenu("62.5E3", setBw),
  SimpleMenu("125E3", setBw),
  SimpleMenu("250E3", setBw),
  SimpleMenu("500E3", setBw)
};

SimpleMenu MenuSub[] = {
  SimpleMenu("..", exitF),
  SimpleMenu("Set Status", 8, MenuSubSprd),
  SimpleMenu("Bandwith", 11, MenuSubBw),
  SimpleMenu("Toggle Debug", ToggleDebug)
};


SimpleMenu Menu[] = {
  //SimpleMenu("Cur Temp",&mainValue),
  SimpleMenu("Start", fStart),
  SimpleMenu("Stop", fStop),
  SimpleMenu("Send-OK", sendStatus),  
  SimpleMenu("Configure", 4, MenuSub)
};

SimpleMenu TopMenu(4, Menu);

void goto_deepsleep()
{
  
     

      if(debug)
      {
        Serial.println("going to sleep!");
        displayMsgS("going to sleep!");        
      }
      
      pinMode(14, INPUT_PULLUP);
      delay(2);


      gpio_hold_en((gpio_num_t)14);


      gpio_deep_sleep_hold_en();

      if(esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, HIGH)==ESP_OK) Serial.println("Lora sleep sleep configured");
      else Serial.println("GPIO_26 sleep ERR");
      
      if(esp_sleep_enable_ext1_wakeup(GPIO_SEL_0, ESP_EXT1_WAKEUP_ALL_LOW)==ESP_OK) Serial.println("GPIO_button sleep configured"); //BUTTON_PIN_BITMASK
      else Serial.println("GPIO_button sleep ERR");
      
      char tempSs[255];
      
      
      
      //ulp_start();
      esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
      
      LoRa.receive();
      esp_deep_sleep_start();


}



void displayMenu(SimpleMenu *_menu)
{
  
  int menucnt = 10;
  SimpleMenu *next;
  SimpleMenu *prev;
  
  u8g2->setDrawColor(0);
  u8g2->drawBox(0, 0, 64, 60);



  u8g2->setDrawColor(1);
  u8g2->drawBox(0, 0, 64, 12);
  
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
        snprintf(buf, sizeof(buf), "%s", next->name);
        displayMsg(buf);
    }
    return next;
}


void setSprd(int *param)
{
  int index = * (int *) param;
  MyHeater.Status = uint8_t(index);
  if(index==5&&debug) Serial.println("OK!!!!!!!!!!!!");
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
    serialize(buf,&MyHeater.start);
    mySerialTX.write(buf, MyHeater.start.len+7);
    if(MyHeater.sync==false)sendHEX(0xF8);
    Serial.println("Request START");
    delay(300);
    sendStatus();
    if(debug)
    {
      displayMsg(print(&MyHeater.start));
      Serial.println(print(&MyHeater.start));
    }
}

void fStop()
{
    char buf[255];
    serialize(buf,&MyHeater.shutdown);
    mySerialTX.write(buf, MyHeater.shutdown.len+7);
    if(MyHeater.sync==false)sendHEX(0xFC);
    Serial.println("Request STOP");
    delay(300);
    sendStatus();
    if(debug)
    {
      displayMsg(print(&MyHeater.shutdown));
      Serial.println(print(&MyHeater.shutdown));
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




/*void displayValue(SimpleMenu *_menu)
{
  int menucnt = 10;
  u8g2->setDrawColor(0);
  u8g2->drawBox(0, 0, 64, 26);


  u8g2->setDrawColor(1);
  u8g2->drawBox(0, 0, 64, 12);
  char buf[256];
  snprintf(buf, sizeof(buf), "%s %s", _menu->name,_menu->getUValue());  
  u8g2->setDrawColor(0);
  u8g2->drawStr(0, menucnt, buf);
  u8g2->sendBuffer();
}*/




//SPIClass SDSPI(HSPI);

void doubleClick()
{
  //Serial.println("DBL");
  tick=0;
  TopMenu.up();
  
}

void Click()
{
  //Serial.println("CL");
  tick=0;
  TopMenu.down();
}

void longClick()
{
  //Serial.println("LNG-CL");
  tick=0;
  TopMenu.select();
}

//std::atomic<bool> rxPending(false);

/*void receiveHandler() 
{
    S_PACKET data;
    data = ReadMySerial();
    MyHeater.Planar_response(data);

}*/


void setup()
{
  EEPROM.begin(256);
  

  


  debug = EEPROM.read(0);
  
  pult = EEPROM.read(1);

  MyHeater.init();
  

  pinMode(PIN_LED, OUTPUT);
  button.attachDoubleClick(doubleClick);
  button.attachClick(Click);
  button.attachLongPressStart(longClick);

  init_oled();

  

  Serial.begin(115200);                   // initialize serial
  while (!Serial);
  Serial.print("Hello\r\n");

  int count=0;
  int speed=0;
  
  
  LoRa_init();
  TopMenu.begin(displayMenu, displayValue);

  if(!pult)
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
    
   
    char tempmsg[64];
    sprintf(tempmsg,"try %d",speed);
    displayMsgS(tempmsg);

    mySerial.begin(speed);
    mySerialTX.begin(speed);
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
      
      //displayMsgS1(tempmsg);
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
  

  

  

// end setup
}



void loop()
{

  if(mySerial.available())
  {
    data=ReadMySerial();
    
    MyHeater.Planar_response(data);
    if(debug)
    {        
        Serial.println(print(&data));
    }  

  } 
  
  
  onReceive(LoRa.parsePacket());


  if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
	{
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
    

    if(LoraCommand==0)
    {
        sendPing();

        if(MyHeater.Status>0&&MyHeater.Status<6)sendStatus();
    }
    
    if(LoraCommand==1)
    {
        fStart();
        LoraCommand=0;
    }
    
    if(LoraCommand==2)
    {
        fStop();
        LoraCommand=0;
    }
    if(LoraCommand==3)
    {
        
        sendStatus();
        LoraCommand=0;
    }
    
    if(LoraBatt>0)MyHeater.Battery=LoraBatt;
    displayBat(MyHeater.Battery);
    if(LoraStatus>0)MyHeater.Status=LoraStatus;
    displayStatus(MyHeater.GetStatus());
    char temps[30];
    if(MyHeater.temp1>0||MyHeater.temp2||MyHeater.temp3) sprintf(temps,"%d %d %d",MyHeater.temp1,MyHeater.temp2,MyHeater.temp3);
    else sprintf(temps,"%d*",LoraTemp);
    displayTemp(temps);
    
    if(tick > 0 && wakeflag > 0)
    {
        tick=0;
    }
    else if(tick > timetosleep)
    {
      //need to go sleep
      goto_deepsleep();     
     
    }
    if(MyHeater.Mode!=4)tick++;
	}

  
  button.tick();


  


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
  
    char status[4];
    status[0]=0xaa;
    if(MyHeater.sync)
    {
      status[1]=MyHeater.Battery;
      status[2]=MyHeater.temp1;
      status[3]=MyHeater.Status;
    }
    else
    {
      status[1]=random(-127,127);
      status[2]=random(-127,127);
      status[3]=MyHeater.Status;
    }    
    sendMessage(status);
  
}

S_PACKET ReadMySerial()
{
  static S_PACKET temp;
  //memset(temp,0,255);
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
      //reading[0]
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
      }
  }

  packetSize = serialize(reading, &temp);

  c_crc=CRC16(reading,packetSize-2);
  //c_crc = bswap16(c_crc); //reverse byte order

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

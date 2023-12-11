#include <ESP32Time.h>
#include <WiFi.h>
#include <LoRa.h>

extern ESP32Time rtc;
extern int commandQueue[255];
extern int curCommand;

typedef struct timerstruct
{
    uint8_t hh;
    uint8_t mm;
} wTIMER;

//RTC_DATA_ATTR unsigned long startTimers[2];
RTC_DATA_ATTR wTIMER waketimer[2];

void check_timers();
void goto_deepsleep();
void print_wakeup_reason();


void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.print("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer");
                                  //check timers
                                  check_timers();
                                  break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void check_timers()
{
  //unsigned long curtime = rtc.getLocalEpoch();
  
    
  unsigned long epoch;
  struct tm t;
  t.tm_hour = rtc.getHour(true);
  t.tm_min = rtc.getMinute();
  epoch = mktime(&t);

  if(t.tm_hour==waketimer[0].hh&&t.tm_min==waketimer[0].mm)
  {
    //start binar
    curCommand++;
    commandQueue[curCommand]=1;
  }
  else
  {
    goto_deepsleep();
    
  }

  //do someting and go to deepsleep again!
}

void goto_deepsleep()
{
    unsigned long currepoch;
    unsigned long nextwake;
    unsigned long waketime;
    struct tm t;
    
    t = rtc.getTimeStruct();
    t.tm_hour = waketimer[0].hh;
    t.tm_hour = waketimer[0].mm;
    nextwake = mktime(&t);
    currepoch = rtc.getLocalEpoch();

    waketime = (unsigned long) abs((long)(nextwake - currepoch));
    
    WiFi.mode(WIFI_OFF);

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
    esp_sleep_enable_timer_wakeup(waketime*1000*1000);
    esp_deep_sleep_start();
    
}
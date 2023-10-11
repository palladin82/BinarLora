#include <LoRa.h>
#include "sx127xRegs.h"


#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN              26
#define RADIO_RST_PIN               14
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32

const int csPin = 7;          // LoRa radio chip select
const int resetPin = 6;       // LoRa radio reset
const int irqPin = 1;         // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message
String LoraMessage;

byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

int LoraCommand=0;
int LoraTemp=0;
int LoraStatus=0;
int LoraBatt=0;
extern unsigned int wakeflag;

uint8_t setRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb)
{
  if ((msb > 7) || (lsb > 7) || (lsb > msb))
  {
    return (ERR_INVALID_BIT_RANGE);
  }

  uint8_t currentValue = LoRa.readRegister(reg);
  uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
  uint8_t newValue = (currentValue & ~mask) | (value & mask);
  LoRa.writeRegister(reg, newValue);
  return (ERR_NONE);
}


void setSprd()
{
  int index;


  //LoRa.setSpreadingFactor(atoi(CurMenu));
  //EEPROM.write(0, atoi(CurMenu));
  //EEPROM.commit();
  //Serial.println(atoi(CurMenu));

}


void LoRa_init()
{
      // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);// set CS, reset, IRQ pin


  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    //Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }


  LoRa.setPreambleLength(16);
  

  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_SLEEP);
  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_LORA); //must be written in sleep mode
  //LoRa.SetMode(SX127x_OPMODE_STANDBY);
  LoRa.idle();

  LoRa.writeRegister(SX127X_REG_PAYLOAD_LENGTH, 8);
  //setRegValue(SX127X_REG_DIO_MAPPING_1, 0b11000000, 7, 6); //undocumented "hack", looking at Table 18 from datasheet SX127X_REG_DIO_MAPPING_1 = 11 appears to be unsupported by in fact it generates an interrupt on both RXdone and TXdone, this saves switching modes.
  LoRa.writeRegister(SX127X_REG_LNA, SX127X_LNA_BOOST_ON);
  LoRa.writeRegister(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_ON | SX1278_LOW_DATA_RATE_OPT_OFF);
  //setRegValue(SX127X_REG_OCP, SX127X_OCP_ON | SX127X_OCP_150MA, 5, 0); //150ma max current
  LoRa.setOCP(240);
  LoRa.setTxPower(20,PA_OUTPUT_PA_BOOST_PIN);
  //LoRa.setPreambleLength(2);
  //SetPreambleLength(SX127X_PREAMBLE_LENGTH_LSB);
  //setRegValue(SX127X_REG_INVERT_IQ, (uint8_t)IQinverted, 6, 6);
  //LoRa.setTxPower(20);
  //LoRa.setFrequency(868E6);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(7);
  LoRa.setSpreadingFactor(12);
}



void sendMessage(String outgoing) {
  digitalWrite(2,1);
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket(true);                     // finish packet and send it
  delay(200);
  digitalWrite(2,0);
  msgCount++;                           // increment message ID
}

void sendHEX(char outgoing) {
  digitalWrite(2,1);
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(1);                        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket(true);                     // finish packet and send it
  delay(200);
  digitalWrite(2,0);
  msgCount++;                           // increment message ID
}


void onReceive(int packetSize) 
{
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";

  //  digitalWrite(LED_PIN, !btnState);

  while (LoRa.available())
  {

    incoming += (char)LoRa.read();

  }

  if (incomingLength != incoming.length()) 
  {   // check length for error
    //Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    //Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:


  
  //in car mode
  if (incoming[0] == 0xF8)
  {
    LoraCommand=1;
    wakeflag++;
  }
  if (incoming[0] == 0xFC)
  {
     LoraCommand=2;
     wakeflag++;
  }
  if (incoming[0] == 0x08)
  {
     LoraCommand=3;
  }
  if (incoming[0] == 0xAA)
  {
     LoraBatt = incoming[1];
     LoraTemp = incoming[2];
     LoraStatus = incoming[3];
  }
  if(incoming[0] == 0x20)
  {

     LoraMessage = incoming;
  }

}





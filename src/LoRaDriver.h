#include <LoRa.h>
#include "sx127xRegs.h"
#include "sx127xRegs.h"
#include "sx1276Regs-LoRa.h"
#include "sx1276Regs-Fsk.h"
#define FREQ_STEP                                   61.03515625


#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              19
#define RADIO_MOSI_PIN              27
#define RADIO_CS_PIN                18
#define RADIO_DIO0_PIN              26
#define RADIO_RST_PIN               14
#define RADIO_DIO1_PIN              33
#define RADIO_BUSY_PIN              32
#define REG_PA_CONFIG            0x09
#define REG_PA_DAC               0x4d
#define REG_LNA                  0x0c
#define Channel                  9093E5
#define bandwidth                125E3

int64_t currChannel=Channel;

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
static void RxChainCalibration( void );
void SX1276SetModem();
uint8_t SX1276Read( uint16_t addr );
void writeRegisterBits(uint8_t reg, uint8_t value, uint8_t mask);
//bool  GetFrequencyErrorbool();
void  SetPPMoffsetReg(float offset);
extern unsigned int wakeflag;
extern int commandQueue[255];
extern int curCommand;

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
  LoRa.prebegin(currChannel);
  //RxChainCalibration();
  LoRa.begin(currChannel);
  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_SLEEP);
  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_SLEEP);
  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_SLEEP);
  LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_LORA); //must be written in sleep mode
  LoRa.idle();
  LoRa.setFrequency(Channel);
  LoRa.setOCP(240);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.writeRegister(REG_PA_CONFIG, 0xff);
  LoRa.setPreambleLength(18);
  LoRa.setCodingRate4(7);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(62.5E3);
  //LoRa.enableCrc();
  //LoRa.writeRegister(SX1278_REG_MODEM_CONFIG_3,RF_RXCONFIG_AGCAUTO_OFF);
  LoRa.writeRegister(SX127X_REG_DETECT_OPTIMIZE, 0x63);
  writeRegisterBits(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, SX127X_DETECT_OPTIMIZE_SF_MASK );
  LoRa.writeRegister(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12 );
  LoRa.writeRegister(SX127X_REG_LNA, SX127X_LNA_BOOST_ON|SX127X_LNA_GAIN_1);
  
  
  /*LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_SLEEP);  
  //LoRa.writeRegister(SX127X_REG_OP_MODE, SX127x_OPMODE_LORA); //must be written in sleep mode
  LoRa.writeRegister(SX127X_REG_PAYLOAD_LENGTH, 32);
  LoRa.writeRegister(SX127X_REG_LNA, SX127X_LNA_BOOST_ON);
  LoRa.writeRegister(SX1278_REG_MODEM_CONFIG_3, SX1278_AGC_AUTO_ON | SX1278_LOW_DATA_RATE_OPT_ON);
  
  LoRa.setOCP(240);
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSignalBandwidth(62.5E3);
  LoRa.setCodingRate4(7);
  LoRa.setSpreadingFactor(12);
  LoRa.setPreambleLength(16);
  writeRegisterBits(SX127X_REG_DETECT_OPTIMIZE, SX127X_DETECT_OPTIMIZE_SF_7_12, SX127X_DETECT_OPTIMIZE_SF_MASK );
  LoRa.writeRegister(SX127X_REG_DETECTION_THRESHOLD, SX127X_DETECTION_THRESHOLD_SF_7_12 );
  //LoRa.writeRegister(REG_PA_CONFIG, 0b11111111); // That's for the transceiver
  LoRa.writeRegister(REG_PA_DAC, 0x87); // That's for the transceiver
  //LoRa.writeRegister(REG_LNA, 00); // TURN OFF LNA FOR TRANSMIT
  LoRa.idle();*/
}



void sendMessage(String outgoing) {
  digitalWrite(2,1);
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  //delay(200);
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
  LoRa.endPacket();                     // finish packet and send it
  //delay(200);
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
    curCommand++;
    commandQueue[curCommand]=1;
    //LoraCommand=1;
    wakeflag++;
  }
  if (incoming[0] == 0xFC)
  {
    curCommand++;
    commandQueue[curCommand]=2;
    //LoraCommand=2;
    wakeflag++;
  }
  if (incoming[0] == 0x08)
  {
    curCommand++;
    commandQueue[curCommand]=3;
    //LoraCommand=3;
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

  //freq correction!!!
  float ferr=0;
  int32_t freqError = ((LoRa.readRegister(0x28) & 0B111)<<16) + (LoRa.readRegister(0x29)<<8) + LoRa.readRegister(0x2a);
  
  int32_t bit=(freqError & (1<<19));

  if (LoRa.readRegister(0x28) & B1000) 
  { // Sign bit is on
    freqError -= 524288; // B1000'0000'0000'0000'0000
  }

  float tf=((float)freqError*16777216/32E6)* (float)125 / 500.0f * 0.95*0.0001;

  
  
  SetPPMoffsetReg(tf);
  

}




uint8_t SX1276Read( uint16_t addr )
{
	return LoRa.readRegister(addr);//read(address);
}

void SX1276Write( uint16_t addr, uint8_t data )
{
	LoRa.writeRegister(addr,data);//write(address, value);
}


void SX1276SetChannel( uint32_t freq )
{
    //SX1276.Settings.Channel = freq;
    freq = ( uint32_t )( ( double )freq / ( double )FREQ_STEP );
    SX1276Write( REG_FRFMSB, ( uint8_t )( ( freq >> 16 ) & 0xFF ) );
    SX1276Write( REG_FRFMID, ( uint8_t )( ( freq >> 8 ) & 0xFF ) );
    SX1276Write( REG_FRFLSB, ( uint8_t )( freq & 0xFF ) );
}

static void RxChainCalibration( void )
{
    uint8_t regPaConfigInitVal;
    uint32_t initialFreq;

    // Save context
    regPaConfigInitVal = SX1276Read( REG_PACONFIG );
    initialFreq = ( double )( ( ( uint32_t )SX1276Read( REG_FRFMSB ) << 16 ) |
                              ( ( uint32_t )SX1276Read( REG_FRFMID ) << 8 ) |
                              ( ( uint32_t )SX1276Read( REG_FRFLSB ) ) ) * ( double )FREQ_STEP;

    // Cut the PA just in case, RFO output, power = -1 dBm
    SX1276Write( REG_PACONFIG, 0x00 );

    // Launch Rx chain calibration for LF band
    SX1276Write( REG_IMAGECAL, ( SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Sets a Frequency in HF band
    SX1276SetChannel( 8683E5 );

    // Launch Rx chain calibration for HF band
    SX1276Write( REG_IMAGECAL, ( SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_MASK ) | RF_IMAGECAL_IMAGECAL_START );
    while( ( SX1276Read( REG_IMAGECAL ) & RF_IMAGECAL_IMAGECAL_RUNNING ) == RF_IMAGECAL_IMAGECAL_RUNNING )
    {
    }

    // Restore context
    SX1276Write( REG_PACONFIG, regPaConfigInitVal );
    SX1276SetChannel( initialFreq );
}



void SX1276SetModem()
{
        LoRa.sleep();
        SX1276Write( REG_OPMODE, ( SX1276Read( REG_OPMODE ) & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON );

        SX1276Write( REG_DIOMAPPING1, 0x00 );
        SX1276Write( REG_DIOMAPPING2, 0x00 );
}

void writeRegisterBits(uint8_t reg, uint8_t value, uint8_t mask)
{
    
        uint8_t currentValue = LoRa.readRegister(reg);
        uint8_t newValue = (currentValue & ~mask) | (value & mask);
        LoRa.writeRegister(reg, newValue);
    
}







void  SetPPMoffsetReg(float offset)
{
  /*uint8_t frfMsb = LoRa.readRegister(0x06);
  uint8_t frfMid = LoRa.readRegister(0x07);
  uint8_t frfLsb = LoRa.readRegister(0x08);
*/
  char msg[255]; 
  currChannel -= offset*1E4;
  
  int offs=round(offset);
  //currfreq -= offs;
  //LoRa.setFrequency(currChannel);
  

  sprintf(msg,"%uk %.1fk",currChannel/1000, offset);
  displayMsgS(msg);

}
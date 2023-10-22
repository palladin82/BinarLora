#define bswap16(x) __builtin_bswap16(x)
#define MIN_PACKET_SIZE 7


uint8_t int_ping[]= {0xaa,0x03,0x00,0x00,0x0f,0x58,0x7c};
char char_ping[]= "\xaa\x03\x00\x00\x0f\x58\x7c";
char char_shut[] =         "\xaa\x03\x00\x00\x03\x5d\x7c";
char char_welcome[]= "\xaa\x03\x01\x00\x11\x7f\x5d\x10";
char char_get_settings[] = "\xaa\x03\x00\x00\x02\x00\x00";
char char_status[] =       "\xaa\x03\x00\x00\x0f\x58\x7c";
char char_start[] = "\xaa\x03\x06\x00\x01\xff\xff\x04\x10\x01\x03\x2e\xaf";



char r_02[] = "\xaa\x04\x06\x00\x02\x00\x32\x04\x01\x02\x00\x00\x00";
char r_03[] = "\xaa\x04\x00\x00\x03\x00\x00";
char r_06[] = "\xaa\x04\x04\x00\x06\x0a\x3a\x05\x01\x00\x00";
char r_0f[] = "\xaa\x04\x0a\x00\x0f\x00\x01\x14\x15\x7f\x00\x81\x01\x26\x00\x00\x00";
char r_11[] = "\xaa\x04\x01\x00\x11\x7f\x00\x00";
char r_1e[] = "\xaa\x00\x00\x00\x1e\x00\x00";
char r_23[] = "\xaa\x04\x04\x00\x23\x00\x32\x00\x00\x00\x00";


/* msg_id2
  0x01: Start heater
              LN    CM       MD SP VN PW
        aa 03 06 00 01 ff ff 04 10 01 03 2e af
            - LN : length
            - CM : cmd
            - MD : MODE 
                    - `01`: By T Heater
                    - `02`: By T Panel
                    - `03`: By T Air
                    - `04`: By Power
            - SP : temperature setpoint
            - VN : ventilation ON/OFF
            - PW : power level 0-9

  0x02: Get/Set settings
  0x03: Shutdown
  0x04: Initialization
  0x11: Report panel temperature  
       aa 03 01 00 11 0a ba d1 / 0a is temp
  0x0F: Status
        aa 03 00 00 0f 58 7c
                       00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 13
        pr id LN    CM ss                xx       tm             vv    vv
        aa 04 13 00 0f 03 00 00 0a 7f 00 84 01 b2 04 00 37 37 00 6d 00 6d 00 64 c3 0a
                  - `CM`: current command
                  - `xx`: Battery voltage * 10, e.g. `0x84` equals 13.2V
                  - `ss`: Status
                  - `00`: Heater off
                  - `01`: Starting
                  - `04`: Running
                  - `05`: Shutting down
                  - `vv`: Ventilation power. During startup this value goes up to `0x96`, and then decreases slowly down to `0x46` during regular operation (status `0x04`). Also, this value seems to appear twice, god knows, why.
*/


struct S_PACKET {
    uint8_t preamble; //0xAA
    uint8_t device; //0x03 controller, 0x04 heater
    uint8_t len; //payload
    uint8_t msg_id1;
    uint8_t msg_id2; 
    char payload[255];
    uint16_t crc;
};


uint16_t CRC16(char *buf, uint8_t len);
uint8_t serialize(char *buf, struct S_PACKET *t);

class Heater
{
  public:    
    uint8_t Status=0;
    uint8_t Battery=1;
    uint8_t VentPower;
    uint8_t TimeAS;
    uint8_t Error;
    uint8_t VentOn;
    uint8_t PowerLevel;
    uint8_t Mode;
    uint8_t TempSetPoint;
    bool running;

    uint8_t temp1=10;
    uint8_t temp2=10;
    int temp3=800;

    S_PACKET start;
    S_PACKET shutdown;
    S_PACKET settingsP;
    bool settingsOk=false;
    bool sync=false;

    

    void recalc_crc(char *buf,int len) 
    {
      uint16_t c_crc;
      c_crc=CRC16(buf,len-2);
      c_crc = bswap16(c_crc); //reverse byte order
      memcpy(buf+len-2,&c_crc,2);
    }

    void recalc_crc_spacket(S_PACKET *packet)
    {
      char buf[255];
      serialize(buf, packet);
      packet->len = buf[2];
      packet->crc = buf[5+packet->len];
      packet->crc |= buf[6+packet->len] << 8;  
    }

    void init()
    {
        shutdown.preamble=0xaa;
        shutdown.preamble = 0xaa;
        shutdown.device = 0x03;
        shutdown.len = 0;
        shutdown.msg_id1=0;
        shutdown.msg_id2=0x03;        
        shutdown.crc=0;
        recalc_crc_spacket(&shutdown);

        start.preamble = 0xaa;
        start.device = 0x03;
        start.len = 0x06;
        start.msg_id1=0;
        start.msg_id2=0x01;
        
        start.payload[0x00]=0xff;
        start.payload[0x01]=0xff;
        
        start.payload[0x02]=0x04; //mode 04 by power // 01 - T-heater/ 02 - T-Panel/ 03 - T-Air
        start.payload[0x03]=0x50; //temp setpoint
        start.payload[0x04]=0x01; //ventilation on
        start.payload[0x05]=0x09; //power level 0-9
        
        start.crc=0;
        recalc_crc_spacket(&start);

        settingsP.preamble = 0xaa;
        settingsP.device = 3;
        settingsP.len = 0;
        settingsP.msg_id1 = 0;
        settingsP.msg_id2 = 0x02;
        recalc_crc_spacket(&settingsP);


    }

    char *GetStatus()
    {
          static char msg[9];
          switch(Status)
          {
            case 0: sprintf(msg,"Off");return msg; break;
            case 1: sprintf(msg,"Starting");return msg; break;
            case 2: sprintf(msg,"WarmUP");return msg; break;
            case 3: sprintf(msg,"Heat");return msg; break;
            case 4: sprintf(msg,"Run");return msg; break;
            case 5: sprintf(msg,"Shut");return msg; break;            
            default: sprintf(msg,"E-%d",Status);return msg; break;
          }
          return msg;
    }

    void Planar_response(S_PACKET data)
    {
      switch(data.msg_id2)
      {
          case '\x01': Mode=data.payload[0x03];
                       TempSetPoint=data.payload[0x04];
                       VentOn=data.payload[0x05];
                       PowerLevel=data.payload[0x06];
                       running=true;                       
                       break;//start                      
          case '\x02': //settings
          /* 
                      0x02: Settings
                                           00 01 02 03 04 05 
                                  LN    CM       MD SP VN PW
                            aa 03 06 00 01 ff ff 04 10 01 03 2e af
                                - LN : length
                                - CM : cmd
                                - MD : MODE 
                                        - `01`: By T Heater
                                        - `02`: By T Panel
                                        - `03`: By T Air
                                        - `04`: By Power
                                - SP : temperature setpoint
                                - VN : ventilation ON/OFF
                                - PW : power level
                    */
                      Mode=data.payload[0x02];
                      TempSetPoint=data.payload[0x03];
                      VentOn=data.payload[0x04];
                      PowerLevel=data.payload[0x05];

                      start.preamble = 0xaa;
                      start.device = 0x03;
                      start.len = 0x06;
                      start.msg_id1=0;
                      start.msg_id2=0x01;
                      strcpy(start.payload, data.payload);
                      start.crc=0;
                      recalc_crc_spacket(&start);
                      settingsOk=true;
                      break;
          case '\x03': running=false;
                       //shutdown
                      break;
          case '\x04': //initializations
                      break;
          case '\x11': //report panel temp
                      break;
          case '\x0f': //status ping
                    //                   00 01 02 03 04 05 06 07 08 09 
                    //H >>aa 04 0a 00 0f 00 01 00 1a 7f 00 7b 01 2b 00 | 50 ad
                     //                  s1 s2 er ht et    bv ft ft
                    //    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
                    /*                   00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 
                          pr id LN    CM ss                xx    CT tm    fs gs    vv    vv gc
                          aa 04 13 00 0f 03 00 00 0a 7f 00 84 01 b2 04 00 37 37 00 6d 00 6d 00 64 c3 0a
                          - `xx`: Battery voltage * 10, e.g. `0x84` equals 13.2V
                                    - 'xx': Battery voltage * 10
                                    - `CT`: core temperature
                                    - `ss`: Status 
                                          - `00`: Heater off
                                          - `01`: Starting
                                          - `04`: Running
                                          - `05`: Shutting down
                                          – `06`: Testing environment
                                          – `08`: Ventilation
                                    - `fs`: set fan speed min=0x45, max=0x70
                                    - `tm`: timer after start
                                    - `gs`: get fan speed min=0x45, max=0x70
                                    - `vv`: Ventilation power. During startup this value goes up to `0x96`, and then 
                                            decreases slowly down to `0x46` during regular operation (status `0x04`). 
                                            Also, this value seems to appear twice, god knows, why.
                                    - `gc`: Glow current (0-9A) during startup
                    */
                      Status=data.payload[0x00];
                      Error=data.payload[0x02];
                      Battery=data.payload[0x06];                      
                      TimeAS=data.payload[0x09];
                      VentPower=data.payload[0x0e];
                      temp1=data.payload[3];                      
                      temp2=data.payload[4];
                      temp3=(data.payload[7]) | (data.payload[8]<<8);
                      break;
      }
    }

  
};



char *printHEX(const uint8_t *s, int len)
{
	static char buf[0xFF];
  memset(buf,0,255);

  int i;

	if (len>(0xFF/3)) {  //3 bytes for every character in s
		sprintf(buf,"[STRING TOO LONG]\0");
		return buf;
	}

  for (i = 0; i < len; ++i) 
      sprintf(buf+(3*i),"%02x ", s[i]);
	
  buf[3*i] = '\0';

	return buf;
}

uint8_t *printHEXs(const char *s, int len)
{
   static uint8_t buf[255];
   memset(buf,0,255);

   for(int i=0;i<=len;i++)
   strcat((char*)buf, printHEX((uint8_t*)&s[i],1));

   return buf;
}

char *print(struct S_PACKET *p) 
{
    //char buf[255];
    static char returnbuf[255];
    memset(returnbuf,0,255);


    uint8_t temp[2];
    memcpy (temp, &p->crc,2);

    strcat(returnbuf, printHEX(&p->preamble, 1));
    strcat(returnbuf, printHEX(&p->device, 1));
    strcat(returnbuf, printHEX(&p->len, 1));
    strcat(returnbuf, printHEX(&p->msg_id1, 1));
    strcat(returnbuf, printHEX(&p->msg_id2, 1));

    //char payloadhex[]=printHEXs(p->payload, p->len);
    strcat(returnbuf, (char*)printHEXs(p->payload, p->len-1));

    

    //printf(printHEX(&temp[0], 1));

    strcat(returnbuf, printHEX(&temp[0], 1));
    strcat(returnbuf, printHEX(&temp[1], 1));
    
    /*printf("id1: 0x%s ",printHEX(&p->msg_id1,1));
    printf("id2: 0x%s ",printHEX(&p->msg_id2,1));
    printf("data (%u): %s",p->len,printHEX(p->payload,p->len));
    */
    //printf("len: %u ",p.len);
    //printf("crc: %s ",printHEX(&p.crc,2));

    return returnbuf;
}

uint16_t CRC16(char *buf, uint8_t len)
{
  uint16_t crc = 0xFFFF;
  uint8_t pos;
  uint8_t i;

  for (pos = 0; pos < len; pos++)
  {
    crc ^= (uint8_t)buf[pos];          // XOR byte into least sig. byte of crc

    for (i = 8; i != 0; i--) {    // Loop over each bit
      if ((crc & 0x0001) != 0) {      // If the LSB is set
        crc >>= 1;                    // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // Else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }
  return crc;
}

uint8_t serialize(char *buf, struct S_PACKET *t) {
  uint16_t crc;

  buf[0] = t->preamble; 
  buf[1] = t->device; 
  buf[2] = t->len; 
  buf[3] = t->msg_id1; 
  buf[4] = t->msg_id2;
  if(t->len > 0) memcpy(buf+5,t->payload,t->len);

  crc = CRC16(buf,t->len+MIN_PACKET_SIZE-2);
  crc = bswap16(crc);
  memcpy(buf+MIN_PACKET_SIZE+t->len-2,&crc,2);

  return t->len+MIN_PACKET_SIZE;
}

 

uint8_t _parse(struct S_PACKET *t, char *buf, int len) { //parse buf into t; returns 0 if no packet found otherwise returns number of bytes consumed
  uint8_t i = 0;
  uint16_t crc;
  uint16_t c_crc;
  uint8_t payload_len;
  char *packet;
  uint16_t r_len;
  uint16_t packet_len;

  if (len<MIN_PACKET_SIZE) return 0;

  while (i<=len-MIN_PACKET_SIZE) {
    if (buf[i]!=0xaa) {i++; continue;}
    //we have preamble - check if we have full packet
    packet = buf+i;
    r_len = len-i;

    payload_len = packet[2];
    packet_len = MIN_PACKET_SIZE+payload_len;
    if (r_len<packet_len) {
      //not
      return 0;
    }
    //yes - but check crc
    memcpy(&crc,packet+packet_len-2,2);
    c_crc=CRC16(packet,packet_len-2);
    c_crc = bswap16(c_crc); //reverse byte order
    if (crc!=c_crc) {
      //printf("incorrect crc!. calculated: %s\n",printHEX(&c_crc,2));
      i++;
      continue;
    }
    
    //all good - we have packet
    t->preamble = packet[0];
    t->device = packet[1];
    t->len = packet[2];
    t->msg_id1 = packet[3];
    t->msg_id2 = packet[4];
    memcpy(t->payload,packet+5,t->len);
    memcpy(&t->crc,packet+packet_len-2,2);
    return i+packet_len;
  }
  return 0;
}

struct S_PACKET *parse(int src, char *buf, int len) 
{ //parse with accumulator for part data 
    //static z = 0;

    static struct S_PACKET p;
    static char s_buf[255];
    static uint8_t s_len = 0;
    uint8_t i;
    uint8_t ret;

    if (s_len+len>=255) 
    {
        //printf("Unable to find valid packet! Are you sure serial baud rate is correct?\n");
        s_len = 0;
    }

    for (i=0;i<len;i++)
      s_buf[s_len++] = buf[i];

   
    ret=_parse(&p,s_buf,s_len);

    if (!ret) return 0; //no packet found

    s_len = 0;

    return &p;
}




S_PACKET deserialize(char *buf)
{
  static S_PACKET t;
  
  t.preamble = buf[0];
  t.device = buf[1];
  t.len = buf[2];
  t.msg_id1 = buf[3];
  t.msg_id2 = buf[4];  
  memcpy(t.payload,buf+5,t.len);
  t.crc = buf[5+t.len];
  t.crc |= buf[6+t.len] << 8;
  
  return t;
}
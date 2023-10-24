# BinarLora

ESP32 TTGO LoRa32 project for remote control AutoTerm Binar 5S


    00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
                   00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f 10 11 12 
    pr id LN    CM PL                xx    CT ss    fs gs    vv    vv gc
    aa 04 13 00 0f 03 00 00 0a 7f 00 84 01 b2 04 00 37 37 00 6d 00 6d 00 64 c3 0a
    - `xx`: Battery voltage * 10, e.g. `0x84` equals 13.2V
            - 'xx': Battery voltage * 10
            - `CT`: core temperature
            - `ss`: Status / OR TIMER ???
                    - `00`: Heater off
                    - `01`: Starting
                    - `04`: Running
                    - `05`: Shutting down
                    – `06`: Testing enviroment
                    – `08`: Ventilation
            - `fs`: set fan speed min=0x45, max=0x70
            - `gs`: get fan speed min=0x45, max=0x70
            - `vv`: Ventilation power. During startup this value goes up to `0x96`, and then 
                    decreases slowly down to `0x46` during regular operation (status `0x04`). 
                    Also, this value seems to appear twice, god knows, why.
            - `gc`: Glow current (0-9A) during startup

<p align="center"> 
<img src="https://github.com/palladin82/BinarRemote/raw/main/img/binar.jpg">
<img src="https://github.com/palladin82/BinarRemote/raw/main/img/LilyGO TTGO Lora_6-1500x1500.jpg">
</p>
 

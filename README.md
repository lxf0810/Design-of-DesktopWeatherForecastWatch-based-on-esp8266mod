# Design-of-DesktopWeatherForecastWatch-based-on-esp8266mod

材料：      
1，0805的贴片电阻和电容（具体数值查看原理图）      
2，0603的贴片电阻和电容（具体数值查看原理图）      
3，TP4057芯片     
4，3.3V充电锂电池  
5，拨码开关  
6，非自锁按键  
7，OLED显示屏(IIC通信)  
8，ESP12F(ESP8266MOD)芯片  
9，AMS1117-3.3   
10，无源蜂鸣器  
11，USB TYPEC接口  
12，XC6206P332MR稳压芯片  
   
硬件连线：  
OLED_SCL----->GPIO2  (D4)  
OLED_SDA----->GPIO14 (D5)  
OLED_VCC----->3V3  
OLED_GND----->GND  

LED---------->GPIO12  (D6)  
蜂鸣器------->GPIO13  (D7)  
按键--------->GPIO5   (D1)  


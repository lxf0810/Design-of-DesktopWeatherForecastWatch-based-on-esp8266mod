/***************************************************************************************************************
项目名称: ESP8266+OLED气象小电视
程序功能: 智能配网、网络授时、获取知心天气信息（当前天气、预报天气）、获取b站up主信息（粉丝数，视频播放数） 
代码参考: b站up主:https://www.bilibili.com/video/BV1hP4y1x7gL?vd_source=4056047798ee681a20ab5a02519e2a03
特别感谢B站UP主：“_铁甲依旧在_”
***************************************************************************************************************/
#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  
#include <ESP8266_Seniverse.h>       
#include "ESP8266_BiliBili.h"
#include <time.h>
#include <sys/time.h>
#include <coredecls.h>
#include <ESP8266HTTPClient.h>
#include "OLEDDisplayUi.h"
#include <OLEDDisplay.h>
#include <Wire.h>
#include "SH1106Wire.h"            //(1.3寸)   #include <SSD1306Wire.h>0.9寸用这个
#include "images.h"
#include "WeatherStationFonts.h"
#include "DrawPicture.h"

/***********************************************************************
* 心知天气API请求所需信息
* 请对以下信息进行修改，填入您的心知天气私钥以及需要获取天气信息的城市和温度单位
* 如需进一步了解心知天气API所提供的城市列表等信息，请前往心知天气官方产品文档网址：
* https://www.seniverse.com/docs
***********************************************************************/
String reqUserKey = "SBpXvd4XVu3HqLYsO";   // 私钥
String reqLocation = "chengdu";            // 城市
String reqUnit = "c";                      // 摄氏/华氏

WeatherNow weatherNow;                     // 建立WeatherNow对象用于获取心知天气信息
Forecast forecast;                         // 建立Forecast对象用于获取心知天气信息
LifeInfo lifeInfo;                         // 建立Forecast对象用于获取生活指数信息

/******************************************************************************************/
// 哔哩哔哩HTTP请求所需信息
String mid = "314404732";                   // 哔哩哔哩用户uid361778729
UpInfo upInfo("314404732");                 // 建立对象用于获取粉丝信息 括号中的参数是B站的UUID    
                               
const char* host = "api.bilibili.com";     // 将要连接的服务器地址  
const int   httpPort = 80;                   // 将要连接的服务器端口      

String reqRes = "/x/space/arc/search?mid=" + mid +"&pn=1&ps=10&order=pubdate&jsonp=jsonp";           
                                           // b站视频播放数请求网址
long numberoffans = -1;
long numberofplay = -1;

#define NUMBEROFVIDEOS           15
bool bilibili_first = true;
char Bilibili_Ate;

volatile int  value = 0;
int           value_2 = 0;

/****************************显示定义***************************************/         
FrameCallback frames[] = {drawTime, drawCurrentWeather, drawForecastWeather, drawBilibili, drawMoveImage};     //创建FrameCallback给ui使用，Frame的作用可以让ui来切换不同的frame，绘制不同的画面
int frameCount = 5;         

//OverlayCallback overlays[] = { drawHeaderOverlay };      // 覆盖回调函数
//int numberOfOverlays = 1;  //覆盖数
        
bool first = true;                                         // 首次更新标志

const unsigned long  BILIBILI_UPDATE_PERIOD = 1*60;        // B站信息更新时间间隔
const unsigned long  WEATHER_UPDATE_PERIOD = 5*60;         // 天气信息更新时间间隔
long timeSinceLastBilibiliUpdate = 0;                      // 上次更新后的时间
long timeSinceLastWeatherUpdate = 0;                       // 上次天气更新后的时间

void ICACHE_RAM_ATTR InterruptHandle(void);                //提前声明中断函数

uint8_t led = 12;
const byte GPIO_Interrupt_Pin = 5;                        //用5号引脚作为中断触发引脚
                                          
void setup() 
{
    Serial.begin(9600);       
    pinMode(led,OUTPUT);
    digitalWrite(led,HIGH);
    
    display.init();                                               // 屏幕初始化

    display.setFont(ArialMT_Plain_10);                            // 设置字体字号
    
    display.clear();
    display.display();                                            // 清屏
    
    ui.setTargetFPS(30);                                          // 刷新频率30帧每秒

    ui.disableAllIndicators();                                    // 关闭frame标签
    //ui.disableAutoTransition();                                   // 关闭自动切换frame功能


    ui.disableAutoTransition();                                   // 关闭自动切换frame功能
    
    ui.enableAllIndicators();                                     // 显示指示器

  //ui.setActiveSymbol(activeSymbole);                            // 设置活动符号（不知道干什么用）
  //ui.setInactiveSymbol(inactiveSymbole);                        // 设置非活动符号（不知道干什么用）
  
    
    ui.setIndicatorPosition(BOTTOM);                              // 符号位置，你可以把这个改成TOP, LEFT, BOTTOM, RIGHT
  
    ui.setIndicatorDirection(LEFT_RIGHT);                         // 定义第一帧在栏中的位置
  
    ui.setFrameAnimation(SLIDE_LEFT);                             // 屏幕切换方向，您可以更改使用的屏幕切换方向 SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
    
    ui.setFrames(frames,frameCount);                              // 设置框架
    
    ui.init();
    display.flipScreenVertically();                               // 屏幕翻转
  
    pinMode(GPIO_Interrupt_Pin, INPUT_PULLUP);                    // 将中断触发引脚（2号引脚）设置为INPUT_PULLUP（输入上拉）模式

    attachInterrupt(digitalPinToInterrupt(GPIO_Interrupt_Pin), InterruptHandle, FALLING);    // 设置中断触发程序

    connectWiFi();                                                //连接WiFi
    
    configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com"); //ntp获取时间，你也可用其他"pool.ntp.org","0.cn.pool.ntp.org","1.cn.pool.ntp.org","ntp1.aliyun.com"
    delay(200);


    // 配置心知天气请求信息（实时天气、预报天气、生活建议都要配置！！！）
    weatherNow.config(reqUserKey, reqLocation, reqUnit);
    forecast.config(reqUserKey, reqLocation, reqUnit);
    lifeInfo.config(reqUserKey, reqLocation, reqUnit);
}

//自动配网
void connectWiFi(void)
{
    WiFiManager wifiManager;                                                          // 建立WiFiManager对象 
                                                                                      
    if(!wifiManager.autoConnect("我是呆瓜呀"))                                         // 自动连接WiFi。以下语句的参数是连接ESP8266时的WiFi名称，无需密码
    {
        display.drawXbm(display.width()/2 - Sad_Person_width/2,display.height()-Sad_Person_height,Sad_Person_width, Sad_Person_height, Sad_Person);
        display.drawXbm(8, 0, Character_1_width, Character_1_height, CHARACTER_1[0]);  /*没*/
        display.drawXbm(24, 0, Character_1_width, Character_1_height, CHARACTER_1[1]); /*有*/
        display.drawXbm(40, 0, Character_1_width, Character_1_height, CHARACTER_1[2]); /*网*/
        display.drawXbm(56, 0, Character_1_width, Character_1_height, CHARACTER_1[3]); /*，*/
        display.drawXbm(72, 0, Character_1_width, Character_1_height, CHARACTER_1[4]); /*不*/
        display.drawXbm(88, 0, Character_1_width, Character_1_height, CHARACTER_1[5]); /*开*/
        display.drawXbm(104, 0, Character_1_width, Character_1_height, CHARACTER_1[6]);/*心*/
        
        display.display();
        wifiManager.autoConnect("我是呆瓜呀");
    }
    
    // 如果您希望该WiFi添加密码，可以使用以下语句：
    // wifiManager.autoConnect("AutoConnectAP", "12345678");
    // 以上语句中的12345678是连接AutoConnectAP的密码
    
    // WiFi连接成功后将通过串口监视器输出连接成功信息 
    Serial.println(""); 
    Serial.print("ESP8266 Connected to ");
    Serial.println(WiFi.SSID());                // WiFi名称
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());             // IP
}

// 更新天气信息
void WeatherUpdate(void)
{
  if(weatherNow.update()&&lifeInfo.update()){                                  
    Serial.println(F("======Weahter Info======"));
    Serial.print("Server Response: ");
    Serial.println(weatherNow.getServerCode());                                // 获取服务器响应码
    Serial.print(F("Weather Now: "));
    Serial.print(weatherNow.getWeatherText());                                 // 获取当前天气（字符串格式）
    currentweather.weather_text = weatherNow.getWeatherText();
    Serial.print(F(" "));
    Serial.println(weatherNow.getWeatherCode());                               // 获取当前天气状态码（整数格式）
    currentweather.weather_code = weatherNow.getWeatherCode();
    Serial.print(F("Temperature: "));
    Serial.println(weatherNow.getDegree());                                    // 获取当前温度数值
    currentweather.degree = weatherNow.getDegree();
    Serial.print(F("Dressing: "));
    Serial.println(lifeInfo.getDressing());                                    // 获取穿衣建议
    currentweather.dressing = lifeInfo.getDressing();
    Serial.print(F("Last Update: "));
    Serial.println(weatherNow.getLastUpdate());                                // 获取服务器更新天气信息时间
    currentweather.lastupdate = weatherNow.getLastUpdate();
    Serial.println(F("========================"));     
    } 
    else {                                                                     // 更新失败
    Serial.println("WeatherNow Update Fail...");   
    Serial.print("Server Response: ");                                         // 输出服务器响应状态码供用户查找问题
    Serial.println(weatherNow.getServerCode());                                // 心知天气服务器错误代码说明可通过以下网址获取
    }                                                                          // https://docs.seniverse.com/api/start/error.html
  
    if(forecast.update()){                                                     // 更新天气信息
    for(int i = 0; i < 3; i++){
      Serial.print(F("========Day ")); 
      Serial.print(i);      
      Serial.println(F("========"));     

      Serial.print(F("Day Weather: "));
      Serial.print(forecast.getDate(i));                                       //获取日期(字符串格式)
      String Datereturn = forecast.getDate(i);
      forecastweather[i].date = Datereturn.substring(5, Datereturn.length());
      
      Serial.print(F("Day Weather: "));
      Serial.print(forecast.getDayText(i));                                    //获取白天天气(字符串格式)
      forecastweather[i].weather_text = forecast.getDayText(i);
      
      Serial.print(F(" "));
      Serial.println(forecast.getDayCode(i));                                 //获取白天天气状态码(整数格式)
      forecastweather[i].weather_code = forecast.getDayCode(i);
      
      Serial.print(F("High: "));
      Serial.print(forecast.getHigh(i));                                      //获取最高气温(整数格式) 
      forecastweather[i].max_temperature = forecast.getHigh(i);
      if(i == 0)
      {
         currentweather.max_temperature = forecast.getHigh(i);
      }
      Serial.println(F("°C"));     
      Serial.print(F("LOW: "));
      Serial.print(forecast.getLow(i));                                       //获取最低气温(整数格式)  
      forecastweather[i].min_temperature = forecast.getLow(i);
      if(i == 0)
      {
         currentweather.min_temperature = forecast.getLow(i);
      }
      Serial.println(F("°C"));
      Serial.print(F("Rainfall: "));                                          //获取降水概率信息(小数格式)
      Serial.print(forecast.getRain(i));  
      forecastweather[i].probability = forecast.getRain(i);
      Serial.println(F("%"));
      
      Serial.print(F("Last Update: "));                                       //获取心知天气信息更新时间(字符串格式)       
      Serial.println(forecast.getLastUpdate());                
    }
    Serial.print(F("Server Code: ")); 
    Serial.println(forecast.getServerCode()); 
    Serial.println(F("====================="));   
  } else {    // 更新失败
    Serial.println("Forecast Update Fail...");   
    Serial.print("Server Response: ");                                        // 输出服务器响应状态码供用户查找问题
    Serial.println(weatherNow.getServerCode());                               // 心知天气服务器错误代码说明可通过以下网址获取
  }                                                                           // https://docs.seniverse.com/api/start/error.html
  delay(5000);
}

// 向bilibili服务器请求视频播放信息并对信息进行解析
void Bilibili_Play_Request(void)
{
    WiFiClient client;
    int i = 0;
    int len;
    String httpRequest = String("GET ") + reqRes + " HTTP/1.1\r\n" +          // 建立http请求信息
                                "Host: " + host + "\r\n" + 
                                "Connection: close\r\n\r\n";
    Serial.println(""); 
    Serial.print("Connecting to "); Serial.print(host);
 
    if (client.connect(host, 80))                                             // 尝试连接服务器    
    {                                                    
      Serial.println(" Success!");

      client.print(httpRequest);                                              // 向服务器发送http请求信息
      Serial.println("Sending request: ");
      Serial.println(httpRequest);  

      String status_response = client.readStringUntil('\n');                  // 获取并显示服务器响应状态行
      Serial.print("status_response: ");
      Serial.println(status_response);
   
      if (client.find("\r\n\r\n"))                                            // 使用find跳过HTTP响应头
      {
        Serial.println("Found Header End. Start Parsing.");
      } 
      if(!bilibili_first)
      {
        client.readStringUntil(Bilibili_Ate); 
        parseInfo_1(client);                          // 利用ArduinoJson库解析响应信息
      }
      if(bilibili_first)
      {
        String temp = client.readStringUntil('{'); 
        Serial.println(temp);
        unsigned int len = temp.length();
        char p[5];
        strcpy(p,temp.c_str());
        Bilibili_Ate = p[2];
        Serial.println(Bilibili_Ate);

        char qc ={'e'};
        if (Bilibili_Ate == 'e')
        {
          Serial.println("right!!!");
        }
        //client.readStringUntil(Bilibili_Ate);
        bilibili_first = false;
      }  
                                                     
   } 
   
   else
   {
      Serial.println(" connection failed!");
   }     
   
  client.stop();                                                              // 断开客户端与服务器连接工作
}

// 利用ArduinoJson库解析响应信息
void parseInfo_1(WiFiClient client){
  const size_t capacity = JSON_ARRAY_SIZE(NUMBEROFVIDEOS)+2*JSON_OBJECT_SIZE(2) +3*JSON_OBJECT_SIZE(3)+JSON_OBJECT_SIZE(4) +NUMBEROFVIDEOS*JSON_OBJECT_SIZE(21) +550*NUMBEROFVIDEOS;
  DynamicJsonDocument doc(capacity); 
  deserializeJson(doc, client);
  
  int code = doc["code"]; 
  const char* message = doc["message"]; 
  int ttl = doc["ttl"]; 

  JsonObject data = doc["data"];
  JsonArray data_list_vlist = data["list"]["vlist"];
  int i = 0;
  numberofplay = 0;
  for(i;i<NUMBEROFVIDEOS;i++)
  {
    JsonObject data_list_vlist_i = data_list_vlist[i];
    long data_list_vlist_i_play = data_list_vlist_i["play"].as<long>();
    numberofplay = numberofplay+data_list_vlist_i_play;
    Serial.print("Play is ");
    Serial.println(String(numberofplay));
    //Serial.println(numberofplay);
    Serial.println(String(client));
    //Serial.println(doc);
  }
  Serial.print("Bilibili Play: ");   
  Serial.println(String(numberofplay));    
}

void bilibiliUpdate(void)
{
    FansInfo fansInfo(mid);   
    if(fansInfo.update())                                                     // 更新信息成功   
    { 
        Serial.println("Update OK"); 
        Serial.print("Server Response: ");  
        Serial.println(fansInfo.getServerCode());     
        Serial.print(F("Fans Number: "));
        Serial.println(fansInfo.getFansNumber());
        numberoffans = fansInfo.getFansNumber();
    } 
    else                                                                      // 更新失败
    {    
        Serial.println("Bilibili Fans_Update Fail...");  
        Serial.print("Server Response: ");   
        Serial.println(upInfo.getServerCode());    
    }

    Bilibili_Play_Request();                                                  // 请求b站视频播放数
    
    Serial.println(F("======================"));  
}

void loop() 
{
   if(first)                                                                  // 首次加载，即开机
   {
      display.clear();
      display.display(); 
      drawStartImage();                                                       // 绘制开机画面
      first = false;   
      
      WeatherUpdate();
      timeSinceLastWeatherUpdate = millis();                                  // 更新天气信息
      bilibiliUpdate();
      timeSinceLastBilibiliUpdate = millis();                                 // 更新b站信息

      ui.switchToFrame(0);                                                    // 切换到第0帧，及显示时间
      Serial.println(String(timeSinceLastBilibiliUpdate));
      Serial.println("Successfully booted !");
   }
   if (millis() - timeSinceLastWeatherUpdate > (1000L * WEATHER_UPDATE_PERIOD))  //定时更新天气信息（每五分钟更新一次）
   { 
      WeatherUpdate();
      timeSinceLastWeatherUpdate = millis();
      Serial.println(String( timeSinceLastWeatherUpdate));
   }
   if (millis() - timeSinceLastBilibiliUpdate > (1000L * BILIBILI_UPDATE_PERIOD))//定时更新b站信息（每一分钟更新一次）
   { 
      bilibiliUpdate();
      timeSinceLastBilibiliUpdate = millis();
      Serial.println(String( timeSinceLastBilibiliUpdate));
   } 
   int remainingTimeBudget = ui.update();                                    // 帧速率由ui控制，更新完ui会返回下一帧需要等待的时间
   if(remainingTimeBudget > 0)                                               // 延迟对应的时间后可再次更新屏幕   
   {
     delay(remainingTimeBudget);
   }
}

//中断处理函数
void ICACHE_RAM_ATTR InterruptHandle(void)
{
  
 //delayMicroseconds(DELAY_PERIOD);
 if(digitalRead(GPIO_Interrupt_Pin) == LOW )
 {
    delayMicroseconds(100000);
    if(digitalRead(GPIO_Interrupt_Pin) == LOW )
    {
      value++;
    }
    value_2 = 0;
 }
 if(value == 1)
 {
  digitalWrite(led,LOW);
  ui.enableAllIndicators();
  ui.switchToFrame(0);                                 // 绘制时间页面
 }
  if(value == 2)
 {
  digitalWrite(led,HIGH);
  ui.enableAllIndicators();
  ui.switchToFrame(1);                                 // 绘制当日天气页面
 }
 if(value == 3)
 {
  digitalWrite(led,LOW);
  ui.enableAllIndicators();                            
  ui.switchToFrame(2);                                 // 绘制天气预报页面
 }
 if(value == 4)
 {
  digitalWrite(led,HIGH);
  ui.disableAllIndicators();                           // 绘制b站信息页面
  ui.switchToFrame(3);
 }
 if(value == 5)
 {
  value = 0;
  value_2 = 1;
  ui.disableAllIndicators();                           // 绘制隐藏画面
  ui.switchToFrame(4);
 }
 delayMicroseconds(50000);
}

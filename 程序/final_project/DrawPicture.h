#ifndef     __DRAWPICTURE_H__
#define     __DRAWPICTURE_H__

#include "images.h"
#include "WeatherStationFonts.h"
#include "DrawPicture.h"

const int I2C_DISPLAY_ADDRESS = 0x3c;  //I2c地址默认
const int SDA_PIN = 14;  //OLED数据引脚
const int SDC_PIN = 2;  //OLED时钟引脚

//定义一个display设备，并且设置它的SDA为SDA_PIN引脚，SDC为SDC_PIN
SH1106Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);    // 1.3寸用这个
OLEDDisplayUi   ui( &display );                                   // 创建一个ui,并且把display传递给它

/**************时间获取宏定义******************/
#define TZ              8                                         // 中国时区为8
#define DST_MN          0                                         // 默认为0

#define TZ_MN           ((TZ)*60)                                 //时间换算
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

const String DAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};  //星期
const String MONTH_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};  //月份

time_t now; //实例化时间

extern long numberoffans;
extern long numberofplay;
extern int value_2;

void drawProgressBarDemo(int count) {
  int progress = count - 10;
  // draw the progress bar
  for(progress;progress < count;progress++)
  {
    display.drawProgressBar(10, 48, 108, 6, progress);                          // 绘制进度条（x,y,width,height,progress(进度取值0~100)）
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.display();
    delay(20);
  }
    display.clear();
}

//绘制开机画面
void drawStartImage() {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_1);
    display.display();
    drawProgressBarDemo(10);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_3);
    display.display();
    drawProgressBarDemo(20);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_2);
    display.display();
    drawProgressBarDemo(30);
   
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_3);
    display.display();
    drawProgressBarDemo(40);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_1);
    display.display();
    drawProgressBarDemo(50);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_3);
    display.display();
    drawProgressBarDemo(60);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_2);
    display.display();
    drawProgressBarDemo(70);
   
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_3);
    display.display();
    drawProgressBarDemo(80);

    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_1);
    display.display();
    drawProgressBarDemo(90);
    
    display.drawXbm(display.width()/2 - Start_Logo_width/2, 0, Start_Logo_width, Start_Logo_height, Start_Logo_3);
    display.display();
    drawProgressBarDemo(100);
}

//------画动态位图
#define XPOS     0
#define YPOS     1
#define SPEED    2

#define NUMBEROFFLAKES  10

//绘制雪花下落画面
void drawMoveImage(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  int8_t f;
  int8_t icons[NUMBEROFFLAKES][3];
  //初始化10个（宏定义数量）“雪花片”的参数：
  //x坐标位置随机
  //y最上端位置隐含
  //下降速度为随机数
  
  for(f=0;f<NUMBEROFFLAKES;f++)
  {
    icons[f][XPOS] = random(1 - Snowflakes_Logo_width,display->width());
    icons[f][YPOS] = -Snowflakes_Logo_height;
    icons[f][SPEED]= random(1,6);
  }
  for(;;)
  {
    display->clear();//清屏
    //画出每个雪花片
    for(f=0;f<NUMBEROFFLAKES;f++)
    {
      display->drawXbm(icons[f][XPOS],icons[f][YPOS],Snowflakes_Logo_width, Snowflakes_Logo_height, Snowflakes_Logo_bits);
    }
    display->display();
    delay(100);
    //更新每个雪花片位置
    for(f=0;f<NUMBEROFFLAKES;f++)
    {
      icons[f][YPOS] += icons[f][SPEED];
      if(icons[f][YPOS] >= display->height())
      {
        icons[f][XPOS] = random(1 - Snowflakes_Logo_width,display->width());
        icons[f][YPOS] = -Snowflakes_Logo_height;
        icons[f][SPEED]= random(1,6);
      } 
      if(value_2 != 1)
        break;
    }   
    if(value_2 != 1)
      break;
  }
}

//显示时间
void drawTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) 
{  
  
  now = time(nullptr);                                                      // 函数的time的返回值为time_t，实际上是一个long数据。此时now存储有年月日时分秒的信息
  struct tm* timeInfo;                                                      // 创建一个tm的结构体指针，该结构体的成员分别是秒、分、时、日、月、年份、星期、一年中的第几天、夏令标识符
  timeInfo = localtime(&now);                                               // localtime的函数的作用是将获取到的long数据类型的数据转化成tm结构体，并存到上一句定义的timeInfo中
  char buff[16];                                                            // 定义一个buff数组

  display->setTextAlignment(TEXT_ALIGN_CENTER);                             // 设置文字显示方式为居中对齐
  display->setFont(ArialMT_Plain_16);                                       // 设置字体样式及大小
  String date = DAY_NAMES[timeInfo->tm_wday];                               // date为星期几

  sprintf_P(buff, PSTR("%04d-%02d-%02d  %s"), timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday, DAY_NAMES[timeInfo->tm_wday].c_str());// 显示日期，星期
  display->drawString(64, 5, String(buff));
  display->setFont(DSEG7_Classic_Regular_24);

  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);// 显示时分秒
  display->drawString(64, 22 , String(buff));
  display->setTextAlignment(TEXT_ALIGN_LEFT);

  char buff_2[14];                                                          // 页眉绘制
  sprintf_P(buff_2, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff_2));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  
  display->drawString(122, 54, String(numberoffans));
  display->drawHorizontalLine(0, 52, 128);
}

//显示b站粉丝数，视频播放量
void drawBilibili(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setFont(ArialMT_Plain_10); 
  display->setTextAlignment(TEXT_ALIGN_CENTER);         
  display->drawString(32, 52, String(numberoffans));
  display->drawString(96, 52, String(numberofplay));
  display->drawXbm(14, 0, Bilibili_Logo_width, Bilibili_Logo_height, Bilibili_Logo);
  
  display->drawXbm(8, 35, CHARACTER_Fans_width, CHARACTER_Fans_height, CHARACTER_Fans[0]);
  display->drawXbm(24, 35, CHARACTER_Fans_width, CHARACTER_Fans_height, CHARACTER_Fans[1]);
  display->drawXbm(40, 35, CHARACTER_Fans_width, CHARACTER_Fans_height, CHARACTER_Fans[2]);

  display->drawXbm(72, 35, CHARACTER_Play_width, CHARACTER_Play_height, CHARACTER_Play[0]);
  display->drawXbm(88, 35, CHARACTER_Play_width, CHARACTER_Play_height, CHARACTER_Play[1]);
  display->drawXbm(104, 35, CHARACTER_Play_width, CHARACTER_Play_height, CHARACTER_Play[2]);
}

//绘图页眉覆盖
void drawHeaderOverlay(OLEDDisplay *display) {   
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[14];
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);

  display->setColor(WHITE);
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(6, 54, String(buff));
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  
  display->drawString(122, 54, String(numberoffans));
  display->drawHorizontalLine(0, 52, 128);
}

//获取天气图标
String getMeteoconIcon(int cond_code) {  //获取天气图标
  if (cond_code == 0 || cond_code == 2) {
    return "B";
  }
  if (cond_code == 99) {
    return ")";
  }
  if (cond_code == 1|| cond_code == 3) {
    return "C";
  }
  if (cond_code == 9) {
    return "D";
  }
  if (cond_code == 500) {
    return "E";
  }
  if (cond_code == 27 || cond_code == 26 || cond_code == 28 || cond_code == 29) {
    return "F";
  }
  if (cond_code == 499 || cond_code == 901) {
    return "G";
  }
  if (cond_code == 5 || cond_code == 6 || cond_code == 7 || cond_code == 8) {//GAI
    return "H";
  }
  if (cond_code == 31 || cond_code == 511 || cond_code == 512 || cond_code == 513) {
    return "L";
  }
  if (cond_code == 30 || cond_code == 509 || cond_code == 510 || cond_code == 514 || cond_code == 515) {
    return "M";
  }
  if (cond_code == 102) {
    return "N";
  }
  if (cond_code == 213) {
    return "O";
  }
  if (cond_code == 11 || cond_code == 303) {
    return "P";
  }
  if (cond_code == 13 || cond_code == 308 || cond_code == 309 || cond_code == 314 || cond_code == 399) {
    return "Q";
  }
  if (cond_code == 14 || cond_code == 15 || cond_code == 16 || cond_code == 17 || cond_code == 18 || cond_code == 315 || cond_code == 316 || cond_code == 317 || cond_code == 318) {
    return "R";
  }
  if (cond_code == 200 || cond_code == 201 || cond_code == 202 || cond_code == 203 || cond_code == 204 || cond_code == 205 || cond_code == 206 || cond_code == 207 || cond_code == 208 || cond_code == 209 || cond_code == 210 || cond_code == 211 || cond_code == 212) {
    return "S";
  }
  if (cond_code == 10 || cond_code == 301) {
    return "T";
  }
  if (cond_code == 22 || cond_code == 408) {
    return "U";
  }
  if (cond_code == 21) {
    return "V";
  }
  if (cond_code == 23 || cond_code == 24 || cond_code == 25 || cond_code == 409 || cond_code == 410) {
    return "W";
  }
  if (cond_code == 12 || cond_code == 19 || cond_code == 20 || cond_code == 405 || cond_code == 406) {
    return "X";
  }
  if (cond_code == 4) {
    return "Y";
  }
  return ")";
}

typedef struct CurrentWeatherData {
  String weather_text;
  int weather_code;
  int degree;
  String dressing;
  int max_temperature;
  int min_temperature;
  String lastupdate;
}CurrentWeatherData;

typedef struct ForecastWeatherData {
  String date;
  String weather_text;
  int weather_code;
  int max_temperature;
  int min_temperature; 
  float probability;
}ForecastWeatherData;

//绘制当前天气
CurrentWeatherData  currentweather;         // 实例化当前天气结构体
ForecastWeatherData forecastweather[3];     // 实例化预报天气结构体   

// 绘制当前天气画面
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  display->setFont(ArialMT_Plain_10);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 38 + y, currentweather.weather_text + " | Dress:" + currentweather.dressing + "  ");

  display->setFont(ArialMT_Plain_24);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp =  String(currentweather.degree) + "°C" ;
  display->drawString(60 + x, 3 + y, temp);
  display->setFont(ArialMT_Plain_10);
  display->drawString(62 + x, 26 + y, String(currentweather.max_temperature) + "°C | " + String(currentweather.min_temperature) + "°C");
  display->setFont(Meteocons_Plain_36);
  display->setTextAlignment(TEXT_ALIGN_CENTER);

  String code = getMeteoconIcon(currentweather.weather_code);
  display->drawString(32 + x, 0 + y, code);

  drawHeaderOverlay(display);
}
// 绘制天气预报画面
void drawForecastSingle(OLEDDisplay *display, int x, int y, int dayIndex) {  
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y, forecastweather[dayIndex].date);
  display->setFont(Meteocons_Plain_21);
  String code = getMeteoconIcon(forecastweather[dayIndex].weather_code);
  display->drawString(x + 20, y + 12, code);

  String temp = String(forecastweather[dayIndex].max_temperature) + " | " + String(forecastweather[dayIndex].min_temperature);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + 20, y + 34, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}
// 绘制天气预报
void drawForecastWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  drawForecastSingle(display, x, y, 0);
  drawForecastSingle(display, x + 44, y, 1);
  drawForecastSingle(display, x + 88, y, 2);
  
  drawHeaderOverlay(display);                  //绘制页眉
}

void Bilibili_AteWords(char *p)
{
  

}
#endif

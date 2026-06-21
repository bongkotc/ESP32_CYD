#include <SPI.h>
#include <TFT_eSPI.h>           
#include <XPT2046_Touchscreen.h> 

// บังคับส่งพินระดับ Hardware คุมหน้าจอ CYD โดยตรง
// (แก้ไขปัญหาเปิดไฟล์ User_Setup.h ไม่ถูกตำแหน่ง)
#define CYD_MISO 12
#define CYD_MOSI 13
#define CYD_SCLK 14
#define CYD_CS   15
#define CYD_DC    2
#define CYD_RST  -1
#define CYD_BL   21

#define TOUCH_CS 33

// ประกาศอินสแตนซ์หน้าจอและระบบทัช
TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen touch(TOUCH_CS);

// กำหนดพิกัดปุ่ม
const int btnX = 60;
const int btnY = 80;
const int btnW = 120;
const int btnH = 50;

#define TS_MINX 300
#define TS_MAXX 3900
#define TS_MINY 200
#define TS_MAXY 3800

void setup() {
  Serial.begin(115200);
  
  // สั่งเปิดไฟ Backlight ของหน้าจอให้ติดสว่าง (พิน 21)
  pinMode(CYD_BL, OUTPUT);
  digitalWrite(CYD_BL, HIGH); 
  
  // เริ่มต้นทำงานหน้าจอ
  tft.init();
  tft.setRotation(1); // จอนอน 320x240
  tft.fillScreen(TFT_BLACK); // ล้างหน้าจอเป็นสีดำ
  
  // เริ่มต้นทำงานทัชสกรีน
  touch.begin();
  touch.setRotation(1); 
  
  // วาดปุ่มทดสอบ
  tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawCentreString("TEST", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
  
  Serial.println("System Initialized! Screen should be active.");
}

void loop() {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    
    int touchX = map(p.x, TS_MINX, TS_MAXX, 0, 320);
    int touchY = map(p.y, TS_MINY, TS_MAXY, 0, 240);
    
    Serial.printf("Touch X: %d, Y: %d\n", touchX, touchY);
    
    if ((touchX >= btnX && touchX <= (btnX + btnW)) && 
        (touchY >= btnY && touchY <= (btnY + btnH))) {
      
      Serial.println("[EVENT] Button Clicked!");
      
      tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN);
      tft.setTextColor(TFT_BLACK);
      tft.drawCentreString("CLICK!", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
      
      delay(300); 
      
      tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLUE);
      tft.setTextColor(TFT_WHITE);
      tft.drawCentreString("TEST", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
    }
  }
}
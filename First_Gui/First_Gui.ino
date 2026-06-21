#include <SPI.h>
#include <TFT_eSPI.h>           
#include <XPT2046_Touchscreen.h> 


// กำหนดพินทัชสกรีนตามแผนผังรูปภาพตรงรุ่น
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define XPT2046_DIN  32
#define XPT2046_OUT  39
#define XPT2046_IRQ  36

// ประกาศอินสแตนซ์หน้าจอและระบบทัช
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

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
  
  // เริ่มต้นระบบบัส SPI แยกเฉพาะสำหรับทัชสกรีน
  touchSPI.begin(XPT2046_CLK, XPT2046_OUT, XPT2046_DIN, XPT2046_CS);
  touch.begin(touchSPI); 
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
    Serial.printf("RAW DATA -> x: %d, y: %d\n", p.x, p.y);
    // Serial.printf("Touch X: %d, Y: %d\n", touchX, touchY);
    
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
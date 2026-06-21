#include <SPI.h>
#include <TFT_eSPI.h>           
#include <XPT2046_Touchscreen.h> 

// กำหนดพินทัชสกรีนตามแผนผังตรงรุ่น
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define XPT2046_DIN  32
#define XPT2046_OUT  39
#define XPT2046_IRQ  36

#define CYD_BL       21 // Backlight

// --- กำหนดพิน LED สำหรับทดสอบ (เลือกอย่างใดอย่างหนึ่ง) ---
#define TARGET_LED   4   // ใช้ Green LED บนบอร์ด (อ้างอิงพิน RGB: IO4)
// #define TARGET_LED 27 // หรือจะสลับไปใช้พิน IO27 ที่พอร์ตขยายภายนอก

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ);

const int btnX = 60;
const int btnY = 80;
const int btnW = 120;
const int btnH = 50;

#define TS_MINX 300
#define TS_MAXX 3900
#define TS_MINY 200
#define TS_MAXY 3800

TaskHandle_t xGuiTaskHandle = NULL;
TaskHandle_t xLogicTaskHandle = NULL;

// ตัวแปรเก็บสถานะ (Thread-Safe Flags)
volatile bool isButtonPressed = false;
volatile bool ledState = false; // false = OFF, true = ON

void vGuiTask(void * pvParameters);
void vLogicTask(void * pvParameters);

void setup() {
  Serial.begin(115200);

  // ตั้งค่าพิน LED
  pinMode(TARGET_LED, OUTPUT);
  digitalWrite(TARGET_LED, LOW); // เริ่มต้นที่ OFF

  pinMode(CYD_BL, OUTPUT);
  digitalWrite(CYD_BL, HIGH);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  touchSPI.begin(XPT2046_CLK, XPT2046_OUT, XPT2046_DIN, XPT2046_CS);
  touch.begin(touchSPI);
  touch.setRotation(1);
  
  // วาดปุ่มสถานะเริ่มต้น (LED: OFF)
  tft.fillRect(btnX, btnY, btnW, btnH, TFT_RED); // OFF ใช้สีแดง
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawCentreString("LED: OFF", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
  
  // สร้าง FreeRTOS Tasks
  xTaskCreatePinnedToCore(vGuiTask, "GUI_Task", 4096, NULL, 2, &xGuiTaskHandle, 1);
  xTaskCreatePinnedToCore(vLogicTask, "Logic_Task", 4096, NULL, 1, &xLogicTaskHandle, 0);
}

// === Core 1: จัดการหน้าจอและอ่านค่าทัชสกรีน ===
void vGuiTask(void * pvParameters) {
  for(;;) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      int touchX = map(p.x, TS_MINX, TS_MAXX, 0, 320);
      int touchY = map(p.y, TS_MINY, TS_MAXY, 0, 240);
      
      if ((touchX >= btnX && touchX <= (btnX + btnW)) && (touchY >= btnY && touchY <= (btnY + btnH))) {
        
        // ส่งสัญญาณบอก Core 0 ว่าปุ่มโดนกดแล้ว
        isButtonPressed = true; 
        
        // รอให้ Core 0 ทำการสลับสถานะ ledState ให้เสร็จก่อนแป๊บหนึ่ง
        vTaskDelay(pdMS_TO_TICKS(50)); 
        
        // วาด UI ใหม่ตามสถานะ ledState ที่ถูกเปลี่ยน
        if (ledState) {
          tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN); // ON ใช้สีเขียว
          tft.setTextColor(TFT_BLACK);
          tft.drawCentreString("LED: ON", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
        } else {
          tft.fillRect(btnX, btnY, btnW, btnH, TFT_RED);   // OFF ใช้สีแดง
          tft.setTextColor(TFT_WHITE);
          tft.drawCentreString("LED: OFF", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1);
        }
        
        vTaskDelay(pdMS_TO_TICKS(400)); // หน่วงเวลา Debounce กันปุ่มรัว
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// === Core 0: ควบคุมฮาร์ดแวร์ LED ด้านหลัง ===
void vLogicTask(void * pvParameters) {
  for(;;) {
    if (isButtonPressed) {
      // สลับสถานะตัวแปร (Toggle)
      ledState = !ledState; 
      
      // สั่งงานขา GPIO ให้กระแสไฟเปลี่ยนตามตัวแปร
      if (ledState) {
        digitalWrite(TARGET_LED, HIGH); // ไฟติด
        Serial.println("[HARDWARE] LED Turned ON");
      } else {
        digitalWrite(TARGET_LED, LOW);  // ไฟดับ
        Serial.println("[HARDWARE] LED Turned OFF");
      }
      
      isButtonPressed = false; // เคลียร์สถานะการกด
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // ทำงานตรวจเช็กทุกๆ 50ms
  }
}

void loop() {
  vTaskDelete(NULL);
}
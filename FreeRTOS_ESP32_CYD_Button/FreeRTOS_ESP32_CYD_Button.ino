#include <SPI.h>
#include <TFT_eSPI.h>           
#include <XPT2046_Touchscreen.h> 

// กำหนดพินทัชสกรีนตามแผนผังรูปภาพตรงรุ่น [cite: 1]
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define XPT2046_DIN  32
#define XPT2046_OUT  39
#define XPT2046_IRQ  36

// พินควบคุม Backlight จอ CYD
#define CYD_BL       21

// ประกาศอินสแตนซ์หน้าจอและระบบทัช [cite: 1]
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI(VSPI); //[cite: 2]
XPT2046_Touchscreen touch(XPT2046_CS, XPT2046_IRQ); //[cite: 2]

// กำหนดพิกัดปุ่ม [cite: 2]
const int btnX = 60;
const int btnY = 80;
const int btnW = 120; //[cite: 3]
const int btnH = 50; //[cite: 3]

// ค่าสำหรับ Calibrate ทัชสกรีน [cite: 3]
#define TS_MINX 300 //[cite: 4]
#define TS_MAXX 3900 //[cite: 4]
#define TS_MINY 200 //[cite: 4]
#define TS_MAXY 3800 //[cite: 4]

// ประกาศ Task Handles ของ FreeRTOS
TaskHandle_t xGuiTaskHandle = NULL;
TaskHandle_t xLogicTaskHandle = NULL;

// ส่วนควบคุมแชร์ตัวแปรสถานะระหว่าง Task (Thread-Safe Flag)
volatile bool isButtonClicked = false;

// ฟังก์ชันต้นแบบสำหรับ Tasks
void vGuiTask(void * pvParameters);
void vLogicTask(void * pvParameters);

void setup() {
  Serial.begin(115200);

  // สั่งเปิดไฟ Backlight ของหน้าจอให้ติดสว่าง [cite: 5]
  pinMode(CYD_BL, OUTPUT); //[cite: 5]
  digitalWrite(CYD_BL, HIGH); //[cite: 5]
  
  // เริ่มต้นทำงานหน้าจอ
  tft.init();
  tft.setRotation(1); //[cite: 5]
  tft.fillScreen(TFT_BLACK); // ล้างหน้าจอเป็นสีดำ [cite: 6]
  
  // เริ่มต้นระบบบัส SPI แยกเฉพาะสำหรับทัชสกรีน [cite: 6]
  touchSPI.begin(XPT2046_CLK, XPT2046_OUT, XPT2046_DIN, XPT2046_CS); //[cite: 6]
  touch.begin(touchSPI); //[cite: 6]
  touch.setRotation(1); //[cite: 7]
  
  // วาดปุ่มทดสอบเริ่มต้น [cite: 7]
  tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLUE);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawCentreString("TEST", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1); //[cite: 8]
  
  Serial.println("System Hardware Initialized.");

  // -------------------------------------------------------------------------
  // สร้าง FreeRTOS Tasks
  // -------------------------------------------------------------------------
  
  // 1. สร้าง Task สำหรับจัดการหน้าจอและทัชสกรีน (Priority 2, รันบน Core 1)
  xTaskCreatePinnedToCore(
    vGuiTask,           // ฟังก์ชันที่ทำงาน
    "GUI_Task",         // ชื่อ Task
    4096,               // Stack Size (Bytes)
    NULL,               // Parameters
    2,                  // Priority (สูงกว่างานเบื้องหลังเพื่อให้ทัชลื่นไหล)
    &xGuiTaskHandle,    // Task Handle
    1                   // Pin to Core 1
  );

  // 2. สร้าง Task สำหรับงานประมวลผลลอจิกอื่นๆ (Priority 1, รันบน Core 0)
  xTaskCreatePinnedToCore(
    vLogicTask,
    "Logic_Task",
    4096,
    NULL,
    1,                  // Priority ต่ำกว่าเล็กน้อยเพื่อไม่ให้กวนงาน UI
    &xLogicTaskHandle,
    0                   // Pin to Core 0 (ทำงานขนานกันโดยแท้จริง)
  );
}

// === Task 1: จัดการหน้าจอและการทัชสกรีน (Core 1) ===
void vGuiTask(void * pvParameters) {
  Serial.println("GUI Task Started on Core 1");
  
  for(;;) {
    if (touch.touched()) { //[cite: 9]
      TS_Point p = touch.getPoint(); //[cite: 9]
      
      int touchX = map(p.x, TS_MINX, TS_MAXX, 0, 320); //[cite: 10]
      int touchY = map(p.y, TS_MINY, TS_MAXY, 0, 240); //[cite: 10]
      
      // ตรวจสอบขอบเขตปุ่ม [cite: 11]
      if ((touchX >= btnX && touchX <= (btnX + btnW)) && 
          (touchY >= btnY && touchY <= (btnY + btnH))) { //[cite: 12]
        
        Serial.println("[EVENT] Button Clicked!"); //[cite: 12]
        
        // ยกธงสถานะขึ้น เพื่อแจ้งบอกให้อีก Core รับทราบ
        isButtonClicked = true; 
        
        // เปลี่ยนสีปุ่มเป็นสีเขียวแจ้งเตือน [cite: 12]
        tft.fillRect(btnX, btnY, btnW, btnH, TFT_GREEN); //[cite: 13]
        tft.setTextColor(TFT_BLACK); //[cite: 13]
        tft.drawCentreString("CLICK!", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1); //[cite: 13]
        
        // ใน FreeRTOS ต้องใช้ vTaskDelay แทนการใช้ delay มาตรฐานเพื่อให้ระบบสลับไปทำงานอื่นได้
        vTaskDelay(pdMS_TO_TICKS(300)); //[cite: 14]
        
        // วาดปุ่มกลับเป็นสีน้ำเงินเหมือนเดิม [cite: 14]
        tft.fillRect(btnX, btnY, btnW, btnH, TFT_BLUE); //[cite: 14]
        tft.setTextColor(TFT_WHITE); //[cite: 14]
        tft.drawCentreString("TEST", btnX + (btnW / 2), btnY + (btnH / 2) - 8, 1); //[cite: 14]
      }
    }
    
    // พักเบรก Task 20ms เพื่อเปิดโอกาสให้ OS บริหารงานส่วนอื่น (ช่วยป้องกันปัญหา Watchdog Reset)
    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}

// === Task 2: งานประมวลผลลอจิก/คํานวณเบื้องหลัง (Core 0) ===
void vLogicTask(void * pvParameters) {
  Serial.println("Logic Task Started on Core 0");
  
  for(;;) {
    // ดักจับข้อมูล Flag ที่ถูกส่งมาจากฝั่ง GUI Task (Core 1)
    if (isButtonClicked) {
      Serial.println("[LOGIC] Core 0 acknowledged: Processing background trigger...");
      
      // >>> คุณสามารถใส่โค้ดประมวลผลหนักๆ หรือโค้ดส่งข้อมูล Wi-Fi/TCP ตรงส่วนนี้ได้ <<<
      
      isButtonClicked = false; // เคลียร์ Flag รอการกดครั้งต่อไป
    }
    
    vTaskDelay(pdMS_TO_TICKS(100)); // พักเบรกสำหรับงานลอจิกเบื้องหลัง 100ms
  }
}

void loop() {
  // สั่งยุติการทำงานของ loop() หลักทิ้งไป เพื่อยกงานทั้งหมดไปให้ระบระบบงาน Task ของ FreeRTOS บริหารโดยตรง
  vTaskDelete(NULL); 
}
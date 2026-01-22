#include <Arduino_GFX_Library.h>
#include "Arduino_PimoroniPAR8.h"
#include "Arduino_ST7789_Parallel.h"

// Define this to use Arduino_Canvas (framebuffer), comment out for direct drawing
#define USE_CANVxxxAS

// COLOR macro - swaps bytes for canvas mode, normal for direct mode
#ifdef USE_CANVAS
  #define COLOR(c) ((uint16_t)(((c) >> 8) | ((c) << 8)))
#else
  #define COLOR(c) (c)
#endif

// RGB565 color definitions (5-6-5 bits: RRRRRGGGGGGBBBBB)
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F

// Use Arduino_GFX built-in canvas for flicker-free drawing
Arduino_PimoroniPAR8 *bus;
Arduino_ST7789_Parallel *display;

#ifdef USE_CANVAS
Arduino_Canvas *gfx;
#else
Arduino_ST7789_Parallel *gfx;
#endif

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  #ifdef USE_CANVAS
  Serial.println("\n=== ST7789 with Canvas (Framebuffer) ===");
  #else
  Serial.println("\n=== ST7789 Direct Drawing ===");
  #endif
  
  // Create bus and display
  bus = new Arduino_PimoroniPAR8();
  display = new Arduino_ST7789_Parallel(bus, GFX_NOT_DEFINED, 0, false, 320, 240);
  
  if(!display->begin()) {
    Serial.println("Display init failed!");
    while(1);
  }
  
  #ifdef USE_CANVAS
  // Create canvas (framebuffer) - Arduino_GFX built-in!
  gfx = new Arduino_Canvas(320, 240, display);
  
  if(!gfx->begin()) {
    Serial.println("Canvas init failed!");
    while(1);
  }
  
  Serial.println("Display and canvas initialized!");
  #else
  // Direct drawing - no framebuffer
  gfx = display;
  Serial.println("Display initialized (direct mode)!");
  #endif
  
  // Set backlight
  display->setBacklight(255);
}

void loop() {
  static uint32_t frame = 0;
  static uint32_t framecount = 0;
  static uint32_t fps = 0;
  static uint32_t next_fps_time = millis() + 1000;
  
  // Draw primitives (to canvas or directly to display)
  // Each primitive handles its own startWrite/endWrite internally
  gfx->fillScreen(COLOR(BLACK));
  
  gfx->setCursor(10, 10);
  gfx->setTextColor(COLOR(WHITE));
  gfx->setTextSize(2);
  gfx->print("Frame: ");
  gfx->print(frame);
  gfx->print(" FPS: ");
  gfx->println(fps);
  
  gfx->drawRect(50, 50, 100, 100, COLOR(RED));
  gfx->fillCircle(100, 100, 30, COLOR(GREEN));
  gfx->drawLine(0, 0, 319, 239, COLOR(BLUE));
  
  int x = (frame * 2) % 220;
  gfx->fillRect(x, 150, 50, 50, COLOR(YELLOW));
  
  #ifdef USE_CANVAS
  // Flush canvas to display (one fast update - no flicker!)
  gfx->flush();
  #endif
  
  frame++;
  framecount++;
  
  if(millis() >= next_fps_time) {
    fps = framecount;
    Serial.print("FPS: ");
    Serial.println(fps);
    framecount = 0;
    next_fps_time = millis() + 1000;
  }
  
  // NO DELAY - run as fast as possible!
}

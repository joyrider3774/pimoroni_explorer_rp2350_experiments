#include <Arduino_GFX_Library.h>
#include "Arduino_PimoroniPAR8.h"
#include "Arduino_ST7789_Parallel.h"

// Color swap macro for existing RGB565 values
// Use with Arduino_GFX's color565() function: COLOR_SWAP(gfx->color565(r, g, b))
#define COLOR_SWAP(c) ((uint16_t)(((c) >> 8) | ((c) << 8)))

// Pre-swapped RGB565 color definitions
// Format: RRRRRGGGGGGBBBBB (5-6-5 bits)
#define BLACK_SWAP   0x0000
#define WHITE_SWAP   COLOR_SWAP(0xFFFF)
#define RED_SWAP     COLOR_SWAP(0xF800)
#define GREEN_SWAP   COLOR_SWAP(0x07E0)
#define BLUE_SWAP    COLOR_SWAP(0x001F)
#define YELLOW_SWAP  COLOR_SWAP(0xFFE0)
#define CYAN_SWAP    COLOR_SWAP(0x07FF)
#define MAGENTA_SWAP COLOR_SWAP(0xF81F)

// Use Arduino_GFX built-in canvas for flicker-free drawing
Arduino_PimoroniPAR8 *bus;
Arduino_ST7789_Parallel *display;
Arduino_Canvas *gfx;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== ST7789 FPS Benchmark ===");
  
  // Create bus and display
  bus = new Arduino_PimoroniPAR8();
  display = new Arduino_ST7789_Parallel(bus, GFX_NOT_DEFINED, 0, false, 320, 240);
  
  if(!display->begin()) {
    Serial.println("Display init failed!");
    while(1);
  }
  
  // Create canvas (framebuffer) - Arduino_GFX built-in!
  gfx = new Arduino_Canvas(320, 240, display);
  
  if(!gfx->begin()) {
    Serial.println("Canvas init failed!");
    while(1);
  }
  
  Serial.println("Display and canvas initialized!");
  
  // Set backlight
  display->setBacklight(255);
}

void loop() {
  static uint32_t frame = 0;
  static uint32_t framecount = 0;
  static uint32_t fps = 0;
  static uint32_t next_fps_time = millis() + 1000;
  
  // Draw to canvas (in RAM - no flicker)
  gfx->fillScreen(BLACK_SWAP);
  
  gfx->setCursor(10, 10);
  gfx->setTextColor(WHITE_SWAP);
  gfx->setTextSize(2);
  gfx->print("Frame: ");
  gfx->print(frame);
  gfx->print(" FPS: ");
  gfx->println(fps);
  
  gfx->drawRect(50, 50, 100, 100, RED_SWAP);
  gfx->fillCircle(100, 100, 30, GREEN_SWAP);
  gfx->drawLine(0, 0, 319, 239, BLUE_SWAP);
  
  int x = (frame * 2) % 320;
  gfx->fillRect(x, 150, 50, 50, YELLOW_SWAP);
  gfx->fillCircle(x+25, 150+25, 20, COLOR_SWAP(gfx->color565(0,0,255)));
  // Flush canvas to display (one fast update - no flicker!)
  gfx->flush();
  
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

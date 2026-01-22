# omoroni_explorer_rp2350_experiments
experiments / test code i used for [Pimoroni Explorer (rp2350)](https://shop.pimoroni.com/products/explorer?variant=42092697845843)

## pimoroni_explorer_display_arduinogfx
provides Arduino_PimoroniPAR8 and Arduino_ST7789_Parallel drivers to be used with arduinogfx so we can use the display from the board in arduino ide. 
It has been created with the help of claude.ai and is based on the display driver from pimoroni itself (https://github.com/pimoroni/pimoroni-pico/tree/main/drivers/st7789)
It seemed to work fine with my testings

The following libraries are required for this example to compile:
- **[arduino_pico](https://github.com/earlephilhower/arduino-pico)**: to be able to use the pimoroni explorer board in arduino ide
- **Arduino_GFX_Library**: for doing actual gfx drawing
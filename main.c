#include <Arduino.h>

#include "I2C.h"
#include "Micro_SSD1306.h"

Micro_SSD1306(oled, -1); // -1 means no reset pin

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Mini_SSD1306.h!");
#endif

void setup()
{
    // Initialize with the I2C addr 0x3C. Some displays use 0x3D instead.
    oled_begin(SSD1306_SWITCHCAPVCC, 0x3C, 0);
    //oled_initPages();
    oled_clearPages();
    oled_setCursor(0, 0);
    oled_dim(true);
    // oled_startscrollright(0x00, 0x0F); // Check if communication works
}

static uint8_t count = 0;

static const char *helloWorld = "HelloWorld!    ";

void loop()
{
    count++;
    delay(1000);

    oled_setCursor(0, 0);
    oled_printString(helloWorld);
    delay(3);

    // oled_printChar(count);
    oled_setCursor(0, 3);
    delay(3);

    // // display charmap of 94 chars
    for (char i = 0; i < 94; i++)
    {
        oled_printChar(i + 0x20);
    }
}

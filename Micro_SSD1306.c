/*
 * This is an example for using the i2c library with a monochrome OLED
 * display based on SSD1306 drivers.
 *
 * The display has 128x64 pixel and uses only SCL and SDA for communication,
 * there is no reset pin.
 *
 * The framebuffer needs to be kept in RAM as reading the display is not
 * supported by the driver chips. Since the STM8S103F3 has only 1kB RAM
 * total, we will see the stack contents in the lower part of the display
 * as a wild bit pattern. Using drawPixel() on this memory would mess up
 * the stack contents and would result in an immediate crash. So don't
 * use the lower lines on low memory devices!
 *
 * This code is a stripped down version of the Adafruit_SSD1306 library
 * adoped for use with the STM8S.
 *
 * This library supports only I2C displays using the I2C library, all SPI
 * related code is removed.
 *
 * All dependencies on the Adafruit_GFX library are removed. This simple
 * code only supports pixel based operations using drawPixel().
 *
 * 2017 modified for the STM8S by Michael Mayer
 */

/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/

#define USE_WIRE 0

#if USE_WIRE
#include <Wire.h>
#define i2c_begin() Wire.begin()
#else
#include <I2C.h>
//# define i2c_begin()		I2c.begin()
//# define i2c_write(A,C,D)	I2c.write(A,C,D)
//# define i2c_write_sn(A,C,B,N)	I2c.write(A,C,B,N)
#define i2c_begin() I2C_begin()
#define i2c_write(A, C, D) I2C_write_c(A, C, D)
#define i2c_write_sn(A, C, B, N) I2C_write_sn(A, C, B, N)
#endif
#include "Micro_SSD1306.h"

#include "LCD_font_5x7.h"

// private:
static int8_t _i2caddr, _vccstate, sid, sclk, dc, rst, cs;
//  static void fastSPIwrite(uint8_t c);

static boolean hwSPI;
#ifdef HAVE_PORTREG
static PortReg *mosiport, *clkport, *csport, *dcport;
static PortMask mosipinmask, clkpinmask, cspinmask, dcpinmask;
#endif

#define ssd1306_swap(a, b) \
  {                        \
    int16_t t = a;         \
    a = b;                 \
    b = t;                 \
  }
#define rotation (0)
#define getRotation() (0)

static void ssd1306_command(uint8_t c);
static void ssd1306_data(uint8_t d);

// initializer for I2C - we only indicate the reset pin!
void Micro_SSD1306_init(int8_t reset)
{
  sclk = dc = cs = sid = -1;
  rst = reset;
}

#if 1
void Micro_SSD1306_begin(uint8_t vccstate, uint8_t i2caddr, bool reset)
{
  _vccstate = vccstate;
  _i2caddr = i2caddr;

  // set pin directions
  {
    // I2C Init
    i2c_begin();
  }
  if ((reset) && (rst >= 0))
  {
    // Setup reset pin direction (used by both SPI and I2C)
    pinMode(rst, OUTPUT);
    digitalWrite(rst, HIGH);
    // VDD (3.3V) goes high at start, lets just chill for a ms
    delay(1);
    // bring reset low
    digitalWrite(rst, LOW);
    // wait 10ms
    delay(10);
    // bring out of reset
    digitalWrite(rst, HIGH);
    // turn on VCC (9V?)
  }

  // Init sequence
  ssd1306_command(SSD1306_DISPLAYOFF);         // 0xAE
  ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV); // 0xD5
  ssd1306_command(0x80);                       // the suggested ratio 0x80

  ssd1306_command(SSD1306_SETMULTIPLEX); // 0xA8
  ssd1306_command(SSD1306_LCDHEIGHT - 1);

  ssd1306_command(SSD1306_SETDISPLAYOFFSET);   // 0xD3
  ssd1306_command(0x0);                        // no offset
  ssd1306_command(SSD1306_SETSTARTLINE | 0x0); // line #0
  ssd1306_command(SSD1306_CHARGEPUMP);         // 0x8D
  if (vccstate == SSD1306_EXTERNALVCC)
  {
    ssd1306_command(0x10);
  }
  else
  {
    ssd1306_command(0x14);
  }
  ssd1306_command(SSD1306_MEMORYMODE); // 0x20
  ssd1306_command(0x00);               // 0x0 act like ks0108
  ssd1306_command(SSD1306_SEGREMAP | 0x1);
  ssd1306_command(SSD1306_COMSCANDEC);

#if defined SSD1306_128_32
  ssd1306_command(SSD1306_SETCOMPINS); // 0xDA
  ssd1306_command(0x02);
  ssd1306_command(SSD1306_SETCONTRAST); // 0x81
  ssd1306_command(0x8F);

#elif defined SSD1306_128_64
  ssd1306_command(SSD1306_SETCOMPINS); // 0xDA
  ssd1306_command(0x12);
  ssd1306_command(SSD1306_SETCONTRAST); // 0x81
  if (vccstate == SSD1306_EXTERNALVCC)
  {
    ssd1306_command(0x9F);
  }
  else
  {
    ssd1306_command(0xCF);
  }

#elif defined SSD1306_96_16
  ssd1306_command(SSD1306_SETCOMPINS);  // 0xDA
  ssd1306_command(0x2);                 //ada x12
  ssd1306_command(SSD1306_SETCONTRAST); // 0x81
  if (vccstate == SSD1306_EXTERNALVCC)
  {
    ssd1306_command(0x10);
  }
  else
  {
    ssd1306_command(0xAF);
  }

#endif

  ssd1306_command(SSD1306_SETPRECHARGE); // 0xd9
  if (vccstate == SSD1306_EXTERNALVCC)
  {
    ssd1306_command(0x22);
  }
  else
  {
    ssd1306_command(0xF1);
  }
  ssd1306_command(SSD1306_SETVCOMDETECT); // 0xDB
  ssd1306_command(0x40);
  ssd1306_command(SSD1306_DISPLAYALLON_RESUME); // 0xA4
  ssd1306_command(SSD1306_NORMALDISPLAY);       // 0xA6

  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(SSD1306_DISPLAYON); //--turn on oled panel
}
#endif

void Micro_SSD1306_invertDisplay(uint8_t i)
{
  if (i)
  {
    ssd1306_command(SSD1306_INVERTDISPLAY);
  }
  else
  {
    ssd1306_command(SSD1306_NORMALDISPLAY);
  }
}

static void ssd1306_command(uint8_t c)
{
  {
  // I2C
#if USE_WIRE
    uint8_t control = 0x00; // Co = 0, D/C = 0
    Wire.beginTransmission(_i2caddr);
    Wire.write(control);
    Wire.write(c);
    Wire.endTransmission();
#else
    i2c_write(_i2caddr, 0x00, c);
#endif
  }
}

// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Micro_SSD1306_startscrollright(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Micro_SSD1306_startscrollleft(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Micro_SSD1306_startscrolldiagright(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Micro_SSD1306_startscrolldiagleft(uint8_t start, uint8_t stop)
{
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void Micro_SSD1306_stopscroll(void)
{
  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

// Dim the display
// dim = true: display is dimmed
// dim = false: display is normal
void Micro_SSD1306_dim(boolean dim)
{
  uint8_t contrast;

  if (dim)
  {
    contrast = 0; // Dimmed display
  }
  else
  {
    if (_vccstate == SSD1306_EXTERNALVCC)
    {
      contrast = 0x9F;
    }
    else
    {
      contrast = 0xCF;
    }
  }
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(contrast);
}

void Micro_SSD1306_initPages(void)
{
  // set address mode
  ssd1306_command(SSD1306_MEMORYMODE);
  ssd1306_command(0x00);
}

void ssd1306_data(uint8_t data)
{
  i2c_write(_i2caddr, 0x40, data);
}

void setPage(uint8_t page)
{
  ssd1306_command(0xB0 | page);
}

void setColumn(uint8_t column)
{
  ssd1306_command(SSD1306_SETLOWCOLUMN | (column & 0x0F));
  ssd1306_command(SSD1306_SETHIGHCOLUMN | ((column & 0xF0) >> 4));
}

void Micro_SSD1306_setCursor(uint8_t column, uint8_t page)
{
  setPage(page);
  setColumn(column);
}

void Micro_SSD1306_clearPages(void)
{
  setPage(0);
  setColumn(0);

  for (uint16_t p = 0; p < (WIDTH) * (HEIGHT / 8); p++)
  {
    ssd1306_data(0b00000000);
  }
}

void Micro_SSD1306_printString(char *s)
{
  for (uint8_t i = 0; i < strlen(s); i++)
  {
    Micro_SSD1306_printChar(s[i]);
  }
}

void Micro_SSD1306_printChar(char c)
{
  // send char segment wise
  for (uint8_t i = 0; i < 5; ++i)
  {
    // fetch char from font map, and convert unknown to '?'
    ssd1306_data(font_5x7[((c < 0x20) ? 0x1F : (c - 0x20)) * 5 + i]);
  }
  ssd1306_data(0x00); // font spacing
  delay(3);
}

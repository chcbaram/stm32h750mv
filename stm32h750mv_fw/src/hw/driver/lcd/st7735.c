/*
 * st7735.c
 *
 *  Created on: 2020. 10. 24.
 *      Author: baram
 */




#include "spi.h"
#include "gpio.h"
#include "lcd/st7735.h"
#include "lcd/st7735_regs.h"


#define _PIN_DEF_BKT    0
#define _PIN_DEF_DC     1
#define _PIN_DEF_CS     2

#define MADCTL_MY       0x80
#define MADCTL_MX       0x40
#define MADCTL_MV       0x20
#define MADCTL_ML       0x10
#define MADCTL_RGB      0x00
#define MADCTL_BGR      0x08
#define MADCTL_MH       0x04



static uint8_t   spi_ch = _DEF_SPI1;

static const int32_t _width  = HW_LCD_WIDTH;
static const int32_t _height = HW_LCD_HEIGHT;
static void (*frameCallBack)(void) = NULL;
volatile static bool  is_write_frame = false;

const uint32_t colstart = 1;
const uint32_t rowstart = 26;

static void writecommand(uint8_t c);
static void writedata(uint8_t d);
static void st7735InitRegs(const uint8_t *addr);
static void st7735FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
static void st7735SetRotation(uint8_t m);
static bool st7735Reset(void);


#define DELAY 0x80
static const uint8_t
  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      100,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      100,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVON , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color
#if 1

  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 80-1,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 160-1 },          //     XEND = 159
#endif

#if HW_LCD_HEIGHT == 128
  Rcmd2green144[] = {              // Init for 7735R, part 2 (green 1.44 tab)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00,   0,             //     XSTART = 0
      0x00, 127,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00,   0,             //     XSTART = 0
      0x00, 127 },           //     XEND = 127
#endif
  Rcmd3[] = {                 // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Magical unicorn dust, 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Sparkles and rainbows, 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      10 };                  //     100 ms delay



static void TransferDoneISR(void)
{
  if (is_write_frame == true)
  {
    is_write_frame = false;
    gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);

    if (frameCallBack != NULL)
    {
      frameCallBack();
    }
  }
}


bool st7735Init(void)
{
  bool ret;

  ret = st7735Reset();

  return ret;
}

bool st7735InitDriver(lcd_driver_t *p_driver)
{
  p_driver->init = st7735Init;
  p_driver->reset = st7735Reset;
  p_driver->setWindow = st7735SetWindow;
  p_driver->getWidth = st7735GetWidth;
  p_driver->getHeight = st7735GetHeight;
  p_driver->setCallBack = st7735SetCallBack;
  p_driver->sendBuffer = st7735SendBuffer;
  return true;
}

bool st7735Reset(void)
{
  spiBegin(spi_ch);
  spiSetBitOrder(spi_ch, SPI_FIRSTBIT_MSB);
  spiSetClockDivider(spi_ch, SPI_BAUDRATEPRESCALER_2);
  spiSetDataMode(spi_ch, SPI_MODE0);

  spiAttachTxInterrupt(spi_ch, TransferDoneISR);

  gpioPinWrite(_PIN_DEF_BKT, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_DC,  _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS,  _DEF_HIGH);
  delay(10);

  st7735InitRegs(Rcmd1);
  st7735InitRegs(Rcmd2red);
  st7735InitRegs(Rcmd3);
#if 0
  writecommand(ST7735_SWRESET);
  delay(120);
  /*exit sleep*/
  writecommand(ST7735_SLPOUT);
  delay(120);

  /*pixel format*/
  writecommand(ST7735_COLMOD);
  writedata(0x55);

  /*display on*/
  writecommand(ST7735_DISPON);
  delay(10);
#endif
  st7735SetRotation(1);
  st7735FillRect(0, 0, HW_LCD_WIDTH, HW_LCD_HEIGHT, black);

  gpioPinWrite(_PIN_DEF_BKT, _DEF_LOW);
  return true;
}

uint16_t st7735GetWidth(void)
{
  return HW_LCD_WIDTH;
}

uint16_t st7735GetHeight(void)
{
  return HW_LCD_HEIGHT;
}

void writecommand(uint8_t c)
{
  gpioPinWrite(_PIN_DEF_DC, _DEF_LOW);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer8(spi_ch, c);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

void writedata(uint8_t d)
{
  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiTransfer8(spi_ch, d);

  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

void st7735InitRegs(const uint8_t *addr)
{
  uint8_t  numCommands, numArgs;
  uint16_t ms;


  numCommands = *addr;   // Number of commands to follow
  addr++;
  while(numCommands--)
  {
    writecommand(*addr); //   Read, issue command
    addr++;
    numArgs  = *addr;    //   Number of args to follow
    addr++;
    ms       = numArgs & DELAY;          //   If hibit set, delay follows args
    numArgs &= ~DELAY;                   //   Mask out delay bit
    while(numArgs--)
    {                   //   For each argument...
      writedata(*addr);  //     Read, issue argument
      addr++;
    }

    if (ms)
    {
      ms = *addr; // Read post-command delay time (ms)
      addr++;
      if(ms == 255) ms = 500;     // If 255, delay for 500 ms
      delay(ms);
    }
  }
}

void st7735SetRotation(uint8_t mode)
{
  writecommand(ST7735_MADCTL);

  switch (mode)
  {
   case 0:
     writedata(MADCTL_MX | MADCTL_MY | MADCTL_BGR);
     break;

   case 1:
     writedata(MADCTL_MY | MADCTL_MV | MADCTL_BGR);
     break;

  case 2:
    writedata(MADCTL_BGR);
    break;

   case 3:
     writedata(MADCTL_MX | MADCTL_MV | MADCTL_BGR);
     break;
  }
}

void st7735SetWindow(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
  spiSetBitWidth(spi_ch, 8);

  writecommand(ST7735_CASET); // Column addr set
  writedata(0x00);
  writedata(x0+colstart);     // XSTART
  writedata(0x00);
  writedata(x1+colstart);     // XEND

  writecommand(ST7735_RASET); // Row addr set
  writedata(0x00);
  writedata(y0+rowstart);     // YSTART
  writedata(0x00);
  writedata(y1+rowstart);     // YEND

  writecommand(ST7735_RAMWR); // write to RAM
}

void st7735FillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
  uint16_t line_buf[w];

  // Clipping
  if ((x >= _width) || (y >= _height)) return;

  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }

  if ((x + w) > _width)  w = _width  - x;
  if ((y + h) > _height) h = _height - y;

  if ((w < 1) || (h < 1)) return;


  st7735SetWindow(x, y, x + w - 1, y + h - 1);
  spiSetBitWidth(spi_ch, 16);

  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  for (int i=0; i<w; i++)
  {
    line_buf[i] = color;
  }
  for (int i=0; i<h; i++)
  {
    spiDmaTransfer(_DEF_SPI1, (void *)line_buf, w, 10);
  }
  gpioPinWrite(_PIN_DEF_CS, _DEF_HIGH);
}

bool st7735SendBuffer(uint8_t *p_data, uint32_t length, uint32_t timeout_ms)
{
  is_write_frame = true;

  spiSetBitWidth(spi_ch, 16);

  gpioPinWrite(_PIN_DEF_DC, _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS, _DEF_LOW);

  spiDmaTransfer(_DEF_SPI1, (void *)p_data, length, 0);
  return true;
}

bool st7735SetCallBack(void (*p_func)(void))
{
  frameCallBack = p_func;

  return true;
}

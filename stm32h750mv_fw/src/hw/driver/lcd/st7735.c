/*
 * st7735.c
 *
 *  Created on: 2020. 10. 24.
 *      Author: baram
 */




#include "spi.h"
#include "gpio.h"
#include "lcd/st7735_regs.h"


#define _PIN_DEF_BKT    0
#define _PIN_DEF_DC     1
#define _PIN_DEF_CS     2


static uint8_t   spi_ch = _DEF_SPI1;


static void writecommand(uint8_t c);
static void writedata(uint8_t d);
static void st7735InitRegs(const uint8_t *addr);


#define DELAY 0x80
static const uint8_t
  Rcmd1[] = {                 // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
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
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      0xC8,                   //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color
#if 1
  Rcmd2green[] = {            // Init for 7735R, part 2 (green tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x02,             //     XSTART = 0
      0x00, 0x7F+0x02,        //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x01,             //     XSTART = 0
      0x00, 0x9F+0x01 },      //     XEND = 159

  Rcmd2red[] = {              // Init for 7735R, part 2 (red tab only)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F },           //     XEND = 159
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
      100 };                  //     100 ms delay





bool st7735Init(void)
{
  spiBegin(spi_ch);
  spiSetBitOrder(spi_ch, SPI_FIRSTBIT_MSB);
  spiSetClockDivider(spi_ch, SPI_BAUDRATEPRESCALER_256);
  spiSetDataMode(spi_ch, SPI_MODE0);

  //spiAttachTxInterrupt(spi_ch, TransferDoneISR);

  gpioPinWrite(_PIN_DEF_BKT, _DEF_LOW);
  gpioPinWrite(_PIN_DEF_DC,  _DEF_HIGH);
  gpioPinWrite(_PIN_DEF_CS,  _DEF_HIGH);

  delay(10);


  writecommand(ST7735_SWRESET);
  delay(120);
  writecommand(ST7735_SWRESET);
  delay(120);
  writecommand(ST7735_SLPOUT);
  delay(150);


  writecommand(ST7735_NORON);
  writedata(0x00);
  delay(10);
  writecommand(ST7735_DISPON);
  writedata(0x00);

  st7735InitRegs(Rcmd1);
  st7735InitRegs(Rcmd2red);
  st7735InitRegs(Rcmd3);

  return true;
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

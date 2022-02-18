
#include <stdio.h>
#include <sys/attribs.h> // __ISR macro
#include <xc.h>          // processor SFR definitions
#include "ST7789.h"
#include "font.h"
#include "spi.h"

#define PI 3.14159
// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF  // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use internal oscillator with pll
#pragma config FSOSCEN = OFF  // disable secondary oscillator
#pragma config IESO = OFF  // disable switching clocks
#pragma config POSCMOD = OFF  // internal RC
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576// use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF  // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0     // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF  // allow multiple reconfigurations



void drawBar(unsigned short x, unsigned short y, unsigned short index, unsigned short color);
void drawString(unsigned short x, unsigned short y, unsigned short color, char *m);
void drawChar(unsigned short x, unsigned short y, unsigned short color,unsigned char letter);

char m[100];

int main() {

  __builtin_disable_interrupts(); // disable interrupts while initializing
                                  // things

  // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
  __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

  // 0 data RAM access wait states
  BMXCONbits.BMXWSDRM = 0x0;

  // enable multi vector interrupts
  INTCONbits.MVEC = 0x1;

  // disable JTAG to get pins back
  DDPCONbits.JTAGEN = 0;

  // do your TRIS and LAT commands here
  TRISBbits.TRISB4 = 1; // B4 is input
  TRISAbits.TRISA4 = 0; // A4 is output
  LATAbits.LATA4 = 0;   // A4 is low

  initSPI();
  LCD_init();

  U1RXRbits.U1RXR = 0b0001; // U1RX is B6
  RPB7Rbits.RPB7R = 0b0001; // U1TX is B7

  // turn on UART3 without an interrupt
  U1MODEbits.BRGH = 0; // set baud to NU32_DESIRED_BAUD
  U1BRG = ((48000000 / 115200) / 16) - 1;

  // 8 bit, no parity bit, and 1 stop bit (8N1 setup)
  U1MODEbits.PDSEL = 0;
  U1MODEbits.STSEL = 0;

  // configure TX & RX pins as output & input pins
  U1STAbits.UTXEN = 1;
  U1STAbits.URXEN = 1;

  // enable the UART3
  U1MODEbits.ON = 1;

  __builtin_enable_interrupts();
  LCD_clearScreen(BLACK);

  char m[100];
  int i = 0;
  while (1) {
    while(i<100) { //use while loop to increment i "faster"
      unsigned int t_start = _CP0_GET_COUNT(); 
      sprintf(m, "Hello world %d!", i);
      drawString(28,32,WHITE,m); //Writing text out on screen
      i++;
      unsigned int t_end = _CP0_GET_COUNT(); // remember time
      float fps = 24000000 / (float)(t_end-t_start); // get FPS from delay and coreticks
      sprintf(m, "FPS = %.1f", fps);
      drawString(53,150,WHITE,m);  //writing fps
      if (i<50){
          drawBar(53,90,i,RED);  //progress bar - <50%
      }else{
          drawBar(53,90,i,GREEN); //progress bar - >50%          
      }
      if(i==100){
          i=0;
      }
    }
  }
}

// drawChar function
void drawChar(unsigned short x, unsigned short y, unsigned short color,unsigned char letter) {
  unsigned short i,j;
  for (i=0;i<5;i++) {   // x 
    for (j=0; j<8;j++) { // y 
      if (ASCII[letter-0x20][i] >> j & 0b1 ) {
        LCD_drawPixel(x+i,y+j,color);
      } else {
        LCD_drawPixel(x+i,y+j,BLACK);
      }
    }
  }
}

// drawString function
void drawString(unsigned short x, unsigned short y, unsigned short color, char *m) {
  unsigned short i;
  for (i=0;i<strlen(m);i++) {
    drawChar(x+i*5,y,color,m[i]);
  }
}

// drawBar function
void drawBar(unsigned short x, unsigned short y, unsigned short index, unsigned short color) {
  unsigned short i,j;
  for (i=0;i<100;i++) { //length of 100
    for (j=0;j<10;j++) { //height of 10
        if (i < index) {
          LCD_drawPixel(x+i,y+j,color);
        } else {
          LCD_drawPixel(x+i,y+j,WHITE);
        }           
    }
  }
}

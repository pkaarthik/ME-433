#include <stdio.h>
#include <sys/attribs.h> // __ISR macro
#include <xc.h>          // processor SFR definitions
#include "i2c_master_noint.h"
#include "ST7789.h"
#include "imu.h"
#include "spi.h"
#include "font.h"
#include <math.h>

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
#pragma config IOL1WAY = OFF  // allow multiple reconfigurations/* ************************************************************************** */

void inclinebars(signed short valuex, signed short valuey) ;

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
  
  
  i2c_master_setup();
  imu_setup();
  initSPI();
  LCD_init();
  LCD_clearScreen(BLACK); 
  
  __builtin_enable_interrupts();
  
  
    signed short data[7];
    char m[100];

    while(1){
      _CP0_SET_COUNT(0);
      LATAINV = 0b10000; //heartbeat 
      
      while (_CP0_GET_COUNT() < 48000000 / 2000000 ) {} 
      
      //read imu 
      imu_read(IMU_OUT_TEMP_L,data,7);
        
      //print imu values
      sprintf(m, "Temp: %4d", data[0]);
      drawString(30, 10, WHITE, m);
      sprintf(m, "Gyroscope Vel.: %4d %4d %4d", data[1], data[2], data[3]);
      drawString(30, 30, WHITE, m);
      sprintf(m, "Accel: %4d %4d %4d", data[4], data[5], data[6]);
      drawString(30, 50, WHITE, m);
      
      //print incline bars  
      inclinebars(data[5],data[4]);
      
      
      //fps calculation and printing
      unsigned int t_end = _CP0_GET_COUNT(); 
      float fps = 24000000 / (float)(t_end);
      
      sprintf(m, "FPS: %.2f", fps);
      drawString(30,190,WHITE,m);  //writing fps
    
      
  }
}

//Use logic in drawBar function in ST7789.c to write a function that draws both bars.
void inclinebars(signed short valuex, signed short valuey) {
  int i;
  int j;
  
  
  //limit imu accel values to 10000 in x and y
  if (valuex > 10000){
      valuex = 10000;
  } 
  if (valuey > 10000){
      valuey = 10000;
  } 
  if (valuex < -10000){
      valuex = -10000;
  } 
  if (valuey < -10000){
      valuey = -10000;
  } 
  
  // scale imu acc values into the length of the progress bars.
  float xacc = (valuex / 10000.0) * 80;
  float yacc = (valuey / 10000.0) * 80;
  
  //4 cases to check: x>0 and i>abs(xacc), x>0 and i<abs(xacc), x<0 and i>abs(xacc), x<0 and i<abs(xacc) 
  //if i>abs(xacc), those pixels are empty, otherwise they are filled in. x sign determines which way bar grows.
  for (i = 0; i < 80; i++) {  // length - 80
    for (j = 0; j < 10; j++) { // width - 10
      //x bar
      if (xacc > 0) {             // bar grows to left and right
        if (i > fabs(xacc)) {     
          LCD_drawPixel(140-i, 140+j, BLACK);
        } else {
          LCD_drawPixel(140-i, 140+j, GREEN);
        }
      } else {             
        if (i > fabs(xacc)) { 
          LCD_drawPixel(140+i, 140+j, BLACK);
        } else {
          LCD_drawPixel(140+i, 140+j, GREEN);
        }
      }
      //y bar 
      if (yacc > 0) {             // bar grows up and down
        if (i > fabs(yacc)) {     
          LCD_drawPixel(140+j, 140-i, BLACK);
        } else {
          LCD_drawPixel(140+j, 140-i, RED);
        }
      } else {             
        if (i > fabs(yacc)) { 
          LCD_drawPixel(140+j, 140+i, BLACK);
        } else {
          LCD_drawPixel(140+j, 140+i, RED);
        }
      }
    }
  }
}
  



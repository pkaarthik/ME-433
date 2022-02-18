#include <stdio.h>
#include <sys/attribs.h> // __ISR macro
#include <xc.h>          // processor SFR definitions
#include "ws2812b.h"


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
  
  
  ws2812b_setup(); //enable neopixels
  __builtin_enable_interrupts();
  
  LATBbits.LATB11 = 0; //turn off lights before running code
  TMR2=0;
  while (TMR2 < 24000) {;}
  
  wsColor led1, led2, led3, led4, led5;
  float sat = 0.8;
  float bright = 0.5;
  
  float hue1 = 0;  //define hues for 5 LEDs - these change in while loop to create a cascading pattern
  float hue2 = 72;
  float hue3 = 144;
  float hue4 = 216;
  float hue5 = 288;

  
  wsColor red = {255, 0, 0};  //define base colors for us to test that colors are working
  wsColor green = {0, 255, 0}; 
  wsColor blue = {0, 0, 255};
  wsColor white = {255, 255, 255};
  wsColor black = {0,0,0};
  wsColor arr_col[5] = {red, green, blue, white, black};
    
  ws2812b_setColor(arr_col, 5);
    
    
    while (1) {
        _CP0_SET_COUNT(0);
        LATAINV = 0b10000; //heartbeat
        
        led1 = HSBtoRGB(hue1, sat, bright);  //use HSBtoRGB to set LED colors for each LED
        led2 = HSBtoRGB(hue2, sat, bright);
        led3 = HSBtoRGB(hue3, sat, bright);
        led4 = HSBtoRGB(hue4, sat, bright);
        led5 = HSBtoRGB(hue5, sat, bright);
        
        
        hue1=hue1+18;  //increment each hue evenly so as to create the pattern
        hue2=hue2+18;
        hue3=hue3+18;
        hue4=hue4+18;
        hue5=hue5+18;
        
        if(hue1>360){   //hue ranges from 0 to 360 - check that it doesn't exceed this
            hue1=0;
        }
        if(hue2>360){
            hue2=0;
        }
        if(hue3>360){
            hue3=0;
        }
        if(hue4>360){
            hue4=0;
        }
        if(hue5>360){
            hue5=0;
        }
        
        wsColor arr_col[5] = {led1, led2, led3, led4,led5}; 

        ws2812b_setColor(arr_col, 5);  //light those LEDs up!
         
        TMR2 = 0;
        while (TMR2 < 2400) {} //delay changing colors a little bit.
//        
        while (_CP0_GET_COUNT() < 48000000 / 40 ) {} //heartbeat timer
    }
      
  }




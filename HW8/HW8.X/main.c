#include <stdio.h>
#include <sys/attribs.h> // __ISR macro
#include <xc.h>          // processor SFR definitions
#include <stdlib.h>
#include "i2c_master_noint.h"

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

void readUART1(char *string, int maxLength);
void writeUART1(const char *string);
void delay();
void initSPI();
unsigned char spi_io(unsigned char o);
void setPin(unsigned char address, unsigned char reg, unsigned char value);
unsigned char readPin(unsigned char address, unsigned char reg);


int main() {

  __builtin_disable_interrupts(); // disable interrupts while initializing things

  // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
  __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

  // 0 data RAM access wait states
  BMXCONbits.BMXWSDRM = 0x0;

  // enable multi vector interrupts
  INTCONbits.MVEC = 0x1;

  // disable JTAG to get pins back
  DDPCONbits.JTAGEN = 0;

  // TRIS and LAT commands
  //B4 is input
  TRISBbits.TRISB4 = 1;

  // A4 is output
  TRISAbits.TRISA4 = 0;

  // A4 is off initially
  LATAbits.LATA4 = 0;
  
  initSPI();

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

  // enable the UART
  U1MODEbits.ON = 1;
  
  i2c_master_setup();
  __builtin_enable_interrupts();
  
  setPin(0b01000000, 0x00, 0x00);   //Set A as output
  setPin(0b01000000, 0x01, 0xFF);   //Set B as input
//  setPin(0b01000000, 0x14, 0xFF);   //turn on LED at IO expander
  
  unsigned char value;
  char m[100];
  unsigned short val = 0;
  unsigned char flag;
  while(1){

    _CP0_SET_COUNT(0); 
    LATAINV = 0b10000;  //Heartbeat LED connected to A4 on PIC

    while (_CP0_GET_COUNT() < 48000000/30) {
      ;
    } 
    
        flag = readPin(0b01000000, 0x13);
        if (flag == 0){  // If button is pushed, flag is zero
            setPin(0b01000000,0x14,0xFF); //turn on LED
        }
        else{
            setPin(0b01000000,0x14,0x00); //turn off LED
        }


  }
}
  
  
  
  
void setPin(unsigned char address, unsigned char reg, unsigned char value) {
    i2c_master_start(); //send start bit
    i2c_master_send(address); //send address with write bit
    i2c_master_send(reg); //send register to change
    i2c_master_send(value); //send value to change to
    i2c_master_stop(); //send stop bit
}


unsigned char readPin(unsigned char address,unsigned char reg) {
    unsigned char address2 = 0b01000001;
    unsigned char recv;
    i2c_master_start(); //send start bit
    i2c_master_send(address); //send address with write bit
    i2c_master_send(reg);//send register to read from
    i2c_master_restart(); // restart
    i2c_master_send(address2); //send address with read bit
    recv = i2c_master_recv(); //receive value from chip
    i2c_master_ack(1);  //acknowledge - 0 means chip expects another read, 1 means no more bytes needed
    i2c_master_stop();  //send stop bit
    
    return recv;
}
  
void delay(){
    _CP0_SET_COUNT(0); //set sysclk to zero. core timer runs at half the sysclk
    while (_CP0_GET_COUNT() < 0.5 * 24000000) {
        ;
    } 
}

// Read from UART1
// block other functions until you get a '\r' or '\n'
// send the pointer to your char array and the number of elements in the array
void readUART1(char * message, int maxLength) {
  char data = 0;
  int complete = 0, num_bytes = 0;
  // loop until you get a '\r' or '\n'
  while (!complete) {
    if (U1STAbits.URXDA) { // if data is available
      data = U1RXREG;      // read the data
      if ((data == '\n') || (data == '\r')) {
        complete = 1;
      } else {
        message[num_bytes] = data;
        ++num_bytes;
        // roll over if the array is too small
        if (num_bytes >= maxLength) {
          num_bytes = 0;
        }
      }
    }
  }
  // end the string
  message[num_bytes] = '\0';
}

// Write a character array using UART1
void writeUART1(const char * string) {
  while (*string != '\0') {
    while (U1STAbits.UTXBF) {
      ; // wait until tx buffer isn't full
    }
    U1TXREG = *string;
    ++string;
  }
}

// initialize SPI1
void initSPI() {
    // Pin B14 has to be SCK1
    // Turn of analog pins
    ANSELA=0;//1 for analog
    TRISAbits.TRISA0 = 0;
    LATAbits.LATA0 = 1;
    // Make A1 SDO1
    RPA1Rbits.RPA1R = 0b0011;
    // Make B5 SDI1
    SDI1Rbits.SDI1R = 0b0001;
    // setup SPI1
    SPI1CON = 0; // turn off the spi module and reset it
    SPI1BUF; // clear the rx buffer by reading from it
    SPI1BRG = 1000; // 1000 for 24kHz, 1 for 12MHz; // baud rate to 10 MHz [SPI1BRG = (48000000/(2*desired))-1]
    SPI1STATbits.SPIROV = 0; // clear the overflow bit
    SPI1CONbits.CKE = 1; // data changes when clock goes from hi to lo (since CKP is 0)
    SPI1CONbits.MSTEN = 1; // master operation
    SPI1CONbits.ON = 1; // turn on spi 
}


// send a byte via spi and return the response
unsigned char spi_io(unsigned char o) {
  SPI1BUF = o;
  while(!SPI1STATbits.SPIRBF) { // wait to receive the byte
    ;
  }
  return SPI1BUF;
}
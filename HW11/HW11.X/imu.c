#include "imu.h"
#include "i2c_master_noint.h"

void imu_setup() {
  unsigned char whoami = 0;
  
  // read from IMU_WHOAMI register

  whoami = readPin(IMU_ADDRESS, IMU_WHOAMI);

  if (whoami != 0b01101001) { // check i2c communication
    while (1) {
       _CP0_SET_COUNT(0);
      LATAINV = 0b10000; //heartbeat 
      
      while (_CP0_GET_COUNT() < 48000000 / 2000000 ) {}  

    }
  } 
  
  //initialize imu
  setPin(IMU_ADDRESS, IMU_CTRL1_XL,0b10000010);
  setPin(IMU_ADDRESS, IMU_CTRL2_G, 0b10001000);
  setPin(IMU_ADDRESS, IMU_CTRL3_C, 0b00000100);
  }

void imu_read(unsigned char reg, signed short * data, int len) {

    // read from imu with multiple pin read function. each piece of data takes 2 pin reads, so we need 2x the number of chars read.
    int x = len * 2, i;
    unsigned char imu_read[x];
    i2c_master_read_multiple(IMU_ADDRESS, reg, imu_read, x);
    
    // converting 8 bit char value to 16 short value
    for (i=0; i<len; i++) {
        data[i] = imu_read[(i*2) +1] << 8 | imu_read[(i*2)];
    }
} 
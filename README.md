# ISL-UTIL

Configuration files(...*....hex) are provided by hardware colleagues. 

When generating the configuration file they need to specify the channel(Configuration ID) to burn,the final effective Configuration ID is based on the hardware design.

sudo ./isl-uitl i2cbus slave-addr device [file-name]
    i2cbus: 
      the i2c channel host link to device, in SG2042 Pisces multi socket server:
            0: SG2042 CPU0 I2C0
            1: SG2042 CPU1 I2C1
    slave-addr:
      the addr slave address is base on hardware design, it is best to be told in advance by the hardware colleagues.
            0x5C: in SG2042 Pisces
    device:
      show the device you want to burn
            0: ISL68127
            1: ISL68224
    [file-name]: 
      the configuration hex file you want to burn.

Example:
  1. To inquire the power chip's device id, reversion id and the NVM slots(remaining number of programming times):

       sudo ./isl-util 0 0x5C 0

  2. In addition to the above functions, you want to burn the configuration hex file to power chip(upgrade the chip) :

       sudo ./isl-util 0 0x5C 0 file-name
    

/* 
 * File:   ACCEL.h
 * Author: Francisco
 *
 * Created on March 28, 2014, 10:06 AM
 */
#ifndef __ACCEL_H__
#define __ACCEL_H__

#define	ACCEL_H

#define WRITE              0x00
#define READ               0x80
#define MULTIBYTE_TRANSFER 0x40
#define BW_RATE_REGISTER   0x2c
#define NO_POWER_SAVING    0x00
#define HZ_100             0x09
#define DATA_FORMAT_REGISTER  0x31
#define DATA_FORMAT_VALUE  0x0b
#define FIFO_CONTROL_REGISTER 0x38
#define FIFO_CONTROL_VALUE 0xbf
#define DATA_REG           0x32
#define POWER_CONTROL_REGISTER   0x2d
#define POWER_CONTROL_VALUE   0x08
#define INT_ENABLE_REGISTER   0x2e
#define INT_ENABLE_VALUE   0x02
#define INT_MAP_REGISTER   0x2f
#define INT_MAP_VALUE      0xfd
#define SIGN_BIT           9
#define ACCEL_MASK         0x01ff
#define CS      LATDbits.LATD0
#define CS_TRIS   TRISDbits.TRISD0
          
#define BIT_22                       (1 << 22)
#define BIT_21                       (1 << 21)
#define BIT_20                       (1 << 20)
#define BIT_19                       (1 << 19)
#define BIT_18                       (1 << 18)
#define BIT_17                       (1 << 17)
#define BIT_16                       (1 << 16)
#define BIT_15                       (1 << 15)
#define BIT_14                       (1 << 14)
#define BIT_13                       (1 << 13)
#define BIT_12                       (1 << 12)
#define BIT_11                       (1 << 11)
#define BIT_10                       (1 << 10)
#define BIT_9                        (1 << 9)
#define BIT_8                        (1 << 8)
#define BIT_7                        (1 << 7)
#define BIT_6                        (1 << 6)
#define BIT_5                        (1 << 5)
#define BIT_4                        (1 << 4)
#define BIT_3                        (1 << 3)
#define BIT_2                        (1 << 2)
#define BIT_1                        (1 << 1)
#define BIT_0                        (1 << 0)


int8_t read_accel_register(int8_t reg);
void read_accel(void) ;
void setup_accelerometer(void);
void standby_mode(void);
void measurement_mode(void);
int read_power_mode(void);
void Rtccinit(void);
void  Portsinit(void);
void PinMapping(void);
void EnableSPIacel(void);
BYTE inc_BCD(BYTE);
void initAdc1(void);
void create_initial_cond(void);
void toggle_led1(void);
void read_initial_cond(void);


#endif
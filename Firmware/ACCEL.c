/*
 accel.c*/


#include <Rtcc.h>
#include <ACCEL.h>
#include <stdint.h>
#include <stdio.h>
#include <libpic30.h>
#include "HardwareProfile.h"
#include "p24Fxxxx.h"





int8_t read_accel_register(int8_t reg) {
    int8_t register_value;
    CS=0;
    __delay_us(10);
    SPI2BUF = (READ | reg);
    //putcSPI2(READ | reg);
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    SPI2BUF = (0x00);
    while(SPI2STATbits.SPIRBF == 0);
    //register_value = getcSPI2();
    register_value=SPI2BUF;

    __delay_us(10);
    CS=1;
    return (register_value);

}




void setup_accelerometer(void) {
   // set interrupt enable register
   int A;
   CS=0;
   __delay_us(1);
   SPI2BUF = (WRITE | INT_ENABLE_REGISTER);
    while(SPI2STATbits.SPIRBF == 0);
    A=SPI2BUF;

    SPI2BUF = (INT_ENABLE_VALUE);
    while(SPI2STATbits.SPIRBF == 0);
    A=SPI2BUF;
   __delay_us(1);
   CS=1;

   // set interrupt map register
   CS=0;
  __delay_us(1);
    SPI2BUF = (WRITE | INT_MAP_REGISTER);
    while(SPI2STATbits.SPIRBF == 0);
    A=SPI2BUF;

   SPI2BUF = (INT_MAP_VALUE);
    while(SPI2STATbits.SPIRBF == 0);
    A=SPI2BUF;

   __delay_us(1);
   CS=1;

   // set data format register appropriately

   CS=0;
   __delay_us(1);
   SPI2BUF = (WRITE | DATA_FORMAT_REGISTER);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;

   SPI2BUF = (DATA_FORMAT_VALUE);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;
   __delay_us(1);
   CS=1;

   // set rate register to 6.25Hz conversion, no power saving mode
   CS=0;
   __delay_us(1);
   SPI2BUF = (WRITE | BW_RATE_REGISTER);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;

   SPI2BUF = (NO_POWER_SAVING | HZ_100);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;
   __delay_us(1);
   CS=1;

   // set FIFO control register
   CS=0;
   __delay_us(1);
   SPI2BUF = (WRITE | FIFO_CONTROL_REGISTER);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;

   SPI2BUF =   (FIFO_CONTROL_VALUE);
   while(SPI2STATbits.SPIRBF == 0);
   A=SPI2BUF;

   __delay_us(1);
   CS=1;

   // set power control register appropriately
   CS=0;
  __delay_us(1);
  SPI2BUF = (WRITE | POWER_CONTROL_REGISTER);
  while(SPI2STATbits.SPIRBF == 0);
  A=SPI2BUF;

  SPI2BUF = (POWER_CONTROL_VALUE);
  while(SPI2STATbits.SPIRBF == 0);
  A=SPI2BUF;

   __delay_us(1);
   CS=1;
}

char read_accel(rtccTimeDate *phora, int msec) {
     int8_t counter=0,A;
      int16_t string_length=0;
      int16_t x_accel_low,y_accel_low,z_accel_low,x_accel_high,y_accel_high,z_accel_high;
    uint16_t x_accel=0, y_accel=0, z_accel=0;
      rtccTimeDate RtccTimeDateVala;
    // to do hacer esto con puntero para que este disponible en main.c
    char output_data[1000];

    while(counter<=31){
        CS=0;
        asm("nop");
        asm("nop");
        SPI2BUF =  (READ | MULTIBYTE_TRANSFER | DATA_REG);
        while(SPI2STATbits.SPIRBF == 0);
        A=SPI2BUF;

        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        x_accel_low=SPI2BUF;
        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        x_accel_high = SPI2BUF; // x high byte

        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        y_accel_low=SPI2BUF;
        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        y_accel_high = SPI2BUF; // y high byte


        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        z_accel_low=SPI2BUF;
        SPI2BUF = (0x00);
        while(SPI2STATbits.SPIRBF == 0);
        z_accel_high = SPI2BUF; // z high byte

        asm("nop");
        asm("nop");
        CS=1;

        x_accel = (x_accel_high << 8) | x_accel_low ;
        y_accel = (y_accel_high << 8) | y_accel_low ;
        z_accel = (z_accel_high << 8) | z_accel_low ;

        string_length += sprintf(output_data+string_length,"%4i,%4i,%4i,%x,%x,%x,%u\n",
        x_accel,y_accel,z_accel,RtccTimeDateVala.f.hour,RtccTimeDateVala.f.min,RtccTimeDateVala.f.sec,msec);

        counter++;
        //a=counter & 0x01;
        //if(a==0)
        //    msec+=312;
        //else
        //    msec+=313;
        msec+=625;

        if (msec>=31250){
            msec-=31250;
            if(RtccTimeDateVala.f.sec<=0x58)
                RtccTimeDateVala.f.sec=inc_BCD(RtccTimeDateVala.f.sec);
            else{
                RtccTimeDateVala.f.sec=0;
                if(RtccTimeDateVala.f.min<=0x58)
                    RtccTimeDateVala.f.min=inc_BCD(RtccTimeDateVala.f.min);
                else{
                    RtccTimeDateVala.f.min=0;
                    if(RtccTimeDateVala.f.hour<=0x22)
                        RtccTimeDateVala.f.hour=inc_BCD(RtccTimeDateVala.f.hour);
                    else{
                        RtccTimeDateVala.f.hour=0;
                   }
               }
         }
      }
        return &output_data[0];
  }







        // if(x_accel>=0x0F00){
        //     x_accel |=0xF000;
        //     x_accel= ~x_accel +1 ;
        //     x_g = -1*(x_accel * 0.00390625);
        // }
        // else
        //      x_g = (x_accel * 0.00390625);
        //
        //
        //  if(y_accel>=0x0F00){
        //     y_accel |=0xF000;
        //     y_accel= ~y_accel +1 ;
        //       y_g = -1*(y_accel * 0.00390625);
        // }
        //  else
        //     y_g = (y_accel * 0.00390625);
        //
        //
        //  if(z_accel>=0x0F00){
        //       z_accel |= 0xF000;
        //     z_accel= ~z_accel +1 ;
        //       z_g = -1*(z_accel * 0.00390625);
        // }
        //  else
        //            z_g = (z_accel * 0.00390625);
        //
        //


}

void standby_mode(){
         //enter standby
    CS=0;
    __delay_us(10);
    SPI2BUF = (WRITE | POWER_CONTROL_REGISTER);
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    SPI2BUF = (0x00);
    while(SPI2STATbits.SPIRBF == 0);
    //register_value = getcSPI2();
    register_value=SPI2BUF;

    __delay_us(10);
    CS=1;
}

void measurement_mode(){
    //enter measurement mode
    CS=0;
    __delay_us(10);
    SPI2BUF = (WRITE | POWER_CONTROL_REGISTER);
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    SPI2BUF = (POWER_CONTROL_VALUE);
    while(SPI2STATbits.SPIRBF == 0);
  //register_value = getcSPI2();
    register_value=SPI2BUF;

    __delay_us(10);
    CS=1;
}

int read_power_mode(){
    CS=0;
    __delay_us(10);
    SPI2BUF = (READ | POWER_CONTROL_REGISTER );
    while(SPI2STATbits.SPIRBF == 0);
    a=SPI2BUF;
    SPI2BUF = (0x00);
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    __delay_us(10);
    CS=1;
    return register_value;
}



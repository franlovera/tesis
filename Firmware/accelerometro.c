#include <p24Fxxxx.h>
#include "USB/usb.h"
#include <stdint.h>
#include "accel.h"
#include <stdio.h>
#ifndef USE_AND_OR 
#define USE_AND_OR
#define FOSC    (32000000)
#define FCY     (FOSC/2)
#endif
#include <libpic30.h>
#include <math.h>
#include "HardwareProfile - PIC24F Starter Kit.h"
#include "realtime_clock.h"
#include "pps.h"
#include "ports.h"
#include "SPI.h"
#include "timer.h"

int8_t read_accel_register(int8_t reg) {
    int8_t register_value;
    CS = 0;
    __delay_us(10);
    SPI2BUF = (READ | reg);
    //putcSPI2(READ | reg);
    while (SPI2STATbits.SPIRBF == 0);
    register_value = SPI2BUF;
    SPI2BUF = (0x00);
    while (SPI2STATbits.SPIRBF == 0);
    //register_value = getcSPI2();
    register_value = SPI2BUF;

    __delay_us(10);
    CS = 1;
    return (register_value);

}

int8_t setup_accelerometer(void) {
    int8_t a;
    // set interrupt enable register
    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | INT_ENABLE_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (INT_ENABLE_VALUE);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;
    __delay_us(1);
    CS = 1;

    // set interrupt map register
    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | INT_MAP_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (INT_MAP_VALUE);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    __delay_us(1);
    CS = 1;

    // set data format register appropriately

    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | DATA_FORMAT_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (DATA_FORMAT_VALUE);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;
    __delay_us(1);
    CS = 1;

    // set rate register to 6.25Hz conversion, no power saving mode
    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | BW_RATE_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (NO_POWER_SAVING | HZ_100);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;
    __delay_us(1);
    CS = 1;

    // set FIFO control register
    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | FIFO_CONTROL_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (FIFO_CONTROL_VALUE);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    __delay_us(1);
    CS = 1;

    // set power control register appropriately
    CS = 0;
    __delay_us(1);
    SPI2BUF = (WRITE | POWER_CONTROL_REGISTER);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    SPI2BUF = (POWER_CONTROL_VALUE);
    while (SPI2STATbits.SPIRBF == 0);
    a = SPI2BUF;

    __delay_us(1);
    CS = 1;
    return 1;
}

uint16_t read_accel(char *p_string, unsigned char *p_hora, uint16_t msec) {
    int8_t counter, x_accel_low, y_accel_low, z_accel_low;
    int16_t x_accel, y_accel, z_accel;
    uint16_t string_length;
    int8_t  hora,minuto,segundo;
    hora=*(p_hora+4);
    minuto=*(p_hora+7);
    segundo=*(p_hora+6);

    counter = 0;
    string_length = 0;

    while (counter <= 31) {
        CS = 0;
        asm("nop");
        asm("nop");
        SPI2BUF = (READ | MULTIBYTE_TRANSFER | DATA_REG);
        while (SPI2STATbits.SPIRBF == 0);
        x_accel_low = SPI2BUF;

        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        x_accel_low = SPI2BUF;
        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        x_accel = SPI2BUF; // x high byte

        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        y_accel_low = SPI2BUF;
        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        y_accel = SPI2BUF; // y high byte


        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        z_accel_low = SPI2BUF;
        SPI2BUF = (0x00);
        while (SPI2STATbits.SPIRBF == 0);
        z_accel = SPI2BUF; // z high byte

        asm("nop");
        asm("nop");
        CS = 1;

        x_accel = (x_accel << 8) | x_accel_low;
        y_accel = (y_accel << 8) | y_accel_low;
        z_accel = (z_accel << 8) | z_accel_low;

                string_length += snprintf   ( (p_string+ string_length),100,"%4i,%4i,%4i,%x,%x,%x,%u\n",
                x_accel,y_accel,z_accel,*(p_hora+4),*(p_hora+7),*(p_hora+6),msec);//,hora, minuto,segundo,msec);

//        string_length += sprintf(output_data + string_length, "%4i,%4i,%4i,%x,%x,%x,%u\n",
//                x_accel, y_accel, z_accel, RtccTimeDateVal.f.hour, RtccTimeDateVal.f.min, RtccTimeDateVal.f.sec, msec);


        counter++;

                msec+=625;
        
                if (msec>=31250){
                    msec-=31250;
                    if(*(p_hora+6)<=0x58)
                       *(p_hora+6)=inc_BCD(*(p_hora+6));
                    else{
                       *(p_hora+6)=0;
                        if(*(p_hora+7)<=0x58)
                            *(p_hora+7)=inc_BCD(*(p_hora+7));
                        else{
                           *(p_hora+7)=0;
                            if(*(p_hora+4)<=0x22)
                                *(p_hora+4)=inc_BCD(*(p_hora+4));
                            else{
                                *(p_hora+4)=0;
                           }
                       }
                 }
              }
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

    return msec;
}

int8_t standby_mode(){
         //enter standby
    int8_t register_value;
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
    return 0;
}

int8_t measurement_mode(){
    //enter measurement mode
    int8_t register_value;
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
    return 1;
}

int read_power_mode(){
    int8_t register_value;
    CS=0;
    __delay_us(10);
    SPI2BUF = (READ | POWER_CONTROL_REGISTER );
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    SPI2BUF = (0x00);
    while(SPI2STATbits.SPIRBF == 0);
    register_value=SPI2BUF;
    __delay_us(10);
    CS=1;
    return register_value;
}

void InitializeSystem(unsigned char  *p_hora) {
    UINT SPICON1Value, SPICON2Value, SPISTATValue;

    mPORTFClearBits(BIT_4);
    mPORTGClearBits(BIT_6 | BIT_8);
    mPORTFOutputConfig(BIT_4);
    mPORTGOutputConfig(BIT_6 | BIT_8);
    mPORTDInputConfig(BIT_5);
    mPORTDInputConfig(BIT_2);

    CS = 1;
    CS_TRIS = 0;


    /* RD6/CN15 is S3 */
    TRISEbits.TRISE4 = 1; // make the port as input
    CNEN4bits.CN62IE = 1; // enable interrupt
    CNPU4bits.CN62PUE = 0; // disable pull-up resistor
    IFS1bits.CNIF = 0; // clear IF
    IPC4bits.CNIP = 3; // set IP as 7
    IEC1bits.CNIE = 1; // enable CN




    //    //***************** Set the TIME & DATA and TIME *****************************************





    *p_hora = 0x14;          //year
    p_hora=p_hora+2;
        *p_hora = 0x08;
            p_hora++;
                *p_hora = 0x09;
                p_hora++;
                    *p_hora = 0x10;
                    p_hora=p_hora+2;
                        *p_hora = 0x39;
                                    p_hora++;
                            *p_hora  = 0x04;










    //Initialize the SPI  memory
    iPPSInput(IN_FN_PPS_SDI1, IN_PIN_PPS_RP13); //SDI1=RP13 EN PLACA FINAL
    iPPSOutput(OUT_PIN_PPS_RP28, OUT_FN_PPS_SDO1); //SDO1=RP28
    iPPSOutput(OUT_PIN_PPS_RP6, OUT_FN_PPS_SCK1OUT); //SCK1=RP6

    //		CNPU5bits.CN68PUE = 1;

    //spi acceler
    iPPSInput(IN_FN_PPS_SDI2, IN_PIN_PPS_RP24); //SDI2=RP24
    iPPSOutput(OUT_PIN_PPS_RP22, OUT_FN_PPS_SDO2); //SDO2=RP22
    iPPSOutput(OUT_PIN_PPS_RP25, OUT_FN_PPS_SCK2OUT); //SCK2=RP25



    SPICON1Value = SPI_MODE8_ON | MASTER_ENABLE_ON | PRI_PRESCAL_1_1 | SEC_PRESCAL_4_1 | SLAVE_ENABLE_OFF | SPI_CKE_OFF | CLK_POL_ACTIVE_LOW;
    SPICON2Value = FRAME_ENABLE_OFF;
    SPISTATValue = SPI_ENABLE;
    OpenSPI2(SPICON1Value, SPICON2Value, SPISTATValue);





    //AD1CON1 Register
    // Data Output Format: integer
    AD1CON1bits.FORM = 0;
    // Sample Clock Source: Timer 3 starts conversion
    //to do ver si esto es la forma mas eficiente, conviene controlar esto no cada 1 segundo en el timer 3 sino cada mas tiempo
    AD1CON1bits.SSRC = 2;


    // ADC Sample Control: Sampling begins when samp is set
    AD1CON1bits.ASAM = 1;

    //AD1CON2 Register
    // Generate interrupt every 16 sample/conversion
    // to do ver si conviene cambiar esto para que salte menos frecuentemente
    AD1CON2bits.SMPI = 0;
    // Buffer configured as one 16-word buffers
    AD1CON2bits.BUFM = 0;

    //AD1CON3 Register
    // ADC Clock is derived from Systems Clock
    AD1CON3bits.ADRC = 0;

    // ADC Conversion Clock Tad=Tcy*(ADCS+1)=(1/8M)*2*32 = 8us (125Khz)
    // ADC Conversion Time for 10-bit Tc=12*Tab = 96us
    AD1CON3bits.ADCS = 31;


    //AD1CHS0: A/D Input Select Register
    // MUXA +ve input selection (AIN5) for CH0
    // MUXA -ve input selection (Vref-) for CH0
    AD1CHS = 9;

    //AD1PCFGH/AD1PCFGL: Port Configuration Register
    AD1PCFG = 0xFFFF;
    AD1PCFGbits.PCFG9 = 0; // AN5 as Analog Input

    //AD1CSSH/AD1CSSL: A/D Input Scan Selection Register
    // Channel Scan is disabled, default state
    AD1CSSL = 0x0000;

    IFS0bits.AD1IF = 0; // Clear the A/D interrupt flag bit
    IEC0bits.AD1IE = 1; // Enable A/D interrupt
    AD1CON1bits.ADON = 1; // Turn on the A/D converter


    #if defined(USE_SELF_POWER_SENSE_IO)
    tris_self_power = INPUT_PIN; // See HardwareProfile.h
#endif

     USBDeviceInit(); //usb_device.c.  Initializes USB module SFRs and firmware
    //variables to known states.




}


BYTE inc_BCD(BYTE number){
    int8_t variable_aux;
     variable_aux= number & 0x000f;
    if(variable_aux==9)
        number+=7;
    else
        number+=1;
     return number;
}

void InitRTCC(unsigned char* rtcctime){
        RtccInitClock(); //turn on clock source
    mRtccSetIntPriority(4); //set interrupt priority to 4
    mRtccSetInt(1);
    RtccWrOn(); //enable RTCC peripheral
    mRtccOn();
    RtccWriteTimeDate(&rtcctime, 0);
}

void toggle_led1(void) {
    mPORTGToggleBits(BIT_6);

}
void led1_off(void){
                    mPORTGClearBits(BIT_6);
}

void all_modules_off(void){
     CloseTimer3();
     CloseSPI2();
     CloseSPI1();
     AD1CON1bits.ADON = 0;
}

void disable_adc_int(void){
    IEC0bits.AD1IE = 0; 
}

void enable_adc_int(void){
    IEC0bits.AD1IE = 1; 
}
void disable_t3_int(void){
    ConfigIntTimer3(T3_INT_OFF | T3_INT_PRIOR_5);
}
void t3_rtcc_sync(void){
 while (mRtccIs2ndHalfSecond());
 while (!mRtccIs2ndHalfSecond());                                //nos garantiza que el RTCC y el timer3 usado para mS esten sincronizados
 OpenTimer3(T3_ON | T3_PS_1_256, 31250);                         //configuramos el timer para que tenga una frecuencia de un hertz
}

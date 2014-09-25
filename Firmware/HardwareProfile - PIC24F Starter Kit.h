/********************************************************************
 FileName:      HardwareProfile - PIC24F Starter Kit.h
 Dependencies:  See INCLUDES section
 Processor:     PIC24FJ256GB106
 Hardware:      PIC24F Starter Kit
 Compiler:      Microchip C30
 Company:       Microchip Technology, Inc.

 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the �Company�) for its PIC� Microcontroller is intended and
 supplied to you, the Company�s customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

********************************************************************
 File Description:

 Change History:
  Rev   Date         Description
  1.0   11/19/2004   Initial release
  2.1   02/26/2007   Updated for simplicity and to use common
                     coding style
  2.3   09/15/2008   Broke out each hardware platform into its own
                     "HardwareProfile - xxx.h" file
********************************************************************/

#ifndef HARDWARE_PROFILE_PIC24F_STARTER_KIT_H
#define HARDWARE_PROFILE_PIC24F_STARTER_KIT_H

    /*******************************************************************/
    /******** USB stack hardware selection options *********************/
    /*******************************************************************/
    //This section is the set of definitions required by the MCHPFSUSB
    //  framework.  These definitions tell the firmware what mode it is
    //  running in, and where it can find the results to some information
    //  that the stack needs.
    //These definitions are required by every application developed with
    //  this revision of the MCHPFSUSB framework.  Please review each
    //  option carefully and determine which options are desired/required
    //  for your application.

    #define USE_SELF_POWER_SENSE_IO
    #define tris_self_power     TRISFbits.TRISF3    // Input
    #define self_power          1

    //#define USE_USB_BUS_SENSE_IO
    #define tris_usb_bus_sense  U1OTGSTATbits.SESVD  //TRISBbits.TRISB5    // Input
    #define USB_BUS_SENSE       U1OTGSTATbits.SESVD

    //Uncomment this to make the output HEX of this project
    //   to be able to be bootloaded using the HID bootloader
    //#define PROGRAMMABLE_WITH_USB_HID_BOOTLOADER

    //If the application is going to be used with the HID bootloader
    //  then this will provide a function for the application to
    //  enter the bootloader from the application (optional)
    #if defined(PROGRAMMABLE_WITH_USB_HID_BOOTLOADER)
        #define EnterBootloader() __asm__("goto 0x400")
    #endif



    /*******************************************************************/
    /******** MDD File System selection options ************************/
    /*******************************************************************/
    #define ERASE_BLOCK_SIZE        1024
    #define WRITE_BLOCK_SIZE        128

/*******************************************************************/
    /******** MDD File System selection options ************************/
    /*******************************************************************/
    #define USE_SD_INTERFACE_WITH_SPI
    #define USE_PIC24F
    #define USE_16BIT

    // Sample definitions for 16-bit processors (modify to fit your own project)
    #define SD_CS				PORTBbits.RB3
    #define SD_CS_TRIS			TRISBbits.TRISB3

    #define SD_CD				0
    #define SD_CD_TRIS			TRISBbits.TRISB14

    #define SD_WE				PORTBbits.RB15
    #define SD_WE_TRIS			TRISBbits.TRISB15

    // Registers for the SPI module you want to use
    #define SPICON1				SPI1CON1
    #define SPISTAT				SPI1STAT
    #define SPIBUF				SPI1BUF
    #define SPISTAT_RBF			SPI1STATbits.SPIRBF
    #define SPICON1bits			SPI1CON1bits
    #define SPISTATbits	MDD_MediaInitialize		SPI1STATbits
    #define SPIENABLE           SPI1STATbits.SPIEN

    // Tris pins for SCK/SDI/SDO lines
    #define SPICLOCK			TRISBbits.TRISB6
    #define SPIIN				TRISBbits.TRISB2
    #define SPIOUT			    TRISBbits.TRISB4
    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/
    /******** Application specific definitions *************************/
    /*******************************************************************/
    /*******************************************************************/
    /*******************************************************************/

    /** Board definition ***********************************************/
    //These defintions will tell the main() function which board is
    //  currently selected.  This will allow the application to add
    //  the correct configuration bits as wells use the correct
    //  initialization functions for the board.  These defitions are only
    //  required in the stack provided demos.  They are not required in
    //  final application design.
    #define DEMO_BOARD PIC24F_STARTER_KIT
    #define PIC24F_STARTER_KIT
    #define CLOCK_FREQ 32000000
    #define GetSystemClock() CLOCK_FREQ
    #define GetInstructionClock() GetSystemClock()

    /** LED ************************************************************/
    #define mInitAllLEDs()      LATG &= 0xFE1F; TRISG &= 0xFE1F; LATF &= 0xFFCF; TRISF &= 0xFFCF; //G6,7,8,9 and F4,5

    #define mGetLED_1()         (TRISG & ~0x0180?1:0)
    #define mGetLED_2()         (TRISG & ~0x0060?1:0)
    #define mGetLED_3()         (TRISF & ~0x0030?1:0)
    #define mGetLED_4()

    #define mLED_1_On()         TRISG |= 0x0180;
    #define mLED_2_On()         TRISG |= 0x0060;
    #define mLED_3_On()         TRISF |= 0x0030;
    #define mLED_4_On()

    #define mLED_1_Off()        TRISG &= ~0x0180;
    #define mLED_2_Off()        TRISG &= ~0x0060;
    #define mLED_3_Off()        TRISF &= ~0x0030;
    #define mLED_4_Off()

    #define mLED_1_Toggle()     TRISG ^= 0x0180;
    #define mLED_2_Toggle()     TRISG ^= 0x0060;
    #define mLED_3_Toggle()     TRISF ^= 0x0030;
    #define mLED_4_Toggle()

    /** SWITCH *********************************************************/
    #define mInitSwitch2()      TRISBbits.TRISB12=1;
    #define mInitSwitch3()      TRISBbits.TRIS13=1;
    #define mInitAllSwitches()  mInitSwitch2();mInitSwitch3();
    #define sw2                 PORTBbits.RB12
    #define sw3                 PORTBbits.RB13


  /** POT ************************************************************/
    #define mInitPOT()  {AD1PCFGLbits.PCFG5 = 0;    AD1CON2bits.VCFG = 0x0;    AD1CON3bits.ADCS = 0xFF;    AD1CON1bits.SSRC = 0x0;    AD1CON3bits.SAMC = 0b10000;    AD1CON1bits.FORM = 0b00;    AD1CON2bits.SMPI = 0x0;    AD1CON1bits.ADON = 1;}

    /** MDD File System error checking *********************************/
    // Will generate an error if the clock speed is too low to interface to the card
    #if (GetSystemClock() < 100000)
        #error Clock speed must exceed 100 kHz
    #endif

    /** I/O pin definitions ********************************************/
    #define INPUT_PIN 1
    #define OUTPUT_PIN 0


void CN_init(void);


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

#endif  //HARDWARE_PROFILE_PIC24F_STARTER_KIT_H

    


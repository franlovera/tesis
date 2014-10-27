#include "USB/usb.h"
#include "HardwareProfile.h"
#include "MDD File System/SD-SPI.h"

#include "USB/usb_function_msd.h"
#include "USB/usb_function_cdc.h"
#include "FSIO.h"
#include "pps.h"
#define USE_AND_OR
#define FOSC    (32000000)
#define FCY     (FOSC/2)
#include "p24Fxxxx.h"
#include "ports.h"
#include "SPI.h"
#include <stdint.h>
#include <math.h>
#include <libpic30.h>
#include <stdio.h>
#include "realtime_clock.h"
#include "timer.h"
#include <ACCEL.h>


/** CONFIGURATION **************************************************/
#if defined(PIC24F_STARTER_KIT)
_CONFIG1(JTAGEN_OFF & GCP_OFF & GWRP_OFF & FWDTEN_OFF & ICS_PGx1)
_CONFIG2(PLL_96MHZ_ON & IESO_OFF & FCKSM_CSDCMD & OSCIOFNC_ON & POSCMOD_HS & FNOSC_PRIPLL & PLLDIV_DIV2 & IOL1WAY_ON)
#endif



/** VARIABLES ******************************************************/


char USB_Out_Buffer[CDC_DATA_OUT_EP_SIZE];
char RS232_Out_Data[CDC_DATA_IN_EP_SIZE];

unsigned char NextUSBOut;
unsigned char NextUSBOut;
//char RS232_In_Data;
unsigned char LastRS232Out; // Number of characters in the buffer
unsigned char RS232cp; // current position within the buffer
unsigned char RS232_Out_Data_Rdy = 0;
USB_HANDLE lastTransmission;

USB_HANDLE USBOutHandle = 0; //Needs to be initialized to 0 at startup.
USB_HANDLE USBInHandle = 0; //Needs to be initialized to 0 at startup.
BOOL blinkStatusValid = TRUE;

char USB_In_Buffer[64];
char USB_Out_Buffer[64];
BOOL stringPrinted;
volatile BOOL buttonPressed;
volatile BYTE buttonCount;




int8_t devid;
uint16_t msec;
char output_data[1000];
char name_file[30], datatime[50];
int measurement = 1, sw_power, adcPtr;

rtccTimeDate RtccTimeDate, RtccTimeDateVal;
int a, i;

FSFILE *logFile;

#if defined(__C30__) || defined(__C32__) || defined __XC16__
//The LUN variable definition is critical to the MSD function driver.  This
//  array is a structure of function pointers that are the functions that 
//  will take care of each of the physical media.  For each additional LUN
//  that is added to the system, an entry into this array needs to be added
//  so that the stack can know where to find the physical layer functions.
//  In this example the media initialization function is named 
//  "MediaInitialize", the read capacity function is named "ReadCapacity",
//  etc.  
LUN_FUNCTIONS LUN[MAX_LUN + 1] ={
    {
        &MDD_SDSPI_MediaInitialize,
        &MDD_SDSPI_ReadCapacity,
        &MDD_SDSPI_ReadSectorSize,
        &MDD_SDSPI_MediaDetect,
        &MDD_SDSPI_SectorRead,
        &MDD_SDSPI_WriteProtectState,
        &MDD_SDSPI_SectorWrite
    }
};
#endif

/* Standard Response to INQUIRY command stored in ROM 	*/
const ROM InquiryResponse inq_resp = {
    0x00, // peripheral device is connected, direct access block device
    0x80, // removable
    0x04, // version = 00=> does not conform to any standard, 4=> SPC-2
    0x02, // response is in format specified by SPC-2
    0x20, // n-4 = 36-4=32= 0x20
    0x00, // sccs etc.
    0x00, // bque=1 and cmdque=0,indicates simple queueing 00 is obsolete,
    // but as in case of other device, we are just using 00
    0x00, // 00 obsolete, 0x80 for basic task queueing
    {'M', 'i', 'c', 'r', 'o', 'c', 'h', 'p'},
    // this is the T10 assigned Vendor ID
    {'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e', ' ', ' ', ' ', ' '},
    {'0', '0', '0', '1'}
};

/** PRIVATE PROTOTYPES *********************************************/
void USBDeviceTasks(void);
void ProcessIO(void);
void YourHighPriorityISRCode(void);
void YourLowPriorityISRCode(void);
void USBCBSendResume(void);
void BlinkUSBStatus(void);
void UserInit(void);
void InitializeUSART(void);
void putcUSART(char c);
unsigned char getcUSART();




/** DECLARATIONS ***************************************************/




/********************************************************************
 * Function:        void main(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 *****************************************************************************/

#if defined(__18CXX)
void main(void)
#else 

int main(void)
#endif
{
    InitializeSystem((unsigned char*) &RtccTimeDateVal);                            //Inicializa los perifericos
    InitRTCC(&RtccTimeDateVal);                                                     //Inicializa el RTCC
    measurement = setup_accelerometer();                                            //configuramos el accel
    devid = read_accel_register(0x00);                                              //se lee el devid para ver si esta bien configurado
    sw_power = PORTEbits.RE4;
    //         while (!FSInit());
    measurement = 0;


    while (1) {
#if defined(USB_INTERRUPT)                                                          //actualiza el estado del USB
        if (USB_BUS_SENSE && (USBGetDeviceState() == DETACHED_STATE)) {
            USBDeviceAttach();
        } else // ADDED BY ANGUEL
            if ((USB_BUS_SENSE != 1) && (USBGetDeviceState() != DETACHED_STATE)) {
            USBDeviceDetach();
        }
#endif
        ProcessIO();                                                                //funcion para leer la hora atraves del USB


        if ((USBDeviceState == DETACHED_STATE)) {
            if (sw_power == 1) {
                if (measurement == 0) {                                             //si no esta en measurement mode, lo pone en este modo
                    IEC0bits.AD1IE = 0;                                             // disable A/D interrupt
                    InitializeSystem((unsigned char*) &RtccTimeDateVal);            //Inicializa el sistema ya que estaba en sleep antes
                    ConfigIntTimer3(T3_INT_OFF | T3_INT_PRIOR_5);                   //se Apagan la interrupts del timer3

                    while (!FSInit());                                              //esperamos hasta inicializar la tarjeta SD
                   measurement= setup_accelerometer();                              //configuramos el accel
                    while (mRtccIs2ndHalfSecond());
                    while (!mRtccIs2ndHalfSecond());                                //nos garantiza que el RTCC y el timer3 usado para mS esten sincronizados
                    OpenTimer3(T3_ON | T3_PS_1_256, 31250);                         //configuramos el timer para que tenga una frecuencia de un hertz
                    IEC0bits.AD1IE = 1; // Enable A/D interrupt
                }
                if (PORTDbits.RD5 == 1)                                             //si salta la interupcion lee los datos del accel
                {
                    if (logFile != NULL)                                            //si el archivo esta abierto lee datos y escribe al file
                    {
                        IEC0bits.AD1IE = 0;                                         // disable A/D interrupt
                        msec= read_accel(&output_data[0], &RtccTimeDateVal, msec);  // leemos el accelerometro y escribimos al string en output_data
                        FSfprintf(logFile, output_data);                            //escribe al file
                        //            FSfwrite( output_data, 2100, 1, logFile);
                        toggle_led1();                                              //togleamos led
                        IEC0bits.AD1IE = 1;                                         // Enable A/D interrupt
                    } else {                                                        //si el archivo no esta abierto, lee la hora, y abre un archivo nuevo
                        IEC0bits.AD1IE = 0;                                         // disable A/D interrupt
                        RtccReadTimeDate(&RtccTimeDateVal);                         //leemos la hora y se guarda en RtccTimeDateVal
                        sprintf(name_file, "%x.%x.%x  %x-%x-%x.csv",
                                RtccTimeDateVal.f.hour, RtccTimeDateVal.f.min,      //escribimos un string en name_file con el nombre del archivo a escribir
                                RtccTimeDateVal.f.sec, RtccTimeDateVal.f.mday,
                                RtccTimeDateVal.f.mon, RtccTimeDateVal.f.year);
                        logFile = FSfopen(name_file, "w");                          //se abre el file
                        FSfprintf(logFile, "x,y,z,hr,min,sec,msec\n\0");            //es escribe el rotulo del archivo
                        msec = ReadTimer3();                                        //se leen los milisegundos
                        IEC0bits.AD1IE = 1;                                         // Enable A/D interrupt
                    }
                }
                else
                    a = 0;                                                          //para testeo
            } else {                                                                //si el switch esta en off
                IEC0bits.AD1IE = 0;
                if (logFile != NULL) {                                              //y el file esta abierto, lo cierra
                    FSfclose(logFile);                                              //se cierra el archivo
                    logFile = NULL;                                                 //se actualiza logFile que funciona como una bandera para saber si hay file abierto o no
                }
                //      if (measurement==1){                    //si esta en measurement, lo pone en standby sino no hace nada
              measurement=  standby_mode();                                         //se pone en standby el acel
                
                led1_off();                                                         // led1 off
                all_modules_off();                                                  //se apagan los modulos para reducir el consumo
                Sleep();                                                            //se duerme el micro
                //          }
            }
        }
        if (!(USBDeviceState == DETACHED_STATE)) {                                  //si se enchufa el usb
            if (logFile != NULL) {                                                  //y hay un archivo abierto
                FSfclose(logFile);                                                  //se cierra el archivo
                logFile = NULL;                                                     //se actualiza la bandera logFile
                led1_off();
                measurement = 0;
            }
        }

    }//end while
}//end main

/********************************************************************
 * Function:        static void InitializeSystem(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        InitializeSystem is a centralize initialization
 *                  routine. All required USB initialization routines
 *                  are called from here.
 *
 *                  User application initialization routine should
 *                  also be called from here.                  
 *
 * Note:            None
 *******************************************************************/
//static void InitializeSystem(void) {
//
//    mPORTFClearBits(BIT_4);
//    mPORTGClearBits(BIT_6 | BIT_8);
//    mPORTFOutputConfig(BIT_4);
//    mPORTGOutputConfig(BIT_6 | BIT_8);
//    mPORTDInputConfig(BIT_5);
//    mPORTDInputConfig(BIT_2);
//
//    CS = 1;
//    CS_TRIS = 0;
//
//
//    /* RD6/CN15 is S3 */
//    TRISEbits.TRISE4 = 1; // make the port as input
//    CNEN4bits.CN62IE = 1; // enable interrupt
//    CNPU4bits.CN62PUE = 0; // disable pull-up resistor
//    IFS1bits.CNIF = 0; // clear IF
//    IPC4bits.CNIP = 3; // set IP as 7
//    IEC1bits.CNIE = 1; // enable CN
//
//
//
//
//    //***************** Set the TIME & DATA and TIME *****************************************
//    RtccTimeDate.f.hour = 0x10;
//    RtccTimeDate.f.min = 0x39;
//    RtccTimeDate.f.sec = 0;
//    RtccTimeDate.f.mday = 0x08;
//    RtccTimeDate.f.mon = 0x09;
//    RtccTimeDate.f.year = 0x14;
//
//
//
//
//
//
//
//
//
//    //Initialize the SPI  memory
//    iPPSInput(IN_FN_PPS_SDI1, IN_PIN_PPS_RP13); //SDI1=RP13 EN PLACA FINAL
//    iPPSOutput(OUT_PIN_PPS_RP28, OUT_FN_PPS_SDO1); //SDO1=RP28
//    iPPSOutput(OUT_PIN_PPS_RP6, OUT_FN_PPS_SCK1OUT); //SCK1=RP6
//
//    //		CNPU5bits.CN68PUE = 1;
//
//    //spi acceler
//    iPPSInput(IN_FN_PPS_SDI2, IN_PIN_PPS_RP24); //SDI2=RP24
//    iPPSOutput(OUT_PIN_PPS_RP22, OUT_FN_PPS_SDO2); //SDO2=RP22
//    iPPSOutput(OUT_PIN_PPS_RP25, OUT_FN_PPS_SCK2OUT); //SCK2=RP25
//
//
//
//    SPICON1Value = SPI_MODE8_ON | MASTER_ENABLE_ON | PRI_PRESCAL_1_1 | SEC_PRESCAL_4_1 | SLAVE_ENABLE_OFF | SPI_CKE_OFF | CLK_POL_ACTIVE_LOW;
//    SPICON2Value = FRAME_ENABLE_OFF;
//    SPISTATValue = SPI_ENABLE;
//    OpenSPI2(SPICON1Value, SPICON2Value, SPISTATValue);
//
//
//
//
//
//    //AD1CON1 Register
//    // Data Output Format: integer
//    AD1CON1bits.FORM = 0;
//    // Sample Clock Source: Timer 3 starts conversion
//    //to do ver si esto es la forma mas eficiente, conviene controlar esto no cada 1 segundo en el timer 3 sino cada mas tiempo
//    AD1CON1bits.SSRC = 2;
//
//
//    // ADC Sample Control: Sampling begins when samp is set
//    AD1CON1bits.ASAM = 1;
//
//    //AD1CON2 Register
//    // Generate interrupt every 16 sample/conversion
//    // to do ver si conviene cambiar esto para que salte menos frecuentemente
//    AD1CON2bits.SMPI = 0;
//    // Buffer configured as one 16-word buffers
//    AD1CON2bits.BUFM = 0;
//
//    //AD1CON3 Register
//    // ADC Clock is derived from Systems Clock
//    AD1CON3bits.ADRC = 0;
//
//    // ADC Conversion Clock Tad=Tcy*(ADCS+1)=(1/8M)*2*32 = 8us (125Khz)
//    // ADC Conversion Time for 10-bit Tc=12*Tab = 96us
//    AD1CON3bits.ADCS = 31;
//
//
//    //AD1CHS0: A/D Input Select Register
//    // MUXA +ve input selection (AIN5) for CH0
//    // MUXA -ve input selection (Vref-) for CH0
//    AD1CHS = 9;
//
//    //AD1PCFGH/AD1PCFGL: Port Configuration Register
//    AD1PCFG = 0xFFFF;
//    AD1PCFGbits.PCFG9 = 0; // AN5 as Analog Input
//
//    //AD1CSSH/AD1CSSL: A/D Input Scan Selection Register
//    // Channel Scan is disabled, default state
//    AD1CSSL = 0x0000;
//
//    IFS0bits.AD1IF = 0; // Clear the A/D interrupt flag bit
//    IEC0bits.AD1IE = 1; // Enable A/D interrupt
//    AD1CON1bits.ADON = 1; // Turn on the A/D converter
//
//
//
//
//
//
//
//    //	The USB specifications require that USB peripheral devices must never source
//    //	current onto the Vbus pin.  Additionally, USB peripherals should not source
//    //	current on D+ or D- when the host/hub is not actively powering the Vbus line.
//    //	When designing a self powered (as opposed to bus powered) USB peripheral
//    //	device, the firmware should make sure not to turn on the USB module and D+
//    //	or D- pull up resistor unless Vbus is actively powered.  Therefore, the
//    //	firmware needs some means to detect when Vbus is being powered by the host.
//    //	A 5V tolerant I/O pin can be connected to Vbus (through a resistor), and
//    // 	can be used to detect when Vbus is high (host actively powering), or low
//    //	(host is shut down or otherwise not supplying power).  The USB firmware
//    // 	can then periodically poll this I/O pin to know when it is okay to turn on
//    //	the USB module/D+/D- pull up resistor.  When designing a purely bus powered
//    //	peripheral device, it is not possible to source current on D+ or D- when the
//    //	host is not actively providing power on Vbus. Therefore, implementing this
//    //	bus sense feature is optional.  This firmware can be made to use this bus
//    //	sense feature by making sure "USE_USB_BUS_SENSE_IO" has been defined in the
//    //	HardwareProfile.h file.
//#if defined(USE_USB_BUS_SENSE_IO)
//    tris_usb_bus_sense = INPUT_PIN; // See HardwareProfile.h
//#endif
//
//    //	If the host PC sends a GetStatus (device) request, the firmware must respond
//    //	and let the host know if the USB peripheral device is currently bus powered
//    //	or self powered.  See chapter 9 in the official USB specifications for details
//    //	regarding this request.  If the peripheral device is capable of being both
//    //	self and bus powered, it should not return a hard coded value for this request.
//    //	Instead, firmware should check if it is currently self or bus powered, and
//    //	respond accordingly.  If the hardware has been configured like demonstrated
//    //	on the PICDEM FS USB Demo Board, an I/O pin can be polled to determine the
//    //	currently selected power source.  On the PICDEM FS USB Demo Board, "RA2"
//    //	is used for	this purpose.  If using this feature, make sure "USE_SELF_POWER_SENSE_IO"
//    //	has been defined in HardwareProfile - (platform).h, and that an appropriate I/O pin
//    //  has been mapped	to it.
//#if defined(USE_SELF_POWER_SENSE_IO)
//    tris_self_power = INPUT_PIN; // See HardwareProfile.h
//#endif
//
//
//    USBDeviceInit(); //usb_device.c.  Initializes USB module SFRs and firmware
//    //variables to known states.
//}//end InitializeSystem




/******************************************************************************
 * Function:        void mySetLineCodingHandler(void)
 *
 * PreCondition:    USB_CDC_SET_LINE_CODING_HANDLER is defined
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function gets called when a SetLineCoding command
 *                  is sent on the bus.  This function will evaluate the request
 *                  and determine if the application should update the baudrate
 *                  or not.
 *
 * Note:            
 *
 *****************************************************************************/
#if defined(USB_CDC_SET_LINE_CODING_HANDLER)

void mySetLineCodingHandler(void) {
    //If the request is not in a valid range
    if (cdc_notice.GetLineCoding.dwDTERate.Val > 115200) {
        //NOTE: There are two ways that an unsupported baud rate could be
        //handled.  The first is just to ignore the request and don't change
        //the values.  That is what is currently implemented in this function.
        //The second possible method is to stall the STATUS stage of the request.
        //STALLing the STATUS stage will cause an exception to be thrown in the 
        //requesting application.  Some programs, like HyperTerminal, handle the
        //exception properly and give a pop-up box indicating that the request
        //settings are not valid.  Any application that does not handle the
        //exception correctly will likely crash when this requiest fails.  For
        //the sake of example the code required to STALL the status stage of the
        //request is provided below.  It has been left out so that this demo
        //does not cause applications without the required exception handling
        //to crash.
        //---------------------------------------
        //USBStallEndpoint(0,1);
    } else {
        //DWORD_VAL dwBaud;

        //Update the baudrate info in the CDC driver
        CDCSetBaudRate(cdc_notice.GetLineCoding.dwDTERate.Val);

        //Update the baudrate of the UART
        //        #if defined(__18CXX)
        //            dwBaud.Val = (GetSystemClock()/4)/line_coding.dwDTERate.Val-1;
        //            SPBRG = dwBaud.v[0];
        //            SPBRGH = dwBaud.v[1];
        //        #elif defined(__C30__) || defined __XC16__
        //            dwBaud.Val = (((GetPeripheralClock()/2)+(BRG_DIV2/2*line_coding.dwDTERate.Val))/BRG_DIV2/line_coding.dwDTERate.Val-1);
        //            U2BRG = dwBaud.Val;
        //        #elif defined(__C32__)
        //            U2BRG = ((GetPeripheralClock()+(BRG_DIV2/2*line_coding.dwDTERate.Val))/BRG_DIV2/line_coding.dwDTERate.Val-1);
        //            //U2MODE = 0;
        //            U2MODEbits.BRGH = BRGH2;
        //            //U2STA = 0;
        //        #endif
    }
}
#endif

/********************************************************************
 * Function:        void ProcessIO(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is a place holder for other user
 *                  routines. It is a mixture of both USB and
 *                  non-USB tasks.
 *
 * Note:            None
 *******************************************************************/
void ProcessIO(void) {
    char USB_In_Buffer[64];
    char USB_Out_Buffer[64];



    // User Application USB tasks
    if ((USBDeviceState < CONFIGURED_STATE) || (USBSuspendControl == 1)) return;


    BYTE numBytesRead = 0;

    if ((USBDeviceState == CONFIGURED_STATE)) {
        if (USBUSARTIsTxTrfReady()) {



            numBytesRead = getsUSBUSART(USB_Out_Buffer, 64);
            if (numBytesRead != 0) {
                BYTE i, jk = 0;

                for (i = 0; i < numBytesRead; i++) {
                    switch (USB_Out_Buffer[i]) {
                        case 0x0A:
                        case 0x0D:
                            USB_In_Buffer[jk] = USB_Out_Buffer[i];
                            break;
                        default:
                            USB_In_Buffer[jk] = USB_Out_Buffer[i];
                            datatime[jk] = USB_Out_Buffer[i];
                            jk++;
                            //todo guardar datos en rtcc
                            break;
                    }

                }

                putUSBUSART(USB_Out_Buffer, numBytesRead);
                jk = 0;
                if (datatime[0] != NULL) if (datatime[1] != NULL)
                        if (datatime[2] != NULL) if (datatime[3] != NULL)
                                if (datatime[4] != NULL) if (datatime[5] != NULL)
                                        if (datatime[6] != NULL) if (datatime[7] != NULL)
                                                if (datatime[8] != NULL) if (datatime[9] != NULL)
                                                        if (datatime[10] != NULL)if (datatime[11] != NULL)
                                                                if (datatime[12] != NULL)if (datatime[13] != NULL)
                                                                        if (datatime[14] != NULL)if (datatime[15] != NULL)
                                                                                if (datatime[16] != NULL) {

                                                                                    RtccTimeDate.f.year = (datatime[0] - 0x30)*16 + (datatime[1] - 0x30);
                                                                                    RtccTimeDate.f.hour = (datatime[9] - 0x30)*16 + (datatime[10] - 0x30);
                                                                                    RtccTimeDate.f.min = (datatime[12] - 0x30)*16 + (datatime[13] - 0x30);
                                                                                    RtccTimeDate.f.sec = (datatime[15] - 0x30)*16 + (datatime[16] - 0x30);
                                                                                    RtccTimeDate.f.mday = (datatime[6] - 0x30)*16 + (datatime[7] - 0x30);
                                                                                    RtccTimeDate.f.mon = (datatime[3] - 0x30)*16 + (datatime[4] - 0x30);
                                                                                    RtccWriteTimeDate(&RtccTimeDate, 0);
                                                                                }

            }

        }


        CDCTxService();
        MSDTasks();
    }
}//end ProcessIO





// ******************************************************************************************************
// ************** USB Callback Functions ****************************************************************
// ******************************************************************************************************
// The USB firmware stack will call the callback functions USBCBxxx() in response to certain USB related
// events.  For example, if the host PC is powering down, it will stop sending out Start of Frame (SOF)
// packets to your device.  In response to this, all USB devices are supposed to decrease their power
// consumption from the USB Vbus to <2.5mA* each.  The USB module detects this condition (which according
// to the USB specifications is 3+ms of no bus activity/SOF packets) and then calls the USBCBSuspend()
// function.  You should modify these callback functions to take appropriate actions for each of these
// conditions.  For example, in the USBCBSuspend(), you may wish to add code that will decrease power
// consumption from Vbus to <2.5mA (such as by clock switching, turning off LEDs, putting the
// microcontroller to sleep, etc.).  Then, in the USBCBWakeFromSuspend() function, you may then wish to
// add code that undoes the power saving things done in the USBCBSuspend() function.

// The USBCBSendResume() function is special, in that the USB stack will not automatically call this
// function.  This function is meant to be called from the application firmware instead.  See the
// additional comments near the function.

// Note *: The "usb_20.pdf" specs indicate 500uA or 2.5mA, depending upon device classification. However,
// the USB-IF has officially issued an ECN (engineering change notice) changing this to 2.5mA for all 
// devices.  Make sure to re-download the latest specifications to get all of the newest ECNs.

/******************************************************************************
 * Function:        void USBCBSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Call back that is invoked when a USB suspend is detected
 *
 * Note:            None
 *****************************************************************************/
void USBCBSuspend(void) {
    //Example power saving code.  Insert appropriate code here for the desired
    //application behavior.  If the microcontroller will be put to sleep, a
    //process similar to that shown below may be used:

    //ConfigureIOPinsForLowPower();
    //SaveStateOfAllInterruptEnableBits();
    //DisableAllInterruptEnableBits();
    //EnableOnlyTheInterruptsWhichWillBeUsedToWakeTheMicro();	//should enable at least USBActivityIF as a wake source
    //Sleep();
    //RestoreStateOfAllPreviouslySavedInterruptEnableBits();	//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.
    //RestoreIOPinsToNormal();									//Preferrably, this should be done in the USBCBWakeFromSuspend() function instead.

    //IMPORTANT NOTE: Do not clear the USBActivityIF (ACTVIF) bit here.  This bit is
    //cleared inside the usb_device.c file.  Clearing USBActivityIF here will cause
    //things to not work as intended.


#if defined(__C30__) || defined __XC16__
#if 0
    U1EIR = 0xFFFF;
    U1IR = 0xFFFF;
    U1OTGIR = 0xFFFF;
    IFS5bits.USB1IF = 0;
    IEC5bits.USB1IE = 1;
    U1OTGIEbits.ACTVIE = 1;
    U1OTGIRbits.ACTVIF = 1;
    Sleep();
#endif
#endif
}


/******************************************************************************
 * Function:        void _USB1Interrupt(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the USB interrupt bit is set
 *					In this example the interrupt is only used when the device
 *					goes to sleep when it receives a USB suspend command
 *
 * Note:            None
 *****************************************************************************/
#if 0

void __attribute__((interrupt)) _USB1Interrupt(void) {
#if !defined(self_powered)
    if (U1OTGIRbits.ACTVIF) {
        IEC5bits.USB1IE = 0;
        U1OTGIEbits.ACTVIE = 0;
        IFS5bits.USB1IF = 0;

        //USBClearInterruptFlag(USBActivityIFReg,USBActivityIFBitNum);
        USBClearInterruptFlag(USBIdleIFReg, USBIdleIFBitNum);
        //USBSuspendControl = 0;
    }
#endif
}
#endif

/******************************************************************************
 * Function:        void USBCBWakeFromSuspend(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The host may put USB peripheral devices in low power
 *					suspend mode (by "sending" 3+ms of idle).  Once in suspend
 *					mode, the host may wake the device back up by sending non-
 *					idle state signalling.
 *					
 *					This call back is invoked when a wakeup from USB suspend 
 *					is detected.
 *
 * Note:            None
 *****************************************************************************/
void USBCBWakeFromSuspend(void) {
    // If clock switching or other power savings measures were taken when
    // executing the USBCBSuspend() function, now would be a good time to
    // switch back to normal full power run mode conditions.  The host allows
    // 10+ milliseconds of wakeup time, after which the device must be
    // fully back to normal, and capable of receiving and processing USB
    // packets.  In order to do this, the USB module must receive proper
    // clocking (IE: 48MHz clock must be available to SIE for full speed USB
    // operation).
    // Make sure the selected oscillator settings are consistent with USB
    // operation before returning from this function.
}

/********************************************************************
 * Function:        void USBCB_SOF_Handler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB host sends out a SOF packet to full-speed
 *                  devices every 1 ms. This interrupt may be useful
 *                  for isochronous pipes. End designers should
 *                  implement callback routine as necessary.
 *
 * Note:            None
 *******************************************************************/
void USBCB_SOF_Handler(void) {
    // No need to clear UIRbits.SOFIF to 0 here.
    // Callback caller is already doing that.

    //This is reverse logic since the pushbutton is active low
    if (buttonPressed == sw2) {
        if (buttonCount != 0) {
            buttonCount--;
        } else {
            //This is reverse logic since the pushbutton is active low
            buttonPressed = !sw2;

            //Wait 100ms before the next press can be generated
            buttonCount = 100;
        }
    } else {
        if (buttonCount != 0) {
            buttonCount--;
        }
    }
}

/*******************************************************************
 * Function:        void USBCBErrorHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The purpose of this callback is mainly for
 *                  debugging during development. Check UEIR to see
 *                  which error causes the interrupt.
 *
 * Note:            None
 *******************************************************************/
void USBCBErrorHandler(void) {
    // No need to clear UEIR to 0 here.
    // Callback caller is already doing that.

    // Typically, user firmware does not need to do anything special
    // if a USB error occurs.  For example, if the host sends an OUT
    // packet to your device, but the packet gets corrupted (ex:
    // because of a bad connection, or the user unplugs the
    // USB cable during the transmission) this will typically set
    // one or more USB error interrupt flags.  Nothing specific
    // needs to be done however, since the SIE will automatically
    // send a "NAK" packet to the host.  In response to this, the
    // host will normally retry to send the packet again, and no
    // data loss occurs.  The system will typically recover
    // automatically, without the need for application firmware
    // intervention.

    // Nevertheless, this callback function is provided, such as
    // for debugging purposes.
}

/*******************************************************************
 * Function:        void USBCBCheckOtherReq(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        When SETUP packets arrive from the host, some
 * 					firmware must process the request and respond
 *					appropriately to fulfill the request.  Some of
 *					the SETUP packets will be for standard
 *					USB "chapter 9" (as in, fulfilling chapter 9 of
 *					the official USB specifications) requests, while
 *					others may be specific to the USB device class
 *					that is being implemented.  For example, a HID
 *					class device needs to be able to respond to
 *					"GET REPORT" type of requests.  This
 *					is not a standard USB chapter 9 request, and 
 *					therefore not handled by usb_device.c.  Instead
 *					this request should be handled by class specific 
 *					firmware, such as that contained in usb_function_hid.c.
 *
 * Note:            None
 *******************************************************************/
void USBCBCheckOtherReq(void) {
    USBCheckMSDRequest();
    USBCheckCDCRequest();
}//end

/*******************************************************************
 * Function:        void USBCBStdSetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USBCBStdSetDscHandler() callback function is
 *					called when a SETUP, bRequest: SET_DESCRIPTOR request
 *					arrives.  Typically SET_DESCRIPTOR requests are
 *					not used in most applications, and it is
 *					optional to support this type of request.
 *
 * Note:            None
 *******************************************************************/
void USBCBStdSetDscHandler(void) {
    // Must claim session ownership if supporting this request
}//end

/*******************************************************************
 * Function:        void USBCBInitEP(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called when the device becomes
 *                  initialized, which occurs after the host sends a
 * 					SET_CONFIGURATION (wValue not = 0) request.  This 
 *					callback function should initialize the endpoints 
 *					for the device's usage according to the current 
 *					configuration.
 *
 * Note:            None
 *******************************************************************/
void USBCBInitEP(void) {
#if (MSD_DATA_IN_EP == MSD_DATA_OUT_EP)
    USBEnableEndpoint(MSD_DATA_IN_EP, USB_IN_ENABLED | USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
#else
    USBEnableEndpoint(MSD_DATA_IN_EP, USB_IN_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
    USBEnableEndpoint(MSD_DATA_OUT_EP, USB_OUT_ENABLED | USB_HANDSHAKE_ENABLED | USB_DISALLOW_SETUP);
#endif

    USBMSDInit();
    CDCInitEP();
}

/********************************************************************
 * Function:        void USBCBSendResume(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        The USB specifications allow some types of USB
 * 					peripheral devices to wake up a host PC (such
 *					as if it is in a low power suspend to RAM state).
 *					This can be a very useful feature in some
 *					USB applications, such as an Infrared remote
 *					control	receiver.  If a user presses the "power"
 *					button on a remote control, it is nice that the
 *					IR receiver can detect this signalling, and then
 *					send a USB "command" to the PC to wake up.
 *					
 *					The USBCBSendResume() "callback" function is used
 *					to send this special USB signalling which wakes 
 *					up the PC.  This function may be called by
 *					application firmware to wake up the PC.  This
 *					function will only be able to wake up the host if
 *                  all of the below are true:
 *					
 *					1.  The USB driver used on the host PC supports
 *						the remote wakeup capability.
 *					2.  The USB configuration descriptor indicates
 *						the device is remote wakeup capable in the
 *						bmAttributes field.
 *					3.  The USB host PC is currently sleeping,
 *						and has previously sent your device a SET 
 *						FEATURE setup packet which "armed" the
 *						remote wakeup capability.   
 *
 *                  If the host has not armed the device to perform remote wakeup,
 *                  then this function will return without actually performing a
 *                  remote wakeup sequence.  This is the required behavior, 
 *                  as a USB device that has not been armed to perform remote 
 *                  wakeup must not drive remote wakeup signalling onto the bus;
 *                  doing so will cause USB compliance testing failure.
 *                  
 *					This callback should send a RESUME signal that
 *                  has the period of 1-15ms.
 *
 * Note:            This function does nothing and returns quickly, if the USB
 *                  bus and host are not in a suspended condition, or are 
 *                  otherwise not in a remote wakeup ready state.  Therefore, it
 *                  is safe to optionally call this function regularly, ex: 
 *                  anytime application stimulus occurs, as the function will
 *                  have no effect, until the bus really is in a state ready
 *                  to accept remote wakeup. 
 *
 *                  When this function executes, it may perform clock switching,
 *                  depending upon the application specific code in 
 *                  USBCBWakeFromSuspend().  This is needed, since the USB
 *                  bus will no longer be suspended by the time this function
 *                  returns.  Therefore, the USB module will need to be ready
 *                  to receive traffic from the host.
 *
 *                  The modifiable section in this routine may be changed
 *                  to meet the application needs. Current implementation
 *                  temporary blocks other functions from executing for a
 *                  period of ~3-15 ms depending on the core frequency.
 *
 *                  According to USB 2.0 specification section 7.1.7.7,
 *                  "The remote wakeup device must hold the resume signaling
 *                  for at least 1 ms but for no more than 15 ms."
 *                  The idea here is to use a delay counter loop, using a
 *                  common value that would work over a wide range of core
 *                  frequencies.
 *                  That value selected is 1800. See table below:
 *                  ==========================================================
 *                  Core Freq(MHz)      MIP         RESUME Signal Period (ms)
 *                  ==========================================================
 *                      48              12          1.05
 *                       4              1           12.6
 *                  ==========================================================
 *                  * These timing could be incorrect when using code
 *                    optimization or extended instruction mode,
 *                    or when having other interrupts enabled.
 *                    Make sure to verify using the MPLAB SIM's Stopwatch
 *                    and verify the actual signal on an oscilloscope.
 *******************************************************************/
void USBCBSendResume(void) {
    static WORD delay_count;

    //First verify that the host has armed us to perform remote wakeup.
    //It does this by sending a SET_FEATURE request to enable remote wakeup,
    //usually just before the host goes to standby mode (note: it will only
    //send this SET_FEATURE request if the configuration descriptor declares
    //the device as remote wakeup capable, AND, if the feature is enabled
    //on the host (ex: on Windows based hosts, in the device manager 
    //properties page for the USB device, power management tab, the 
    //"Allow this device to bring the computer out of standby." checkbox 
    //should be checked).
    if (USBGetRemoteWakeupStatus() == TRUE) {
        //Verify that the USB bus is in fact suspended, before we send
        //remote wakeup signalling.
        if (USBIsBusSuspended() == TRUE) {
            USBMaskInterrupts();

            //Clock switch to settings consistent with normal USB operation.
            USBCBWakeFromSuspend();
            USBSuspendControl = 0;
            USBBusIsSuspended = FALSE; //So we don't execute this code again,
            //until a new suspend condition is detected.

            //Section 7.1.7.7 of the USB 2.0 specifications indicates a USB
            //device must continuously see 5ms+ of idle on the bus, before it sends
            //remote wakeup signalling.  One way to be certain that this parameter
            //gets met, is to add a 2ms+ blocking delay here (2ms plus at 
            //least 3ms from bus idle to USBIsBusSuspended() == TRUE, yeilds
            //5ms+ total delay since start of idle).
            delay_count = 3600U;
            do {
                delay_count--;
            } while (delay_count);

            //Now drive the resume K-state signalling onto the USB bus.
            USBResumeControl = 1; // Start RESUME signaling
            delay_count = 1800U; // Set RESUME line for 1-13 ms
            do {
                delay_count--;
            } while (delay_count);
            USBResumeControl = 0; //Finished driving resume signalling

            USBUnmaskInterrupts();
        }
    }
}

/*******************************************************************
 * Function:        BOOL USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, WORD size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  WORD size - size of the event data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This function is called from the USB stack to
 *                  notify a user application that a USB event
 *                  occured.  This callback is in interrupt context
 *                  when the USB_INTERRUPT option is selected.
 *
 * Note:            None
 *******************************************************************/
BOOL USER_USB_CALLBACK_EVENT_HANDLER(USB_EVENT event, void *pdata, WORD size) {
    switch ((INT) event) {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;
        case EVENT_SOF:
            USBCB_SOF_Handler();
            break;
        case EVENT_SUSPEND:
            USBCBSuspend();
            break;
        case EVENT_RESUME:
            USBCBWakeFromSuspend();
            break;
        case EVENT_CONFIGURED:
            USBCBInitEP();
            break;
        case EVENT_SET_DESCRIPTOR:
            USBCBStdSetDscHandler();
            break;
        case EVENT_EP0_REQUEST:
            USBCBCheckOtherReq();
            break;
        case EVENT_BUS_ERROR:
            USBCBErrorHandler();
            break;
        case EVENT_TRANSFER_TERMINATED:
            //Add application specific callback task or callback function here if desired.
            //The EVENT_TRANSFER_TERMINATED event occurs when the host performs a CLEAR
            //FEATURE (endpoint halt) request on an application endpoint which was 
            //previously armed (UOWN was = 1).  Here would be a good place to:
            //1.  Determine which endpoint the transaction that just got terminated was 
            //      on, by checking the handle value in the *pdata.
            //2.  Re-arm the endpoint if desired (typically would be the case for OUT 
            //      endpoints).

            //Check if the host recently did a clear endpoint halt on the MSD OUT endpoint.
            //In this case, we want to re-arm the MSD OUT endpoint, so we are prepared
            //to receive the next CBW that the host might want to send.
            //Note: If however the STALL was due to a CBW not valid condition, 
            //then we are required to have a persistent STALL, where it cannot 
            //be cleared (until MSD reset recovery takes place).  See MSD BOT 
            //specs v1.0, section 6.6.1.
            if (MSDWasLastCBWValid() == FALSE) {
                //Need to re-stall the endpoints, for persistent STALL behavior.
                USBStallEndpoint(MSD_DATA_IN_EP, IN_TO_HOST);
                USBStallEndpoint(MSD_DATA_OUT_EP, OUT_FROM_HOST);
            } else {
                //Check if the host cleared halt on the bulk out endpoint.  In this
                //case, we should re-arm the endpoint, so we can receive the next CBW.
                if ((USB_HANDLE) pdata == USBGetNextHandle(MSD_DATA_OUT_EP, OUT_FROM_HOST)) {
                    USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP, (BYTE*) & msd_cbw, MSD_OUT_EP_SIZE);
                }
            }
            break;
        default:
            break;
    }
    return TRUE;
}

/** EOF main.c ***************************************************************/







void __attribute__((interrupt, no_auto_psv)) _CNInterrupt(void) {

    IFS1bits.CNIF = 0; // clear IF
    sw_power = PORTEbits.RE4; // read for next interrupt

}

//************************* Interrupt service routine for RTCC *****************************

void __attribute__((interrupt, no_auto_psv)) _RTCCInterrupt(void) {
    mRtcc_Clear_Intr_Status_Bit; //clear the interrupt status
}

void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void) {
    T3_Clear_Intr_Status_Bit;
    //clear the interrupt flag

    //                        toggle_led1();
    //        segundos=RtccTimeDateVal.f.sec;
    //        minutos=RtccTimeDateVal.f.min;
    //        RtccReadTimeDate(&RtccTimeDateVal);
    //        if(RtccTimeDateVal.f.sec != segundos){
    //                    mPORTFToggleBits(BIT_4);
    //                       }
}

void __attribute__((interrupt, no_auto_psv)) _ADC1Interrupt(void) {

    IFS0bits.AD1IF = 0; //Clear the ADC1 Interrupt Flag
    adcPtr = ADC1BUF0;
//    if (adcPtr <= 546) { //en este valor tenemos aproximadamente 3.5V en la bateria
//        //            prender led rojo
//        //                    mPORTGToggleBits(BIT_6);
//        if (logFile != NULL) { //y el file esta abierto, lo cierra
//            FSfclose(logFile);
//            logFile = NULL;
//        }
//        standby_mode();
//        measurement = 0;
//        //to do apagar modulos y led
//        //con watch dog podemos apagar y prender un led
//        /* tenemos que definir adcPtr como volatil
//         prender led
//         * delay
//         * apagar led
//         *
//         * EnableWDT(WDT_ENABLE);
//         * sleep
//         * Disable wdt
//         * }
//         */
//        mPORTGClearBits(BIT_6);
//        //                    to do apagar todos los modulos
//        CloseTimer3();
//        CloseSPI2();
//        CloseSPI1();
//        AD1CON1bits.ADON = 0;
//
//        Sleep();
//    }




}

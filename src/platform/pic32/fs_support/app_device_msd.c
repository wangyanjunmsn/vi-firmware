/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/
#include "fs_platforms.h"
#include "system.h"
#include "system_config.h"
#include "USB/usb.h"

#include "USB/usb_function_msd.h"
#include "fileio.h"
#include "sd_spi.h"


#ifdef FS_SUPPORT
/** VARIABLES ******************************************************/
// The sdCardMediaParameters structure defines user-implemented functions needed by the SD-SPI fileio driver.
// The driver will call these when necessary.  For the SD-SPI driver, the user must provide
// parameters/functions to define which SPI module to use, Set/Clear the chip select pin,
// get the status of the card detect and write protect pins, and configure the CS, CD, and WP
// pins as inputs/outputs (as appropriate).
// For this demo, these functions are implemented in system.c, since the functionality will change
// depending on which demo board/microcontroller you're using.
// This structure must be maintained as long as the user wishes to access the specified drive.
extern FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters;


//The LUN variable definition is critical to the MSD function driver.  This
//  array is a structure of function pointers that are the functions that
//  will take care of each of the physical media.  For each additional LUN
//  that is added to the system, an entry into this array needs to be added
//  so that the stack can know where to find the physical layer functions.
//  In this example the media initialization function is named
//  "MediaInitialize", the read capacity function is named "ReadCapacity",
//  etc.

void * MSD_SPI_MediaInitialize(void){
	return ((void *)FILEIO_SD_MediaInitialize (&sdCardMediaParameters));
}
uint32_t MSD_SPI_CapacityRead(void){
	return((uint32_t)FILEIO_SD_CapacityRead(&sdCardMediaParameters));
}
uint16_t MSD_SPI_SectorSizeRead(void){
	return FILEIO_SD_SectorSizeRead(&sdCardMediaParameters);
}
bool MSD_SPI_MediaDetect(void){
	return (FILEIO_SD_MediaDetect(&sdCardMediaParameters));
}

bool MSD_SPI_SectorRead(uint32_t sectorAddress, uint8_t* buffer){
	return  FILEIO_SD_SectorRead(&sdCardMediaParameters,sectorAddress,buffer);
}

bool MSD_SPI_WriteProtectStateGet(void){
	return FILEIO_SD_WriteProtectStateGet(&sdCardMediaParameters);
}

uint8_t MSD_SPI_SectorWrite(uint32_t sectorAddress, uint8_t* buffer, uint8_t allowWriteToZero){
	 return FILEIO_SD_SectorWrite(&sdCardMediaParameters, sectorAddress, buffer, (bool)allowWriteToZero);
}

LUN_FUNCTIONS LUN[MAX_LUN + 1] =
{
    {
        &MSD_SPI_MediaInitialize,
        &MSD_SPI_CapacityRead,
        &MSD_SPI_SectorSizeRead,
        &MSD_SPI_MediaDetect,
        &MSD_SPI_SectorRead,
        &MSD_SPI_WriteProtectStateGet,
        &MSD_SPI_SectorWrite,
    }
};


/* Standard Response to INQUIRY command stored in ROM 	*/
const InquiryResponse inq_resp = {
	0x00,		// peripheral device is connected, direct access block device
	0x80,       // removable
	0x04,	 	// version = 00=> does not conform to any standard, 4=> SPC-2
	0x02,		// response is in format specified by SPC-2
	0x20,		// n-4 = 36-4=32= 0x20
	0x00,		// sccs etc.
	0x00,		// bque=1 and cmdque=0,indicates simple queueing 00 is obsolete,
                // but as in case of other device, we are just using 00
	0x00,		// 00 obsolete, 0x80 for basic task queueing
	'M','i','c','r','o','c','h','p',    	// this is the T10 assigned Vendor ID
	'M','a','s','s',' ','S','t','o','r','a','g','e',' ',' ',' ',' ',
	'0','0','0','1'
};


/*********************************************************************
* Function: void APP_DeviceMSDInitialize(void);
*
* Overview: Initializes the Custom HID demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceMSDInitialize(void)
{
    #if (MSD_DATA_IN_EP == MSD_DATA_OUT_EP)
        USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    #else
        USBEnableEndpoint(MSD_DATA_IN_EP,USB_IN_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
        USBEnableEndpoint(MSD_DATA_OUT_EP,USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);
    #endif

    USBMSDInit();
}

/*********************************************************************
* Function: void APP_DeviceMSDTasks(void);
*
* Overview: Keeps the Custom HID demo running.
*
* PreCondition: The demo should have been initialized and started via
*   the APP_DeviceMSDInitialize() and APP_DeviceMSDStart() demos
*   respectively.
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceMSDTasks(void)
{
	
    MSDTasks();
}

void MSDActivity(void){

	#if defined(USB_POLLING)
		// Interrupt or polling method.  If using polling, must call
		// this function periodically.  This function will take care
		// of processing and responding to SETUP transactions
		// (such as during the enumeration process when you first
		// plug in).  USB hosts require that USB devices should accept
		// and process SETUP packets in a timely fashion.  Therefore,
		// when using polling, this function should be called
		// regularly (such as once every 1.8ms or faster** [see
		// inline code comments in usb_device.c for explanation when
		// "or faster" applies])  In most cases, the USBDeviceTasks()
		// function does not take very long to execute (ex: <100
		// instruction cycles) before it returns.
		USBDeviceTasks();
	#endif
		
	if( USBGetDeviceState() < CONFIGURED_STATE )
	{
		return;
	}
	/* If we are currently suspended, then we need to see if we need to
	 * issue a remote wakeup.  In either case, we shouldn't process any
	 * keyboard commands since we aren't currently communicating to the host
	 * thus just continue back to the start of the while loop. */
	if( USBIsDeviceSuspended() == true )
	{
		return;
	}
	APP_DeviceMSDTasks();
}


/*******************************************************************
 * Function:        bool USER_USB_CALLBACK_EVENT_HANDLER(
 *                        USB_EVENT event, void *pdata, uint16_t size)
 *
 * PreCondition:    None
 *
 * Input:           USB_EVENT event - the type of event
 *                  void *pdata - pointer to the event data
 *                  uint16_t size - size of the event data
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
bool USER_USB_CALLBACK_EVENT_HANDLER_MSD(USB_EVENT event, void *pdata, uint16_t size)
{
	
    switch((int)event)
    {
        case EVENT_TRANSFER:
            //Add application specific callback task or callback function here if desired.
            break;

        case EVENT_SOF:
            break;

        case EVENT_SUSPEND:
            break;

        case EVENT_RESUME:
            break;

        case EVENT_CONFIGURED:
            /* When the device is configured, we can (re)initialize the demo
             * code. */
            APP_DeviceMSDInitialize();
            break;

        case EVENT_SET_DESCRIPTOR:
            break;

        case EVENT_EP0_REQUEST:
            /* We have received a non-standard USB request.  The MSD driver
             * needs to check to see if the request was for it. */
            USBCheckMSDRequest();
            break;

        case EVENT_BUS_ERROR:
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
          //  if(MSDWasLastCBWValid() == false)
           // {
          //      //Need to re-stall the endpoints, for persistent STALL behavior.
          //      USBStallEndpoint(MSD_DATA_IN_EP, IN_TO_HOST);
          //      USBStallEndpoint(MSD_DATA_OUT_EP, OUT_FROM_HOST);
          //  }
          //  else
           // {
                //Check if the host cleared halt on the bulk out endpoint.  In this
                //case, we should re-arm the endpoint, so we can receive the next CBW.
           //     if((USB_HANDLE)pdata == USBGetNextHandle(MSD_DATA_OUT_EP, OUT_FROM_HOST))
             //   {
             //       USBMSDOutHandle = USBRxOnePacket(MSD_DATA_OUT_EP, (uint8_t*)&msd_cbw, MSD_OUT_EP_SIZE);
            //    }
            //}

			
            break;

        default:
            break;
    }
    return true;
}

#endif
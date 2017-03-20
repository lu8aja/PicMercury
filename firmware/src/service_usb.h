/* 
 * File:   service_usb.h
 * Author: Javier
 *
 * Created on March 19, 2017, 6:36 PM
 */

#define LIB_USB


#include "app_globals.h"
#include "usb/usb.h"
#include "usb/usb_device_cdc.h"


#define USB_serviceDeviceTasks()     USBDeviceTasks()    // @ usb_device
#define USB_InterruptFlag            USBInterruptFlag    // @ usb_hal_pic18


#define USB_available()              (USB_getDeviceState() == CONFIGURED_STATE   && !USB_isDeviceSuspended())

#define USB_isOutputAvailable()      (USB_available() && USBUSARTIsTxTrfReady())


inline void USB_service(void);

void USB_input(void);

void USB_output(void);

void USB_configured(void);
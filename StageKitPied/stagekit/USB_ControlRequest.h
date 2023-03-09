#ifndef _USB_CONTROLREQUEST_H_
#define _USB_CONTROLREQUEST_H_

#define USB_MAX_BUFFER_SIZE 256

typedef struct {
	unsigned char bRequestType;
	unsigned char bRequest;
	unsigned short wValue;
	unsigned short wIndex;
	unsigned short wLength;
} USB_ControlRequestHeader;

typedef struct {
	USB_ControlRequestHeader header;
	unsigned char data[USB_MAX_BUFFER_SIZE];
} USB_ControlRequest;

#endif

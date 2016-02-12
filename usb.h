#ifndef __USB_H__
#define __USB_H__

#include <pthread.h>
#include <semaphore.h>
#include <libusb-1.0/libusb.h>

#define USB_OK						0
#define USB_ERROR_BASE				-100
#define USB_ERROR_PARAM				(USB_ERROR_BASE-1)
#define USB_ERROR_MUTEX				(USB_ERROR_BASE-2)
#define USB_ERROR_SEMAPHORE			(USB_ERROR_BASE-3)
#define USB_ERROR_MEMORY			(USB_ERROR_BASE-4)

typedef struct _usb_context
{
	libusb_context *ctx;
	pthread_t thread_events;
	int thread_events_valid;
	pthread_mutex_t mutex_devcount;
	int devcount;
	sem_t sem_stop;
} usb_context;

typedef struct _usb_device_handle
{
	libusb_device_handle *handle;
	usb_context *ctx;
} usb_device_handle;

int usb_init(usb_context **context);
void usb_exit(usb_context *context);
usb_device_handle *usb_open_device_with_vid_pid(usb_context *ctx, int vid, int pid);
void usb_close(usb_device_handle *dev);

#endif // __USB_H__

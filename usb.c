#include "usb.h"
#include <stdlib.h>

static void *usb_event_thread_proc(void *p)
{
	usb_context *ctx = p;
	
	while (sem_trywait(&ctx->sem_stop) != 0)
	{
		libusb_handle_events(ctx->ctx);
	}
	
	return NULL;
}

int usb_init(usb_context **context)
{
	int retval;
	usb_context *ctx;
	
	if (context == NULL)
	{
		return USB_ERROR_PARAM;
	}
	
	ctx = malloc(sizeof(usb_context));
	
	if (!ctx)
	{
		return USB_ERROR_MEMORY;
	}
	
	ctx->devcount = 0;
	ctx->thread_events_valid = 0;
	
	retval = pthread_mutex_init(&ctx->mutex_devcount, NULL);
	
	if (retval)
	{
		free(ctx);
		return USB_ERROR_MUTEX;
	}
	
	retval = sem_init(&ctx->sem_stop, 0, 0);
	
	if (retval)
	{
		pthread_mutex_destroy(&ctx->mutex_devcount);
		free(ctx);
		return USB_ERROR_SEMAPHORE;
	}
	
	retval = libusb_init(&ctx->ctx);
	
	if (retval)
	{
		sem_destroy(&ctx->sem_stop);
		pthread_mutex_destroy(&ctx->mutex_devcount);
		free(ctx);
		return retval;
	}
	
	*context = ctx;
	
	return USB_OK;
}

void usb_exit(usb_context *context)
{
	if (!context)
	{
		return;
	}
	
	libusb_exit(context->ctx);
	sem_destroy(&context->sem_stop);
	pthread_mutex_destroy(&context->mutex_devcount);
}

usb_device_handle *usb_open_device_with_vid_pid(usb_context *ctx, int vid, int pid)
{
	usb_device_handle *dev;
	int retval;
	
	if (!ctx)
	{
		return NULL;
	}
	
	dev = malloc(sizeof(usb_device_handle));
	
	if (!dev)
	{
		return NULL;
	}
	
	dev->ctx = ctx;
	dev->handle = libusb_open_device_with_vid_pid(ctx->ctx, vid, pid);
	
	if (!dev->handle)
	{
		free(dev);
		return NULL;
	}
	
	pthread_mutex_lock(&ctx->mutex_devcount);
	
	if (ctx->devcount == 0)
	{
		// Create the event thread after opening the first device
		retval = pthread_create(&ctx->thread_events, NULL, usb_event_thread_proc, ctx);
		
		if (retval)
		{
			libusb_close(dev->handle);
			pthread_mutex_unlock(&ctx->mutex_devcount);
			free(dev);
			return NULL;
		}
	}
	
	ctx->devcount++;
	
	pthread_mutex_unlock(&ctx->mutex_devcount);
	
	return dev;
}

void usb_close(usb_device_handle *dev)
{
	if (!dev)
	{
		return;
	}
	
	pthread_mutex_lock(&dev->ctx->mutex_devcount);
	
	if (dev->ctx->devcount == 1)
	{
		// Signal the event thread to stop
		sem_post(&dev->ctx->sem_stop);
	}
	
	libusb_close(dev->handle);
	
	if (dev->ctx->devcount == 1)
	{
		// Wait for the event thread
		pthread_join(dev->ctx->thread_events, NULL);
	}
	
	dev->ctx->devcount--;
	
	pthread_mutex_unlock(&dev->ctx->mutex_devcount);
}

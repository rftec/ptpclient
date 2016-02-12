/* 
 * PTP client v0.1 for Sony Alpha 6000 (ILCE-6000)
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <libusb-1.0/libusb.h>
#include "ptp.h"
#include "ptp-pima.h"
#include "ptp-sony.h"
#include "timer.h"
#include "usb.h"

//#define IMAGE_PATH "images"	// Target path, undefine to disable saving
//#define IMAGE_PATH "/media/ubuntu/Data/CameraImages"
#define IMAGE_COUNT 30			// Minimal number of images to capture
//#define OBJECT_POLL_PENDING	// Define to poll the "Pending images" property instead of polling events
#define USE_EVENT_CALLBACK		// Define to use the event callback instead of polling


#ifndef USE_EVENT_CALLBACK
sem_t sem_stop_polling;
#endif

sem_t sem_objects;
uint8_t print_log = 1;

void plog(int ret, const char *desc)
{
	if (print_log)
	{
		printf("%s: %d\n", desc, ret);
	}
}

void on_object_added(uint32_t handle)
{
	struct timeval tv_now;
	
	// Notify the main thread that an object was added
	sem_post(&sem_objects);
	
	gettimeofday(&tv_now, NULL);
	printf("[%10ld.%06ld] Got object 0x%08X\n", tv_now.tv_sec, tv_now.tv_usec, handle);
}

#ifdef USE_EVENT_CALLBACK

void event_callback(ptp_device *dev, const ptp_params *params, void *ctx)
{
	if (params->code == PTP_EC_SONY_ObjectAdded)
	{
		on_object_added((params->num_params > 0) ? params->params[0] : 0xFFFFC001);
	}
	
	printf("Got event %04Xh with parameter %08Xh\n", params->code, params->params[0]);
}

#else

void *poll_events(void *ptpdev)
{
	int ret;
	uint32_t object_handle;
	ptp_device *dev = (ptp_device *)ptpdev;
	
	if (!dev)
	{
		return NULL;
	}
	
	do
	{
		ret = ptp_sony_wait_object(dev, &object_handle, 1000);
		
		if (ret == PTP_OK)
		{
			on_object_added(object_handle);
		}
	}
	while (sem_trywait(&sem_stop_polling) != 0);
	
	return NULL;
}

#endif

#ifdef IMAGE_PATH
void save_image(void *data, int size, int index)
{
	FILE *f;
	char fname[256] = { };
	
	snprintf(fname, sizeof(fname) - 1, "%s/output-%d.jpg", IMAGE_PATH, index);
	
	f = fopen(fname, "wb");
	fwrite(data, 1, size, f);
	fclose(f);
}
#endif

int transfer_image(ptp_device *dev, uint32_t object_handle, int index)
{
	int retval;
	void *image_data;
	timer tm;
	
	timer_start(&tm);
	
	// Get the object's info
	retval = ptp_pima_get_object_info(dev, object_handle, NULL);
	
	if (retval != PTP_OK)
	{
		plog(retval, "ptp_pima_get_object_info()");
		return retval;
	}
	
	// Get the object's data
	retval = ptp_pima_get_object(dev, object_handle, &image_data);
	
	if (retval >= 0)
	{
		double rate, t;
		struct timeval tv;
		
		// Stop the timer
		timer_stop(&tm);
		timer_elapsed(&tm, &tv);
		
		// Print some stats
		printf("Image size:    %d bytes\n", retval);
		printf("Transfer time: ");
		timer_print_elapsed(&tm);
		
		t = tv.tv_sec + tv.tv_usec * 0.000001;
		rate = ((double)retval / t) / (1024.0 * 1024.0);
		
		printf("Transfer rate: %.2f MB/s\n", rate);
		
		#ifdef IMAGE_PATH
		// Save the image
		save_image(image_data, retval, index);
		#endif
		
		free(image_data);
		
		retval = PTP_OK;
	}
	else
	{
		plog(retval, "ptp_pima_get_object()");
	}
	
	return retval;
}

void take_pictures(ptp_device *dev, int count)
{
	int retval, taken, pending, prev_pending, ready;
	struct timeval tv_dur, tv_ppic;
	timer tm;
	uint64_t usec_ppic;
	
	taken = 0;
	pending = 0;
	prev_pending = 0;
	
	sleep(1);
	
	// Shutter press
	plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_AFLock, 2), "ptp_sony_set_control_device_b(0xD2C1, 0x0002)");
	plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_Shutter, 2), "ptp_sony_set_control_device_b(0xD2C2, 0x0002)");
	
	timer_start(&tm);
	
	while (taken < count || pending > 0)
	{
		retval = ptp_sony_get_pending_objects(dev);
		pending = retval & 0x7FFF;
		ready = (retval & 0x8000) ? 1 : 0;
		
		/*if (pending != prev_pending)
		{
			//printf("Pending objects: %d\n", pending);
			if (pending > prev_pending)
			{
				gettimeofday(&tv_end, NULL);
				printf("[%ld.%06ld] [%d/%d] New image pending     (pending: %d+%d)\n", tv_end.tv_sec, tv_end.tv_usec, taken + 1, count, pending, ready);
			}
			
			prev_pending = pending;
		}*/
		
		#ifdef OBJECT_POLL_PENDING // Poll using the "Pending objects" property
		
		if (ready)
		{
			
		#else // Poll using events
		
		if (taken < count || pending > 0)
		{
			sem_wait(&sem_objects);
			
		#endif
		
			gettimeofday(&tv_dur, NULL);
			printf("[%10ld.%06ld] [%d/%d] Transferring image... (pending: %d+%d)\n", tv_dur.tv_sec, tv_dur.tv_usec, taken + 1, count, pending, ready);
			
			retval = transfer_image(dev, 0xFFFFC001, taken);
			
			if (retval != PTP_OK)
			{
				break;
				// Do nothing for now
			}
			
			prev_pending--;
			taken++;
			
			if (taken == count)
			{
				// Shutter release
				plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_Shutter, 1), "ptp_sony_set_control_device_b(0xD2C2, 0x0001)");
				plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_AFLock, 1), "ptp_sony_set_control_device_b(0xD2C1, 0x0001)");
			}
		}
	}
	
	timer_stop(&tm);
	
	// Shutter release (just to make sure)
	plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_Shutter, 1), "ptp_sony_set_control_device_b(0xD2C2, 0x0001)");
	plog(ptp_sony_set_control_device_b(dev, PTP_DPC_SONY_CTRL_AFLock, 1), "ptp_sony_set_control_device_b(0xD2C1, 0x0001)");
	
	if (taken > 0)
	{
		timer_elapsed(&tm, &tv_dur);
		
		usec_ppic = (uint64_t)tv_dur.tv_sec * 1000000 + tv_dur.tv_usec;
		usec_ppic /= taken;
		tv_ppic.tv_sec = usec_ppic / 1000000;
		tv_ppic.tv_usec = usec_ppic % 1000000;
		
		printf(
			"Took %d pictures in %ld.%06ld seconds (%ld.%06ld sec/pic)\n", 
			taken, 
			tv_dur.tv_sec, 
			tv_dur.tv_usec, 
			tv_ppic.tv_sec, 
			tv_ppic.tv_usec
		);
	}
	else
	{
		printf("No pictures taken\n");
	}
}

void wait_property(ptp_device *ptpdev)
{
	int retval;
	ptp_pima_prop_desc_list *list;
	
	do
	{
		ptp_pima_prop_code propcode;
		
		retval = ptp_sony_wait_property(ptpdev, &propcode, 0);
		
		if (retval == PTP_OK)
		{
			const char *pname = ptp_sony_get_prop_name(propcode);
			
			retval = ptp_pima_proplist_create(&list);
			
			if (retval == PTP_OK)
			{
				retval = ptp_sony_get_all_dev_prop_data(ptpdev, list);
				
				if (retval != PTP_OK)
				{
					ptp_pima_proplist_free(list);
					list = NULL;
				}
			}
			else
			{
				list = NULL;
			}
			
			printf("Property changed: <%04Xh> %s", propcode, pname ? pname : "?");
			
			if (list)
			{
				ptp_pima_prop_desc *desc = ptp_pima_proplist_get_prop(list, propcode);
				
				if (desc)
				{
					printf(" = ");
					ptp_pima_print_prop_value(desc->type, &desc->val);
				}
				
				ptp_pima_proplist_free(list);
			}
			
			printf("\n");
		}
	}
	while (retval == PTP_OK);
	
	plog(retval, "ptp_sony_wait_property");
}

void print_libusb_version(void)
{
	const struct libusb_version *v = libusb_get_version();
	
	printf("Using libusb version %d.%d.%d.%d\n", v->major, v->minor, v->micro, v->nano);
}

void print_device_speed(usb_device_handle *usbdev)
{
	libusb_device *dev;
	
	dev = libusb_get_device(usbdev->handle);
	
	if (dev)
	{
		int speed;
		const char *s;
		
		speed = libusb_get_device_speed(dev);
		
		switch (speed)
		{
		case LIBUSB_SPEED_LOW:
			s = "low speed (1.5Mbps)";
			break;
			
		case LIBUSB_SPEED_FULL:
			s = "full speed (12Mbps)";
			break;
			
		case LIBUSB_SPEED_HIGH:
			s = "high speed (480Mbps)";
			break;
			
		case LIBUSB_SPEED_SUPER:
			s = "super speed (5Gbps)";
			break;
			
		case LIBUSB_SPEED_UNKNOWN:
		default:
			s = "unknown speed";
			break;
		}
		
		printf("Device is operating at %s\n", s);
	}
}

int main(int argc, char **argv)
{
	int ret;
	usb_context *ctx;
	//libusb_device **list;
	//libusb_device *found = NULL;
	//ssize_t cnt, i;
	usb_device_handle *usbdev;
	ptp_device *ptpdev;
	ptp_pima_prop_desc_list *list;
	ptp_pima_device_info *devinfo;
	#ifndef USE_EVENT_CALLBACK
	pthread_t thread_poll;
	#endif
	
	ret = sem_init(&sem_objects, 0, 0);
	
	if (ret)
	{
		printf("sem_init: %d\n", errno);
		exit(1);
	}
	
	#ifndef USE_EVENT_CALLBACK
	ret = sem_init(&sem_stop_polling, 0, 0);
	
	if (ret)
	{
		printf("sem_init: %d\n", errno);
		sem_destroy(&sem_objects);
		exit(1);
	}
	#endif
	
	ret = usb_init(&ctx);
	if (ret != 0)
	{
		printf("libusb_init: %d\n", ret);
		#ifndef USE_EVENT_CALLBACK
		sem_destroy(&sem_stop_polling);
		#endif
		sem_destroy(&sem_objects);
		exit(1);
	}
	
	print_libusb_version();
	
	// discover devices
	/*cnt = libusb_get_device_list(NULL, &list);
	
	if (cnt < 0)
	{
		printf("libusb_get_device_list: %d\n", cnt);
		libusb_exit(ctx);
		exit(1);
	}
	
	i = 0;
		
	for (i = 0; i < cnt; i++)
	{
		libusb_device *device = list[i];
		
		if (is_interesting(device))
		{
			found = device;
			break;
		}
	}*/
	
	/*if (found) {
		libusb_device_handle *handle;
		err = libusb_open(found, &handle);
		if (err)
			error();
		// etc
	}*/
	
	//libusb_free_device_list(list, 1);
	
	usbdev = usb_open_device_with_vid_pid(ctx, 0x054C, 0x094E);
	
	if (usbdev == NULL)
	{
		printf("Camera not detected\n");
		usb_exit(ctx);
		#ifndef USE_EVENT_CALLBACK
		sem_destroy(&sem_stop_polling);
		#endif
		sem_destroy(&sem_objects);
		exit(1);
	}
	
	print_device_speed(usbdev);
	
	#ifdef USE_EVENT_CALLBACK
	ret = ptp_device_init(&ptpdev, usbdev, event_callback, NULL);
	#else
	ret = ptp_device_init(&ptpdev, usbdev, NULL, NULL);
	#endif
	
	if (ret != PTP_OK)
	{
		printf("ptp_device_init: %d\n", ret);
		usb_close(usbdev);
		usb_exit(ctx);
		#ifndef USE_EVENT_CALLBACK
		sem_destroy(&sem_stop_polling);
		#endif
		sem_destroy(&sem_objects);
		exit(1);
	}
	
	#ifndef USE_EVENT_CALLBACK
	// Polling mode, create the polling thread
	ret = pthread_create(&thread_poll, NULL, poll_events, ptpdev);
	
	if (ret)
	{
		printf("pthread_create: %d\n", ret);
		ptp_device_free(ptpdev);
		usb_close(usbdev);
		usb_exit(ctx);
		#ifndef USE_EVENT_CALLBACK
		sem_destroy(&sem_stop_polling);
		#endif
		sem_destroy(&sem_objects);
		exit(1);
	}
	#endif
	
	// Initialize the camera
	plog(ptp_sony_sdio_connect(ptpdev, 1, 0, 0), "ptp_sony_sdio_connect(1, 0, 0)");
	plog(ptp_sony_sdio_connect(ptpdev, 2, 0, 0), "ptp_sony_sdio_connect(2, 0, 0)");
	plog(ptp_sony_get_sdio_ext_devinfo(ptpdev, 200, NULL), "ptp_sony_get_sdio_ext_devinfo(200 #1)");
	plog(ptp_sony_get_sdio_ext_devinfo(ptpdev, 200, NULL), "ptp_sony_get_sdio_ext_devinfo(200 #2)");
	plog(ptp_sony_sdio_connect(ptpdev, 3, 0, 0), "ptp_sony_sdio_connect(3, 0, 0)");
	
	// Create a device info object
	plog(ret = ptp_pima_devinfo_create(&devinfo), "ptp_pima_devinfo_create");
	
	if (ret == PTP_OK)
	{
		// Get the device's information
		plog(ret = ptp_pima_get_device_info(ptpdev, devinfo), "ptp_pima_get_device_info");
		plog(ret = ptp_sony_get_sdio_ext_devinfo(ptpdev, 200, devinfo), "ptp_sony_get_sdio_ext_devinfo");
	
		if (ret == PTP_OK)
		{
			// Print the info
			ptp_sony_print_device_info(devinfo);
		}
		
		ptp_pima_devinfo_free(devinfo);
	}
	
	// Create a property list
	plog(ret = ptp_pima_proplist_create(&list), "ptp_pima_proplist_create");
	
	if (ret != PTP_OK)
	{
		list = NULL;
	}
	
	// Get all the device's properties
	plog(ptp_sony_get_all_dev_prop_data(ptpdev, list), "ptp_sony_get_all_dev_prop_data()");
	
	if (list)
	{
		int i;
		
		// Print the properties
		for (i = 0; i < list->count; i++)
		{
			ptp_sony_print_prop_desc(&list->desc[i]);
		}
		
		ptp_pima_proplist_free(list);
	}
	
	#if 0
	// Wait for property changes and display them (polling mode only)
	wait_property(ptpdev);
	#endif
	
	// Take some pictures
	take_pictures(ptpdev, IMAGE_COUNT);
	
	#ifndef USE_EVENT_CALLBACK
	sem_post(&sem_stop_polling);
	pthread_join(thread_poll, NULL);
	#endif
	
	ptp_device_free(ptpdev);
	usb_close(usbdev);
	usb_exit(ctx);
	#ifndef USE_EVENT_CALLBACK
	sem_destroy(&sem_stop_polling);
	#endif
	sem_destroy(&sem_objects);
	return 0;
}

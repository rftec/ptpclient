#ifndef __PTP_H__
#define __PTP_H__

#include <stdint.h>
#include <endian.h>
#include <libusb-1.0/libusb.h>
#include "usb.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN	
	//#define htod16(x)	(x)
	//#define htod32(x)	(x)
	//#define dtoh16(x)	htod16(x)
	//#define dtoh32(x)	htod32(x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#else
	#error Unknown endianness
#endif

#define htod16(x)	htole16(x)
#define htod32(x)	htole32(x)
#define dtoh16(x)	le16toh(x)
#define dtoh32(x)	le32toh(x)

#define PTP_OK						0
#define PTP_ERROR_BASE				-50
#define PTP_ERROR_PARAM				(PTP_ERROR_BASE-1)
#define PTP_ERROR_MEMORY			(PTP_ERROR_BASE-2)
#define PTP_ERROR_DATA_LEN			(PTP_ERROR_BASE-3)
#define PTP_ERROR_TRANSACTION_ID	(PTP_ERROR_BASE-4)
#define PTP_ERROR_CONTAINER_TYPE	(PTP_ERROR_BASE-5)
#define PTP_ERROR_RC				(PTP_ERROR_BASE-6)
#define PTP_ERROR_EC				(PTP_ERROR_BASE-7)
#define PTP_ERROR_RESULT_PARAM		(PTP_ERROR_BASE-8)
#define PTP_ERROR_NOT_FOUND			(PTP_ERROR_BASE-9)
#define PTP_ERROR_PROP_TYPE			(PTP_ERROR_BASE-10)
#define PTP_ERROR_PROP_VALUE		(PTP_ERROR_BASE-11)

#define PTP_MAX_PARAMS	5

#define PTP_EVENT_TRANSFER_COUNT	10

typedef struct _ptp_params
{
	uint16_t code;
	uint32_t num_params;
	uint32_t params[PTP_MAX_PARAMS];
} ptp_params;

struct _ptp_device;
typedef struct _ptp_device	ptp_device;

typedef void (*ptp_event_callback)(ptp_device *dev, const ptp_params *params, void *ctx);

typedef struct _ptp_event_transfer
{
	ptp_device *dev;
	struct libusb_transfer *xfer;
	int completed;
	void *buf;
} ptp_event_transfer;

struct _ptp_device
{
	libusb_device_handle *usbdev;
	libusb_context *usbctx;
	uint32_t transaction_id;
	uint32_t recv_size;
	void *recv_buf;
	ptp_event_callback event_cb;
	void *user_ctx;
	ptp_event_transfer event_xfers[PTP_EVENT_TRANSFER_COUNT];
	struct libusb_transfer *bulk_xfer;
};

int ptp_device_init(ptp_device **dev, usb_device_handle *usbdev, ptp_event_callback event_cb, void *user_ctx);
void ptp_device_free(ptp_device *dev);
int ptp_transact(
	ptp_device *dev, 
	const ptp_params *params_out, const void *data_out, uint32_t data_out_size, 
	ptp_params *params_in, void **data_in, uint32_t *data_in_size);
int ptp_wait_event(ptp_device *dev, ptp_params *params, int timeout);

#endif /* __PTP_H__ */

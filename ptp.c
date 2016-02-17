#include "ptp.h"
#include "ptp-pima.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PTP_RETRY_COUNT	2

#define PTP_EP_IN		0x81
#define PTP_EP_OUT		0x02
#define PTP_EP_EVENT	0x83

typedef enum _ptp_container_type
{
	PTP_TYPE_UNDEF = 0,
	PTP_TYPE_COMMAND = 1,
	PTP_TYPE_DATA = 2,
	PTP_TYPE_RESPONSE = 3,
	PTP_TYPE_EVENT = 4
} ptp_container_type;

#pragma pack(push, 1)

typedef struct _ptp_container
{
	uint32_t len;
	uint16_t type;
	uint16_t code;
	uint32_t transaction_id;
} ptp_container;

typedef struct _ptp_command_container
{
	ptp_container container;
	uint32_t params[PTP_MAX_PARAMS];
} ptp_command_container, ptp_response_container, ptp_event_container;

#pragma pack(pop)

static void ptp_event_handle_completion(struct libusb_transfer *transfer, ptp_event_transfer *xfer);
static void ptp_event_transfer_callback(struct libusb_transfer *transfer);
static void ptp_submit_event_transfer(ptp_event_transfer *xfer);
static void ptp_cancel_event_transfer(ptp_event_transfer *xfer);
static void ptp_submit_event_transfers(ptp_device *dev);
static void ptp_cancel_event_transfers(ptp_device *dev);


int ptp_device_init(ptp_device **dev, usb_device_handle *usbdev, ptp_event_callback event_cb, void *user_ctx)
{
	int ret;
	
	if (!dev || !usbdev)
	{
		return PTP_ERROR_PARAM;
	}
	
	if (!(*dev = malloc(sizeof(ptp_device))))
	{
		return PTP_ERROR_MEMORY;
	}
	
	(*dev)->usbdev = usbdev->handle;
	(*dev)->usbctx = usbdev->ctx->ctx;
	(*dev)->transaction_id = (uint32_t)-1;
	(*dev)->recv_size = 512;
	(*dev)->event_cb = event_cb;
	(*dev)->user_ctx = user_ctx;
	(*dev)->bulk_xfer = NULL;
	memset((*dev)->event_xfers, 0, sizeof((*dev)->event_xfers));
	
	(*dev)->recv_buf = malloc((*dev)->recv_size);
	
	if (!((*dev)->recv_buf))
	{
		free(*dev);
		return PTP_ERROR_MEMORY;
	}
	
	ret = libusb_claim_interface((*dev)->usbdev, 0);
	
	if (ret < 0)
	{
		fprintf(stderr, "libusb_claim_interface: %d: Could not claim interface 0\n", ret);
		free((*dev)->recv_buf);
		free(*dev);
		return ret;
	}
	
	if (event_cb)
	{
		ptp_submit_event_transfers(*dev);
	}
	
	(*dev)->bulk_xfer = libusb_alloc_transfer(0);
	
	if (!(*dev)->bulk_xfer)
	{
		ptp_cancel_event_transfers(*dev);
		libusb_release_interface((*dev)->usbdev, 0);
		free((*dev)->recv_buf);
		free(*dev);
		return LIBUSB_ERROR_NO_MEM;
	}
	
	ret = ptp_pima_open_session(*dev, 1);
	
	if (ret != PTP_OK)
	{
		ptp_cancel_event_transfers(*dev);
		libusb_release_interface((*dev)->usbdev, 0);
		free((*dev)->recv_buf);
		free(*dev);
		return ret;
	}
	
	return PTP_OK;
}

void ptp_device_free(ptp_device *dev)
{
	if (dev)
	{
		ptp_pima_close_session(dev);
		libusb_free_transfer(dev->bulk_xfer);
		ptp_cancel_event_transfers(dev);
		libusb_release_interface(dev->usbdev, 0);
		free(dev->recv_buf);
		free(dev);
	}
}

static void ptp_event_handle_completion(struct libusb_transfer *transfer, ptp_event_transfer *xfer)
{
	uint32_t len;
	ptp_params params;
	ptp_event_container *event;
	int i;
	
	if (!xfer->dev->event_cb)
	{
		return;
	}
	
	event = (ptp_event_container *)xfer->buf;
	
	if (transfer->actual_length < sizeof(event->container))
	{
		return;
	}
	
	len = dtoh32(event->container.len);
		
	if (len != (uint32_t)transfer->actual_length)
	{
		return;
	}
	
	if (event->container.type != htod32(PTP_TYPE_EVENT))
	{
		return;
	}
	
	params.code = dtoh16(event->container.code);
	params.num_params = (len - sizeof(event->container)) / sizeof(event->params[0]);
	
	for (i = 0; i < params.num_params; i++)
	{
		params.params[i] = dtoh32(event->params[i]);
	}
	
	xfer->dev->event_cb(xfer->dev, &params, xfer->dev->user_ctx);
}

static void ptp_event_transfer_callback(struct libusb_transfer *transfer)
{
	ptp_event_transfer *xfer = transfer->user_data;
	
	switch (transfer->status)
	{
	case LIBUSB_TRANSFER_CANCELLED:
		xfer->completed = 1;
		break;
		
	case LIBUSB_TRANSFER_COMPLETED:
		ptp_event_handle_completion(transfer, xfer);
		// Fall through and resubmit
		
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
	case LIBUSB_TRANSFER_OVERFLOW:
	default:
		if (libusb_submit_transfer(transfer) != 0)
		{
			xfer->completed = 1;
		}
		
		break;
	}
}

static void ptp_submit_event_transfer(ptp_event_transfer *xfer)
{
	xfer->completed = 0;
	xfer->buf = NULL;
	xfer->xfer = libusb_alloc_transfer(0);
	
	if (xfer->xfer)
	{
		int retval;
		
		xfer->buf = malloc(sizeof(ptp_event_container));
		
		if (!xfer->buf)
		{
			libusb_free_transfer(xfer->xfer);
			xfer->xfer = NULL;
		}
		else
		{
			libusb_fill_interrupt_transfer(
				xfer->xfer, 
				xfer->dev->usbdev, 
				PTP_EP_EVENT, 
				xfer->buf, 
				sizeof(ptp_event_container), 
				ptp_event_transfer_callback, 
				xfer, 
				0
			);
			
			retval = libusb_submit_transfer(xfer->xfer);
			
			if (retval)
			{
				// Failed to submit
				free(xfer->buf);
				libusb_free_transfer(xfer->xfer);
				xfer->buf = NULL;
				xfer->xfer = NULL;
			}
		}
	}
}

static void ptp_cancel_event_transfer(ptp_event_transfer *xfer)
{
	if (xfer->xfer)
	{
		libusb_cancel_transfer(xfer->xfer);
		
		// Wait for completion
		while (!xfer->completed) {
			libusb_handle_events_completed(xfer->dev->usbctx, &xfer->completed);
		}
		
		libusb_free_transfer(xfer->xfer);
		xfer->xfer = NULL;
	}
	
	if (xfer->buf)
	{
		free(xfer->buf);
		xfer->buf = NULL;
	}
}

static void ptp_submit_event_transfers(ptp_device *dev)
{
	int i;

	for (i = 0; i < PTP_EVENT_TRANSFER_COUNT; i++)
	{
		dev->event_xfers[i].dev = dev;
		
		ptp_submit_event_transfer(&dev->event_xfers[i]);
	}
}

static void ptp_cancel_event_transfers(ptp_device *dev)
{
	int i;

	for (i = 0; i < PTP_EVENT_TRANSFER_COUNT; i++)
	{
		ptp_cancel_event_transfer(&dev->event_xfers[i]);
	}
}

// Based on libusb-1.0.19/libusb/sync.c
static void LIBUSB_CALL ptp_bulk_callback(struct libusb_transfer *transfer)
{
	int *completed = transfer->user_data;
	*completed = 1;
}

// Based on libusb-1.0.19/libusb/sync.c
static void ptp_transfer_wait_for_completion(ptp_device *dev)
{
	int r, *completed = dev->bulk_xfer->user_data;

	while (!*completed) {
		r = libusb_handle_events_completed(dev->usbctx, completed);
		if (r < 0) {
			if (r == LIBUSB_ERROR_INTERRUPTED)
				continue;
			libusb_cancel_transfer(dev->bulk_xfer);
			continue;
		}
	}
}

// Based on libusb-1.0.19/libusb/sync.c
static int ptp_bulk_transfer(ptp_device *dev, unsigned char endpoint, void *data, int length, int *transferred)
{
	int completed = 0;
	int r;
	
	if (!transferred)
	{
		return PTP_ERROR_PARAM;
	}

	if (endpoint & 0x80)
	{
		// IN endpoint
		
		do
		{
			*transferred = 0;
			r = libusb_bulk_transfer(dev->usbdev, endpoint, data, length, transferred, 0);
			
			if (r == 0 && *transferred == 0)
			{
				fprintf(stderr, "WARNING: Detected zero-length packet\n");
			}
		}
		while (r == 0 && *transferred == 0);
		
		return r;
	}
	else
	{
		// OUT endpoint
		
		if (!dev->bulk_xfer)
		{
			return libusb_bulk_transfer(dev->usbdev, endpoint, data, length, transferred, 0);
		}

		libusb_fill_bulk_transfer(dev->bulk_xfer, dev->usbdev, endpoint, data, length, ptp_bulk_callback, &completed, 0);
		dev->bulk_xfer->type = LIBUSB_TRANSFER_TYPE_BULK;

		completed = 0;
		r = libusb_submit_transfer(dev->bulk_xfer);
		
		if (r < 0)
		{
			return r;
		}

		ptp_transfer_wait_for_completion(dev);
			
		*transferred = dev->bulk_xfer->actual_length;
		switch (dev->bulk_xfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			r = 0;
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			r = LIBUSB_ERROR_TIMEOUT;
			break;
		case LIBUSB_TRANSFER_STALL:
			r = LIBUSB_ERROR_PIPE;
			break;
		case LIBUSB_TRANSFER_OVERFLOW:
			r = LIBUSB_ERROR_OVERFLOW;
			break;
		case LIBUSB_TRANSFER_NO_DEVICE:
			r = LIBUSB_ERROR_NO_DEVICE;
			break;
		case LIBUSB_TRANSFER_ERROR:
		case LIBUSB_TRANSFER_CANCELLED:
			r = LIBUSB_ERROR_IO;
			break;
			
		default:
			r = LIBUSB_ERROR_OTHER;
		}
		
		return r;
	}
}

static int ptp_send(ptp_device *dev, void *data, int size)
{
	int retval, i, transferred;
	
	for (i = 0; i < PTP_RETRY_COUNT; i++)
	{
		retval = ptp_bulk_transfer(dev, PTP_EP_OUT, data, size, &transferred);
		
		if (retval == LIBUSB_ERROR_PIPE)
		{
			libusb_clear_halt(dev->usbdev, PTP_EP_OUT);
			
			if (i == PTP_RETRY_COUNT - 1)
			{
				fprintf(stderr, "[ptp_send] ptp_bulk_transfer: LIBUSB_ERROR_PIPE\n");
				return retval;
			}
		}
		else if (retval != 0)
		{
			fprintf(stderr, "[ptp_send] ptp_bulk_transfer: %d\n", retval);
			return retval;
		}
		else
		{
			break;
		}
	}
	
	return PTP_OK;
}

int ptp_send_command(ptp_device *dev, const ptp_params *params)
{
	ptp_command_container command;
	uint32_t i;
	int size;
	
	if (!dev || !params || params->num_params > PTP_MAX_PARAMS)
	{
		fprintf(stderr, "[ptp_send_command] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	size = sizeof(ptp_container) + params->num_params * sizeof(uint32_t);
	
	command.container.len = htod32((uint32_t)size);
	command.container.type = htod16(PTP_TYPE_COMMAND);
	command.container.code = htod16(params->code);
	command.container.transaction_id = htod32(dev->transaction_id);
	
	for (i = 0; i < params->num_params; i++)
	{
		command.params[i] = htod32(params->params[i]);
	}
	
	return ptp_send(dev, &command, size);
}

int ptp_send_data(ptp_device *dev, uint16_t code, const void *data, int size)
{
	int retval;
	ptp_container *container;
	
	if (!dev || !data || size < 0)
	{
		fprintf(stderr, "[ptp_send_data] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	container = malloc(sizeof(ptp_container) + size);
	
	if (!container)
	{
		fprintf(stderr, "[ptp_send_data] Invalid container: PTP_ERROR_MEMORY\n");
		return PTP_ERROR_MEMORY;
	}
	
	memcpy(container + 1, data, size);
	
	size += sizeof(ptp_container);
	
	container->len = htod32((uint32_t)size);
	container->type = htod16(PTP_TYPE_DATA);
	container->code = htod16(code);
	container->transaction_id = htod32(dev->transaction_id);
	
	retval = ptp_send(dev, container, size);
	
	free(container);
	
	return retval;
}

int ptp_recv_response(ptp_device *dev, ptp_params *params)
{
	int retval, transferred;
	uint32_t len, i;
	ptp_response_container response;
	
	if (!dev || !params)
	{
		fprintf(stderr, "[ptp_recv_response] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	retval = ptp_bulk_transfer(dev, PTP_EP_IN, &response, sizeof(response), &transferred);
	
	if (retval != 0 && retval != LIBUSB_ERROR_TIMEOUT)
	{
		fprintf(stderr, "[ptp_recv_response] ptp_bulk_transfer: %d\n", retval);
		return retval;
	}
	
	if (transferred < sizeof(response.container))
	{
		fprintf(stderr, "[ptp_recv_response] Data length too short: transferred=%d, retval=%d\n", transferred, retval);
		return PTP_ERROR_DATA_LEN;
	}
	
	len = dtoh32(response.container.len);
	
	if (len != (uint32_t)transferred)
	{
		fprintf(stderr, "[ptp_recv_response] Transfer length mismatch: transferred=%d, expected=%u, retval=%d\n", transferred, len, retval);
		return retval ? retval : PTP_ERROR_DATA_LEN;
	}
	
	if (dtoh32(response.container.transaction_id) != dev->transaction_id)
	{
		fprintf(stderr, "[ptp_recv_response] PTP_ERROR_CONTAINER_ID\n");
		return PTP_ERROR_TRANSACTION_ID;
	}
	
	if (response.container.type != htod32(PTP_TYPE_RESPONSE))
	{
		fprintf(stderr, "[ptp_recv_response] PTP_ERROR_CONTAINER_TYPE\n");
		return PTP_ERROR_CONTAINER_TYPE;
	}
	
	params->code = dtoh16(response.container.code);
	params->num_params = (len - sizeof(response.container)) / sizeof(response.params[0]);
	
	for (i = 0; i < params->num_params; i++)
	{
		params->params[i] = dtoh32(response.params[i]);
	}
	
	return PTP_OK;
}

int ptp_recv_data(ptp_device *dev, void **data)
{
	int retval, transferred, buf_size;
	uint32_t len;
	uint8_t *buf;
	ptp_container *container;
	
	if (!dev || !data)
	{
		fprintf(stderr, "[ptp_recv_data] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	container = dev->recv_buf;
	
	if (!container)
	{
		fprintf(stderr, "[ptp_recv_data] Invalid container (PTP_ERROR_MEMORY)\n");
		return PTP_ERROR_MEMORY;
	}
	
	retval = ptp_bulk_transfer(dev, PTP_EP_IN, container, dev->recv_size, &transferred);
	
	if (retval != 0 && retval != LIBUSB_ERROR_TIMEOUT)
	{
		fprintf(stderr, "[ptp_recv_data] ptp_bulk_transfer: %d\n", retval);
		return retval;
	}
	
	if (transferred < sizeof(ptp_container))
	{
		fprintf(stderr, "[ptp_recv_data] Data length too short: transferred=%d, retval=%d\n", transferred, retval);
		return retval ? retval : PTP_ERROR_DATA_LEN;
	}
	
	len = dtoh32(container->len);
	
	if (len > transferred && transferred < dev->recv_size)
	{
		fprintf(stderr, "[ptp_recv_data] Early termination: transferred=%d, len=%d, retval=%d\n", transferred, len, retval);
		return retval ? retval : PTP_ERROR_DATA_LEN;
	}
	
	if (dtoh32(container->transaction_id) != dev->transaction_id)
	{
		fprintf(stderr, "[ptp_recv_data] Transaction ID mismatch: transaction_id=0x%08x, expected=0x%08x\n", dtoh32(container->transaction_id), dev->transaction_id);
		return PTP_ERROR_TRANSACTION_ID;
	}
	
	if (container->type != htod32(PTP_TYPE_DATA))
	{
		fprintf(stderr, "[ptp_recv_data] PTP_ERROR_CONTAINER_TYPE\n");
		return PTP_ERROR_CONTAINER_TYPE;
	}
	
	buf_size = len - sizeof(ptp_container);
	buf = malloc(buf_size);
	
	if (!buf)
	{
		fprintf(stderr, "[ptp_recv_data] Could not allocate buffer (PTP_ERROR_MEMORY)\n");
		return PTP_ERROR_MEMORY;
	}
	
	memcpy(buf, container + 1, transferred - sizeof(ptp_container));
	
	if (len > transferred)
	{
		uint32_t remaining = len - (uint32_t)transferred;
		
		retval = libusb_bulk_transfer(
			dev->usbdev, 
			PTP_EP_IN, 
			buf + transferred - sizeof(ptp_container), 
			(int)remaining, 
			&transferred, 
			0);
		
		if (retval != 0 && retval != LIBUSB_ERROR_TIMEOUT)
		{
			free(buf);
			fprintf(stderr, "[ptp_recv_data] ptp_bulk_transfer #2: %d\n", retval);
			return retval;
		}
		
		if ((uint32_t)transferred != remaining)
		{
			free(buf);
			fprintf(stderr, "[ptp_recv_data] ptp_bulk_transfer #2: transferred=%d, remaining=%d\n", transferred, (int)remaining);
			return retval ? retval : PTP_ERROR_DATA_LEN;
		}
	}
	
	*data = buf;
	return buf_size;
}

int ptp_transact(
	ptp_device *dev, 
	const ptp_params *params_out, const void *data_out, uint32_t data_out_size, 
	ptp_params *params_in, void **data_in, uint32_t *data_in_size)
{
	int retval, temp_data_in_size;
	void *temp_data_in;
	
	if (!params_out || !params_in || params_out->num_params > PTP_MAX_PARAMS || 
		(data_out && data_in) || (data_in && !data_in_size))
	{
		fprintf(stderr, "[ptp_transact] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	if (!data_out) data_out_size = 0;
	
	if (data_in)
	{
		*data_in = NULL;
		*data_in_size = 0;
	}
	
	dev->transaction_id++;
	
	retval = ptp_send_command(dev, params_out);
	
	if (retval != PTP_OK)
	{
		fprintf(stderr, "[ptp_transact] ptp_send_command: %d\n", retval);
		return retval;
	}
	
	if (data_out != NULL)
	{
		retval = ptp_send_data(dev, params_out->code, data_out, data_out_size);
		
		if (retval != PTP_OK)
		{
			fprintf(stderr, "[ptp_transact] ptp_send_data: %d\n", retval);
			return retval;
		}
	}
	else if (data_in)
	{
		retval = ptp_recv_data(dev, &temp_data_in);
		
		if (retval < 0)
		{
			fprintf(stderr, "[ptp_transact] ptp_recv_data: %d\n", retval);
			return retval;
		}
		
		temp_data_in_size = retval;
	}
	
	retval = ptp_recv_response(dev, params_in);
	
	if (retval != PTP_OK)
	{
		if (data_in)
		{
			free(temp_data_in);
		}
		
		fprintf(stderr, "[ptp_transact] ptp_recv_response: %d\n", retval);
	}
	else if (data_in)
	{
		*data_in = temp_data_in;
		*data_in_size = temp_data_in_size;
	}
	
	return retval;
}

int ptp_wait_event(ptp_device *dev, ptp_params *params, int timeout)
{
	int retval, transferred;
	uint32_t len, i;
	ptp_event_container event;
	
	if (!dev || !params)
	{
		fprintf(stderr, "[ptp_wait_event] PTP_ERROR_PARAM\n");
		return PTP_ERROR_PARAM;
	}
	
	retval = libusb_interrupt_transfer(dev->usbdev, PTP_EP_EVENT, (unsigned char *)&event, sizeof(event), &transferred, timeout);
	
	if (retval != 0 && retval != LIBUSB_ERROR_TIMEOUT)
	{
		fprintf(stderr, "[ptp_wait_event] libusb_interrupt_transfer: %d\n", retval);
		return retval;
	}
	
	if (transferred < sizeof(event.container))
	{
		fprintf(stderr, "[ptp_wait_event] Data length too short: transferred=%d, retval=%d\n", transferred, retval);
		return retval ? retval : PTP_ERROR_DATA_LEN;
	}
	
	len = dtoh32(event.container.len);
	
	if (len != (uint32_t)transferred)
	{
		fprintf(stderr, "[ptp_wait_event] Transfer length mismatch: transferred=%d, expected=%u, retval=%d\n", transferred, len, retval);
		return retval ? retval : PTP_ERROR_DATA_LEN;
	}
	
	if (event.container.type != htod32(PTP_TYPE_EVENT))
	{
		fprintf(stderr, "[ptp_wait_event] PTP_ERROR_CONTAINER_TYPE\n");
		return PTP_ERROR_CONTAINER_TYPE;
	}
	
	params->code = dtoh16(event.container.code);
	params->num_params = (len - sizeof(event.container)) / sizeof(event.params[0]);
	
	for (i = 0; i < params->num_params; i++)
	{
		params->params[i] = dtoh32(event.params[i]);
	}
	
	return PTP_OK;
}

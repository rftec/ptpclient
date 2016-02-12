#include "ptp-sony.h"
#include <stdlib.h>
#include <stdio.h>

#define PTP_SONY_READ_OPT	0

static const ptp_pima_code_name g_prop_names[] = {
	{ PTP_DPC_SONY_DPCCompensation,		"DPC Compensation" },
	{ PTP_DPC_SONY_DRangeOptimize,		"D-Range Optimize" },
	{ PTP_DPC_SONY_ImageSize,			"Image size" },
	{ PTP_DPC_SONY_ShutterSpeed,		"Shutter speed" },
	{ PTP_DPC_SONY_ColorTemp,			"Color temperature" },
	{ PTP_DPC_SONY_CCFilter,			"CC Filter" },
	{ PTP_DPC_SONY_AspectRatio,			"Aspect ratio" },
	{ PTP_DPC_SONY_PendingImages,		"Pending images" },
	{ PTP_DPC_SONY_ExposeIndex,			"Exposure index" },
	{ PTP_DPC_SONY_BatteryLevel,		"Battery level" },
	{ PTP_DPC_SONY_PictureEffect,		"Picture effect" },
	{ PTP_DPC_SONY_ABFilter,			"AB Filter" },
	{ PTP_DPC_SONY_ISO,					"ISO" },
	{ PTP_DPC_SONY_CTRL_AFLock,			"<CTRL> AF Lock" },
	{ PTP_DPC_SONY_CTRL_Shutter,		"<CTRL> Shutter" },
	{ PTP_DPC_SONY_CTRL_AELock,			"<CTRL> AE Lock" },
	{ PTP_DPC_SONY_CTRL_StillImage,		"<CTRL> Still image" },
	{ PTP_DPC_SONY_CTRL_Movie,			"<CTRL> Movie" },
	{ 0x0000,							NULL }
};

static const ptp_pima_code_name g_op_names[] = {
	{ PTP_OP_SONY_SDIOCONNECT,			"SDIOConnect" },
	{ PTP_OP_SONY_GETSDIOEXTDEVINFO,	"GetSDIOExtDevInfo" },
	{ PTP_OP_SONY_GETDEVICEPROPDESC,	"GetDevicePropDesc" },
	{ PTP_OP_SONY_SETCONTROLDEVICEA,	"SetControlDeviceA" },
	{ PTP_OP_SONY_SETCONTROLDEVICEB,	"SetControlDeviceB" },
	{ PTP_OP_SONY_GETALLDEVPROPDATA,	"GetAllDevPropData" },
	{ 0x0000,							NULL }
};

#define cr(x)		do { int _cr_ret = x; if (_cr_ret != PTP_OK) return _cr_ret; } while (0)

int ptp_sony_sdio_connect(ptp_device *dev, uint32_t param1, uint32_t param2, uint32_t param3)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	params_out.code = PTP_OP_SONY_SDIOCONNECT;
	params_out.num_params = 3;
	params_out.params[0] = param1;
	params_out.params[1] = param2;
	params_out.params[2] = param3;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	free(data);
	
	if (params_in.code != PTP_RC_OK)
	{
		return PTP_ERROR_RC;
	}
	
	return PTP_OK;
}

int ptp_sony_get_sdio_ext_devinfo(ptp_device *dev, uint32_t version, ptp_pima_device_info *info)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	params_out.code = PTP_OP_SONY_GETSDIOEXTDEVINFO;
	params_out.num_params = 1;
	params_out.params[0] = version;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	if (info)
	{
		ptp_pima_decode_context ctx;
		
		ctx.ptr = data;
		ctx.size = data_size;
		ctx.buf = info->buf;
		
		retval = ptp_sony_decode_device_info(&ctx, info);
		
		if (retval != PTP_OK)
		{
			free(data);
			return retval;
		}
	}
	
	free(data);
	
	if (params_in.code != PTP_RC_OK)
	{
		return PTP_ERROR_RC;
	}
	
	return PTP_OK;
}

int ptp_sony_get_all_dev_prop_data(ptp_device *dev, ptp_pima_prop_desc_list *list)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	params_out.code = PTP_OP_SONY_GETALLDEVPROPDATA;
	params_out.num_params = 0;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	if (list)
	{
		ptp_pima_decode_context ctx;
		
		ctx.ptr = data;
		ctx.size = data_size;
		ctx.buf = list->buf;
		
		retval = ptp_sony_decode_prop_desc_list(&ctx, list);
		
		if (retval != PTP_OK)
		{
			free(data);
			return retval;
		}
	}
	
	free(data);
	
	if (params_in.code != PTP_RC_OK)
	{
		return PTP_ERROR_RC;
	}
	
	return PTP_OK;
}

int ptp_sony_set_control_device_b(ptp_device *dev, uint32_t propcode, uint16_t value)
{
	ptp_params params_out, params_in;
	int retval;
	uint16_t data;
	
	params_out.code = PTP_OP_SONY_SETCONTROLDEVICEB;
	params_out.num_params = 1;
	params_out.params[0] = propcode;
	
	data = htod16(value);
	
	retval = ptp_transact(dev, &params_out, &data, sizeof(data), &params_in, NULL, NULL);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	if (params_in.code != PTP_RC_OK)
	{
		return PTP_ERROR_RC;
	}
	
	return PTP_OK;
}

int ptp_sony_wait_object(ptp_device *dev, uint32_t *object_handle, int timeout)
{
	ptp_params params;
	int retval;
	
	if (!dev || !object_handle)
	{
		return PTP_ERROR_PARAM;
	}
	
	*object_handle = 0;
	
	do
	{
		retval = ptp_wait_event(dev, &params, timeout);
		
		if (retval != PTP_OK)
		{
			return retval;
		}
	}
	while (params.code != PTP_EC_SONY_ObjectAdded);
	
	if (params.num_params < 1)
	{
		return PTP_ERROR_RESULT_PARAM;
	}
	
	*object_handle = params.params[0];
	
	return PTP_OK;
}

int ptp_sony_wait_property(ptp_device *dev, ptp_pima_prop_code *code, int timeout)
{
	ptp_params params;
	int retval;
	
	if (!dev || !code)
	{
		return PTP_ERROR_PARAM;
	}
	
	*code = 0;
	
	do
	{
		retval = ptp_wait_event(dev, &params, timeout);
		
		if (retval != PTP_OK)
		{
			return retval;
		}
	}
	while (params.code != PTP_EC_SONY_PropertyChanged);
	
	if (params.num_params < 1)
	{
		return PTP_ERROR_RESULT_PARAM;
	}
	
	*code = (ptp_pima_prop_code)(params.params[0] & 0xFFFF);
	
	return PTP_OK;
}

int ptp_sony_wait_pending_object(ptp_device *dev)
{
	#if PTP_SONY_READ_OPT == 1
	
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	uint16_t val;
	
	params_out.code = PTP_OP_SONY_GETALLDEVPROPDATA;
	params_out.num_params = 0;
	
	do
	{
		retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
		
		if (retval != PTP_OK)
		{
			return retval;
		}
		
		if (params_in.code != PTP_RC_OK)
		{
			free(data);
			return PTP_ERROR_RC;
		}
		
		val = *(uint16_t *)(((uint8_t *)data) + 0x451);
		
		free(data);
	}
	while (!(val & 0x8000));
	
	return PTP_OK;
	
	#else
	
	int pending;
	
	do
	{
		pending = ptp_sony_get_pending_objects(dev);
	}
	while (pending >= 0);
	
	return (pending < 0) ? pending : PTP_OK;
	
	#endif
}

int ptp_sony_get_pending_objects(ptp_device *dev)
{
	#if PTP_SONY_READ_OPT == 1
	
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	uint16_t val;
	
	params_out.code = PTP_OP_SONY_GETALLDEVPROPDATA;
	params_out.num_params = 0;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
	
	if (retval != PTP_OK)
	{
		return retval;
	}
	
	if (params_in.code != PTP_RC_OK)
	{
		free(data);
		return PTP_ERROR_RC;
	}
	
	val = *(uint16_t *)(((uint8_t *)data) + 0x451);
	val = val & 0x7FFF;
	free(data);
	
	return val;
	
	#else
	
	ptp_pima_prop_desc_list *list;
	int retval;
	
	retval = ptp_pima_proplist_create(&list);
	
	if (retval == PTP_OK)
	{
		retval = ptp_sony_get_all_dev_prop_data(dev, list);
		
		if (retval == PTP_OK)
		{
			ptp_pima_prop_desc *desc;
			
			desc = ptp_pima_proplist_get_prop(list, PTP_DPC_SONY_PendingImages);
			
			if (desc == NULL)
			{
				retval = PTP_ERROR_NOT_FOUND;
			}
			else if (desc->type != PTP_DTC_UINT16)
			{
				retval = PTP_ERROR_PROP_TYPE;
			}
			else if (desc->val.value == NULL)
			{
				retval = PTP_ERROR_PROP_VALUE;
			}
			else
			{
				retval = desc->val.value->u16;
			}
		}
		
		ptp_pima_proplist_free(list);
	}
	
	return retval;
	
	#endif
}

const char *ptp_sony_get_prop_name(ptp_pima_prop_code code)
{
	const char *name;
	
	name = ptp_pima_get_prop_name(code);
	
	if (name)
	{
		return name;
	}
	
	return ptp_pima_get_code_name(code, g_prop_names);
}

const char *ptp_sony_get_op_name(ptp_pima_op_code code)
{
	const char *name;
	
	name = ptp_pima_get_op_name(code);
	
	if (name)
	{
		return name;
	}
	
	return ptp_pima_get_code_name(code, g_op_names);
}

int ptp_sony_decode_prop_desc(ptp_pima_decode_context *ctx, ptp_pima_prop_desc *desc)
{
	dynbuf_adjust_context actx;
	uint8_t unk;
	
	if (!ctx || !desc)
	{
		return PTP_ERROR_PARAM;
	}
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	cr(ptp_pima_decode_int(ctx, &desc->code, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &desc->type, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &desc->get_set, sizeof(uint8_t)));
	cr(ptp_pima_decode_int(ctx, &unk, sizeof(uint8_t)));
	
	cr(ptp_pima_decode_prop_value(ctx, desc->type, &desc->def));
	dynbuf_adjust_update(&actx, ctx->buf, (void **)&desc);
	
	cr(ptp_pima_decode_prop_value(ctx, desc->type, &desc->val));
	dynbuf_adjust_update(&actx, ctx->buf, (void **)&desc);
	
	cr(ptp_pima_decode_prop_form(ctx, desc->type, &desc->form));
	
	return PTP_OK;
}

int ptp_sony_decode_prop_desc_list(ptp_pima_decode_context *ctx, ptp_pima_prop_desc_list *list)
{
	dynbuf_adjust_context actx;
	uint32_t count, unk, i;
	ptp_pima_prop_desc *desc;
	
	if (!ctx || !list)
	{
		return PTP_ERROR_PARAM;
	}
	
	cr(ptp_pima_decode_int(ctx, &count, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &unk, sizeof(uint32_t)));
	
	desc = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_prop_desc) * (size_t)count);
	
	if (!desc)
	{
		return PTP_ERROR_MEMORY;
	}
	
	list->desc = desc;
	list->count = count;
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	for (i = 0; i < count; i++)
	{
		cr(ptp_sony_decode_prop_desc(ctx, &desc[i]));
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&desc);
	}
	
	return PTP_OK;
}

int ptp_sony_decode_device_info(ptp_pima_decode_context *ctx, ptp_pima_device_info *info)
{
	if (!ctx || !info)
	{
		return PTP_ERROR_PARAM;
	}
	
	cr(ptp_pima_decode_int(ctx, &info->version, sizeof(uint16_t)));
	
	// Properties
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->properties.count, (void **)&info->properties.values));
	
	// Control values
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->properties.count, (void **)&info->properties.values));
	
	return PTP_OK;
}

void ptp_sony_print_prop_desc(const ptp_pima_prop_desc *desc)
{
	const char *name, *type;
	
	if (!desc)
	{
		return;
	}
	
	name = ptp_sony_get_prop_name(desc->code);
	type = ptp_pima_get_type_name(desc->type);
	
	printf("<%04Xh> %-30s [%-7s] ", desc->code, name ? name : "?", type);
	ptp_pima_print_prop_value(desc->type, &desc->val);
	printf("\n");
}

void ptp_sony_print_device_info(const ptp_pima_device_info *info)
{
	int i;
	const char *name;
	
	if (!info)
	{
		return;
	}
	
	printf("Protocol version: %u.%02u\n", info->version / 100, info->version % 100);
	printf("Vendor extension ID: %u\n", info->vendor_extension_id);
	printf("Vendor extension version: %u.%02u\n", info->vendor_extension_version / 100, info->vendor_extension_version % 100);
	printf("Vendor extension description: %S\n", info->vendor_extension_desc);
	
	printf("\nSupported operations:\n");
	for (i = 0; i < info->operations.count; i++)
	{
		name = ptp_sony_get_op_name(info->operations.values[i]);
		printf("<%04Xh> %s\n", info->operations.values[i], name ? name : "?");
	}
	
	if (i == 0) printf("(None)\n");
	
	printf("\nSupported events:\n");
	for (i = 0; i < info->events.count; i++)
	{
		printf("<%04Xh>\n", info->events.values[i]);
	}
	
	if (i == 0) printf("(None)\n");
	
	printf("\nSupported properties:\n");
	for (i = 0; i < info->properties.count; i++)
	{
		name = ptp_sony_get_prop_name(info->properties.values[i]);
		printf("<%04Xh> %s\n", info->properties.values[i], name ? name : "?");
	}
	
	if (i == 0) printf("(None)\n");
	
	printf("\nSupported capture formats:\n");
	for (i = 0; i < info->capture_formats.count; i++)
	{
		printf("<%04Xh>\n", info->capture_formats.values[i]);
	}
	
	if (i == 0) printf("(None)\n");
	
	printf("\nSupported image formats:\n");
	for (i = 0; i < info->image_formats.count; i++)
	{
		printf("<%04Xh>\n", info->image_formats.values[i]);
	}
	
	if (i == 0) printf("(None)\n");
	
	printf("\n");
	printf("Manufacturer: %S\n", info->manufacturer);
	printf("Model: %S\n", info->model);
	printf("Device version: %S\n", info->device_version);
	printf("Serial number: %S\n", info->serial_number);
}

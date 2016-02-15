#include "ptp-pima.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

static const ptp_pima_code_name g_prop_names[] = {
	{ PTP_DPC_Undefined,				"Undefined" },
	{ PTP_DPC_BatteryLevel,				"Battery level" },
	{ PTP_DPC_FunctionalMode,			"Functional mode" },
	{ PTP_DPC_ImageSize,				"Image size" },
	{ PTP_DPC_CompressionSetting,		"Compression setting" },
	{ PTP_DPC_WhiteBalance,				"White balance" },
	{ PTP_DPC_RGBGain,					"RGB Gain" },
	{ PTP_DPC_FNumber,					"F Number" },
	{ PTP_DPC_FocalLength,				"Focal length" },
	{ PTP_DPC_FocusDistance,			"Focal distance" },
	{ PTP_DPC_FocusMode,				"Focus mode" },
	{ PTP_DPC_ExposureMeteringMode,		"Exposure metering mode" },
	{ PTP_DPC_FlashMode,				"Flash mode" },
	{ PTP_DPC_ExposureTime,				"Exposure time" },
	{ PTP_DPC_ExposureProgramMode,		"Exposure program mode" },
	{ PTP_DPC_ExposureIndex,			"Exposure index" },
	{ PTP_DPC_ExposureBiasCompensation,	"Exposure bias compensation" },
	{ PTP_DPC_DateTime,					"Date/Time" },
	{ PTP_DPC_CaptureDelay,				"Capture delay" },
	{ PTP_DPC_StillCaptureMode,			"Still capture mode" },
	{ PTP_DPC_Contrast,					"Contrast" },
	{ PTP_DPC_Sharpness,				"Sharpness" },
	{ PTP_DPC_DigitalZoom,				"Digital zoom" },
	{ PTP_DPC_EffectMode,				"Effect mode" },
	{ PTP_DPC_BurstNumber,				"Burst number" },
	{ PTP_DPC_BurstInterval,			"Burst interval" },
	{ PTP_DPC_TimelapseNumber,			"Timelapse number" },
	{ PTP_DPC_TimelapseInterval,		"Timelapse interval" },
	{ PTP_DPC_FocusMeteringMode,		"Focus metering mode" },
	{ PTP_DPC_UploadURL,				"Upload URL" },
	{ PTP_DPC_Artist,					"Artist" },
	{ PTP_DPC_CopyrightInfo,			"Copyright info" },
	{ 0x0000,							NULL }
};

static const ptp_pima_code_name g_op_names[] = {
	{ PTP_OP_PIMA_GetDeviceInfo,		"GetDeviceInfo" },
	{ PTP_OP_PIMA_OpenSession,			"OpenSession" },
	{ PTP_OP_PIMA_CloseSession,			"CloseSession" },
	{ PTP_OP_PIMA_GetStorageIDs,		"GetStorageIDs" },
	{ PTP_OP_PIMA_GetStorageInfo,		"GetStorageInfo" },
	{ PTP_OP_PIMA_GetNumObjects,		"GetNumObjects" },
	{ PTP_OP_PIMA_GetObjectHandles,		"GetObjectHandles" },
	{ PTP_OP_PIMA_GetObjectInfo,		"GetObjectInfo" },
	{ PTP_OP_PIMA_GetObject,			"GetObject" },
	{ PTP_OP_PIMA_GetThumb,				"GetThumb" },
	{ PTP_OP_PIMA_DeleteObject,			"DeleteObject" },
	{ PTP_OP_PIMA_SendObjectInfo,		"SendObjectInfo" },
	{ PTP_OP_PIMA_SendObject,			"SendObject" },
	{ PTP_OP_PIMA_InitiateCapture,		"InitiateCapture" },
	{ PTP_OP_PIMA_FormatStore,			"FormatStore" },
	{ PTP_OP_PIMA_ResetDevice,			"ResetDevice" },
	{ PTP_OP_PIMA_SelfTest,				"SelfTest" },
	{ PTP_OP_PIMA_SetObjectProtection,	"SetObjectProtection" },
	{ PTP_OP_PIMA_PowerDown,			"PowerDown" },
	{ PTP_OP_PIMA_GetDevicePropDesc,	"GetDevicePropDesc" },
	{ PTP_OP_PIMA_GetDevicePropValue,	"GetDevicePropValue" },
	{ PTP_OP_PIMA_SetDevicePropValue,	"SetDevicePropValue" },
	{ PTP_OP_PIMA_ResetDevicePropValue,	"ResetDevicePropValue" },
	{ PTP_OP_PIMA_TerminateOpenCapture,	"TerminateOpenCapture" },
	{ PTP_OP_PIMA_MoveObject,			"MoveObject" },
	{ PTP_OP_PIMA_CopyObject,			"CopyObject" },
	{ PTP_OP_PIMA_GetPartialObject,		"GetPartialObject" },
	{ PTP_OP_PIMA_InitiateOpenCapture,	"InitiateOpenCapture" },
	{ 0x0000,							NULL }
};

#define cr(x)		do { int _cr_ret = x; if (_cr_ret != PTP_OK) return _cr_ret; } while (0)
#define adjust_ptr_offset(v,o)	v=(void *)(((uint8_t *)(v)) + o)


int ptp_pima_open_session(ptp_device *dev, uint32_t session_id)
{
	ptp_params params_out, params_in;
	int retval;
	
	params_out.code = PTP_OP_PIMA_OpenSession;
	params_out.num_params = 1;
	params_out.params[0] = session_id;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, NULL, NULL);
	
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

int ptp_pima_close_session(ptp_device *dev)
{
	ptp_params params_out, params_in;
	int retval;
	
	params_out.code = PTP_OP_PIMA_CloseSession;
	params_out.num_params = 0;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, NULL, NULL);
	
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

int ptp_pima_get_device_info(ptp_device *dev, ptp_pima_device_info *info)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	params_out.code = PTP_OP_PIMA_GetDeviceInfo;
	params_out.num_params = 0;
	
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
		
		retval = ptp_pima_decode_device_info(&ctx, info);
		
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

int ptp_pima_get_object_info(ptp_device *dev, uint32_t object_handle, ptp_pima_object_info *info)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	params_out.code = PTP_OP_PIMA_GetObjectInfo;
	params_out.num_params = 1;
	params_out.params[0] = object_handle;
	
	retval = ptp_transact(dev, &params_out, NULL, 0, &params_in, &data, &data_size);
	
	if (retval != PTP_OK)
	{
		printf("ptp_transact: %d\n", retval);
		return retval;
	}
	
	if (info)
	{
		ptp_pima_decode_context ctx;
		
		ctx.ptr = data;
		ctx.size = data_size;
		ctx.buf = info->buf;
		
		retval = ptp_pima_decode_object_info(&ctx, info);
		
		if (retval != PTP_OK)
		{
			free(data);
			return retval;
		}
	}
	
	free(data);
	
	if (params_in.code != PTP_RC_OK)
	{
		printf("PTP result code: %04X\n", params_in.code);
		return PTP_ERROR_RC;
	}
	
	return PTP_OK;
}

int ptp_pima_get_object(ptp_device *dev, uint32_t object_handle, void **object_data)
{
	ptp_params params_out, params_in;
	void *data;
	int retval;
	uint32_t data_size;
	
	if (!object_data)
	{
		return PTP_ERROR_PARAM;
	}
	
	*object_data = NULL;
	
	params_out.code = PTP_OP_PIMA_GetObject;
	params_out.num_params = 1;
	params_out.params[0] = object_handle;
	
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
	
	*object_data = data;
	
	return data_size;
}

int ptp_pima_decode_int(ptp_pima_decode_context *ctx, void *p, size_t size)
{
	if (size > sizeof(uint128_t))
	{
		return PTP_ERROR_PARAM;
	}
	
	if (ctx->size < size)
	{
		return PTP_ERROR_DATA_LEN;
	}
	
	memcpy(p, ctx->ptr, size);
	
	// TODO: Flip on big endian systems
	
	ctx->ptr = (void *)(((uint8_t *)ctx->ptr) + size);
	ctx->size -= size;
	
	return PTP_OK;
}

int ptp_pima_decode_string(ptp_pima_decode_context *ctx, wchar_t **str)
{
	uint8_t i, count;
	
	if (!ctx || !str)
	{
		return PTP_ERROR_PARAM;
	}
	
	cr(ptp_pima_decode_int(ctx, &count, sizeof(count)));
	
	// Allocate memory for the string with a null terminator and initialize to zero
	*str = dynbuf_append(ctx->buf, NULL, sizeof(wchar_t) * (size_t)(count + 1));
	
	if (!(*str))
	{
		return PTP_ERROR_MEMORY;
	}
	
	// Append the string
	for (i = 0; i < count; i++)
	{
		cr(ptp_pima_decode_int(ctx, (*str) + i, sizeof(uint16_t)));
	}
	
	return PTP_OK;
}

int ptp_pima_decode_int_array(ptp_pima_decode_context *ctx, size_t elem_size, int *count, void **p)
{
	dynbuf_adjust_context actx;
	uint32_t i, c;
	size_t prev_count;
	void *prev_ptr;
	
	if (!ctx || !count || !p)
	{
		return PTP_ERROR_PARAM;
	}
	
	prev_ptr = *p;
	prev_count = prev_ptr ? (size_t)(*count) : 0;
	
	*count = 0;
	*p = NULL;
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	cr(ptp_pima_decode_int(ctx, &c, sizeof(c)));
	
	// Allocate memory for the string with a null terminator and initialize to zero
	*p = dynbuf_append(ctx->buf, NULL, elem_size * ((size_t)c + prev_count));
	
	if (!(*p))
	{
		return PTP_ERROR_MEMORY;
	}
	
	if (prev_ptr)
	{
		dynbuf_adjust_update(&actx, ctx->buf, &prev_ptr);
		memmove(*p, prev_ptr, prev_count * elem_size);
	}
	
	// Append the string
	for (i = 0; i < c; i++)
	{
		cr(ptp_pima_decode_int(ctx, ((uint8_t *)(*p)) + elem_size * (prev_count + i), elem_size));
	}
	
	*count = (int)(c + prev_count);
	
	return PTP_OK;
}

int ptp_pima_decode_basic_value(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_basic_value *value)
{
	size_t size;
	
	if (!ctx || !value)
	{
		return PTP_ERROR_PARAM;
	}
	
	switch (type)
	{
	case PTP_DTC_UINT8:
	case PTP_DTC_INT8:
		size = sizeof(uint8_t);
		break;
		
	case PTP_DTC_UINT16:
	case PTP_DTC_INT16:
		size = sizeof(uint16_t);
		break;
		
	case PTP_DTC_UINT32:
	case PTP_DTC_INT32:
		size = sizeof(uint32_t);
		break;
		
	case PTP_DTC_UINT64:
	case PTP_DTC_INT64:
		size = sizeof(uint64_t);
		break;
		
	case PTP_DTC_UINT128:
	case PTP_DTC_INT128:
		size = sizeof(uint128_t);
		break;
		
	default:
		return PTP_ERROR_PARAM;
	}
	
	return ptp_pima_decode_int(ctx, value, size);
}

int ptp_pima_decode_prop_value(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_prop_value *value)
{
	dynbuf_adjust_context actx;
	void *vptr;
	
	if (!ctx || !value)
	{
		return PTP_ERROR_PARAM;
	}
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	if (type == PTP_DTC_STR)
	{
		uint8_t i, count;
		
		cr(ptp_pima_decode_int(ctx, &count, sizeof(count)));
		
		// Allocate memory for the string with a null terminator and initialize to zero
		vptr = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_basic_value) * (size_t)(count + 1));
		
		if (!vptr)
		{
			return PTP_ERROR_MEMORY;
		}
		
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&value);
		
		value->value = vptr;
		value->count = count;
		
		// Append the string
		for (i = 0; i < count; i++)
		{
			cr(ptp_pima_decode_basic_value(ctx, PTP_DTC_UINT16, (ptp_pima_basic_value *)&value->value[i]));
		}
	}
	else if (type & PTP_DTC_ARRAY_MASK)
	{
		uint32_t i, count;
		
		cr(ptp_pima_decode_int(ctx, &count, sizeof(count)));
		
		// Allocate memory for the array values
		vptr = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_basic_value) * (size_t)count);
		
		if (!vptr)
		{
			return PTP_ERROR_MEMORY;
		}
		
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&value);
		
		value->value = vptr;
		value->count = count;
		
		// Append the values
		for (i = 0; i < count; i++)
		{
			cr(ptp_pima_decode_basic_value(ctx, type & ~PTP_DTC_ARRAY_MASK, &value->value[i]));
		}
	}
	else
	{
		// Allocate memory for the value
		vptr = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_basic_value));
		
		if (!vptr)
		{
			return PTP_ERROR_MEMORY;
		}
		
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&value);
		
		value->value = vptr;
		value->count = 1;
		
		return ptp_pima_decode_basic_value(ctx, type, value->value);
	}
	
	return PTP_OK;
}

int ptp_pima_decode_prop_form(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_prop_form **form)
{
	uint8_t flag;
	dynbuf_adjust_context actx;
	ptp_pima_prop_form *f;
	
	if (!ctx || !form)
	{
		return PTP_ERROR_PARAM;
	}
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	*form = NULL;
	
	cr(ptp_pima_decode_int(ctx, &flag, sizeof(uint8_t)));
	
	if (flag == PTP_FORM_NONE)
	{
		return PTP_OK;
	}
	
	f = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_prop_form));
	
	if (f == NULL)
	{
		return PTP_ERROR_MEMORY;
	}
	
	dynbuf_adjust_update(&actx, ctx->buf, (void **)&form);
	*form = f;
	(*form)->type = flag;
	
	if (flag == PTP_FORM_RANGE)
	{
		cr(ptp_pima_decode_prop_value(ctx, type, &(*form)->range.min));
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&form);
		
		cr(ptp_pima_decode_prop_value(ctx, type, &(*form)->range.max));
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&form);
		
		cr(ptp_pima_decode_prop_value(ctx, type, &(*form)->range.step));
	}
	else if (flag == PTP_FORM_ENUM)
	{
		uint16_t i, count;
		void *values;
		
		cr(ptp_pima_decode_int(ctx, &count, sizeof(count)));
		
		values = dynbuf_append(ctx->buf, NULL, sizeof(ptp_pima_prop_value) * (size_t)count);
		
		dynbuf_adjust_update(&actx, ctx->buf, (void **)&form);
		
		(*form)->penum.values = values;
		(*form)->penum.count = count;
		
		if (!((*form)->penum.values))
		{
			return PTP_ERROR_MEMORY;
		}
		
		for (i = 0; i < count; i++)
		{
			cr(ptp_pima_decode_prop_value(ctx, type, &(*form)->penum.values[i]));
			dynbuf_adjust_update(&actx, ctx->buf, (void **)&form);
		}
	}
	else
	{
		return PTP_ERROR_PARAM;
	}
	
	return PTP_OK;
}

int ptp_pima_decode_prop_desc(ptp_pima_decode_context *ctx, ptp_pima_prop_desc *desc)
{
	dynbuf_adjust_context actx;
	
	if (!ctx || !desc)
	{
		return PTP_ERROR_PARAM;
	}
	
	dynbuf_adjust_begin(&actx, ctx->buf);
	
	cr(ptp_pima_decode_int(ctx, &desc->code, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &desc->type, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &desc->get_set, sizeof(uint8_t)));
	
	cr(ptp_pima_decode_prop_value(ctx, desc->type, &desc->def));
	dynbuf_adjust_update(&actx, ctx->buf, (void **)&desc);
	
	cr(ptp_pima_decode_prop_value(ctx, desc->type, &desc->val));
	dynbuf_adjust_update(&actx, ctx->buf, (void **)&desc);
	
	cr(ptp_pima_decode_prop_form(ctx, desc->type, &desc->form));
	
	return PTP_OK;
}

int ptp_pima_decode_device_info(ptp_pima_decode_context *ctx, ptp_pima_device_info *info)
{
	if (!ctx || !info)
	{
		return PTP_ERROR_PARAM;
	}
	
	cr(ptp_pima_decode_int(ctx, &info->version, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &info->vendor_extension_id, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->vendor_extension_version, sizeof(uint16_t)));
	cr(ptp_pima_decode_string(ctx, &info->vendor_extension_desc));
	cr(ptp_pima_decode_int(ctx, &info->functional_mode, sizeof(uint16_t)));
	
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->operations.count, (void **)&info->operations.values));
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->events.count, (void **)&info->events.values));
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->properties.count, (void **)&info->properties.values));
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->capture_formats.count, (void **)&info->capture_formats.values));
	cr(ptp_pima_decode_int_array(ctx, sizeof(uint16_t), &info->image_formats.count, (void **)&info->image_formats.values));
	
	cr(ptp_pima_decode_string(ctx, &info->manufacturer));
	cr(ptp_pima_decode_string(ctx, &info->model));
	cr(ptp_pima_decode_string(ctx, &info->device_version));
	cr(ptp_pima_decode_string(ctx, &info->serial_number));
	
	return PTP_OK;
}

int ptp_pima_decode_object_info(ptp_pima_decode_context *ctx, ptp_pima_object_info *info)
{
	if (!ctx || !info)
	{
		return PTP_ERROR_PARAM;
	}
	
	cr(ptp_pima_decode_int(ctx, &info->storage_id, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->object_format, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &info->protection_status, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &info->object_compressed_size, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->thumb_format, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &info->thumb_compressed_size, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->thumb_pix_width, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->thumb_pix_height, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->image_pix_width, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->image_pix_height, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->image_bit_depth, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->parent_object, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->assoc_type, sizeof(uint16_t)));
	cr(ptp_pima_decode_int(ctx, &info->assoc_desc, sizeof(uint32_t)));
	cr(ptp_pima_decode_int(ctx, &info->seq_number, sizeof(uint32_t)));
	
	cr(ptp_pima_decode_string(ctx, &info->filename));
	cr(ptp_pima_decode_string(ctx, &info->capture_date));
	cr(ptp_pima_decode_string(ctx, &info->modification_date));
	cr(ptp_pima_decode_string(ctx, &info->keywords));
	
	return PTP_OK;
}

void ptp_pima_adjust_prop_value(dynbuf *buf, ssize_t offset, ptp_pima_prop_value *value)
{
	if (value->value)
	{
		adjust_ptr_offset(value->value, offset);
	}
}

void ptp_pima_adjust_prop_form(dynbuf *buf, ssize_t offset, ptp_pima_prop_form *form)
{
	if (form->type == PTP_FORM_RANGE)
	{
		ptp_pima_adjust_prop_value(buf, offset, &form->range.min);
		ptp_pima_adjust_prop_value(buf, offset, &form->range.max);
		ptp_pima_adjust_prop_value(buf, offset, &form->range.step);
	}
	else if (form->type == PTP_FORM_ENUM)
	{
		if (form->penum.values)
		{
			int i;
			
			adjust_ptr_offset(form->penum.values, offset);
			
			for (i = 0; i < form->penum.count; i++)
			{
				ptp_pima_adjust_prop_value(buf, offset, &form->penum.values[i]);
			}
		}
	}
}

void ptp_pima_adjust_prop_desc(dynbuf *buf, ssize_t offset, ptp_pima_prop_desc *desc)
{
	ptp_pima_adjust_prop_value(buf, offset, &desc->def);
	ptp_pima_adjust_prop_value(buf, offset, &desc->val);
	
	if (desc->form)
	{
		adjust_ptr_offset(desc->form, offset);
		
		ptp_pima_adjust_prop_form(buf, offset, desc->form);
	}
}

void ptp_pima_adjust_prop_desc_list(dynbuf *buf, ssize_t offset, void *context)
{
	ptp_pima_prop_desc_list *list = context;
	
	if (list && list->desc)
	{
		int i;
		
		adjust_ptr_offset(list->desc, offset);
		
		for (i = 0; i < list->count; i++)
		{
			ptp_pima_adjust_prop_desc(buf, offset, &list->desc[i]);
		}
	}
}

void ptp_pima_adjust_device_info(dynbuf *buf, ssize_t offset, void *context)
{
	ptp_pima_device_info *info = context;
	
	if (!info)
	{
		return;
	}
	
	if (info->vendor_extension_desc) adjust_ptr_offset(info->vendor_extension_desc, offset);
	
	if (info->operations.values) adjust_ptr_offset(info->operations.values, offset);
	if (info->events.values) adjust_ptr_offset(info->events.values, offset);
	if (info->properties.values) adjust_ptr_offset(info->properties.values, offset);
	if (info->capture_formats.values) adjust_ptr_offset(info->capture_formats.values, offset);
	if (info->image_formats.values) adjust_ptr_offset(info->image_formats.values, offset);
	
	if (info->manufacturer) adjust_ptr_offset(info->manufacturer, offset);
	if (info->model) adjust_ptr_offset(info->model, offset);
	if (info->device_version) adjust_ptr_offset(info->device_version, offset);
	if (info->serial_number) adjust_ptr_offset(info->serial_number, offset);
}

void ptp_pima_adjust_object_info(dynbuf *buf, ssize_t offset, void *context)
{
	ptp_pima_object_info *info = context;
	
	if (!info)
	{
		return;
	}
	
	if (info->filename) adjust_ptr_offset(info->filename, offset);
	if (info->capture_date) adjust_ptr_offset(info->capture_date, offset);
	if (info->modification_date) adjust_ptr_offset(info->modification_date, offset);
	if (info->keywords) adjust_ptr_offset(info->keywords, offset);
}

int ptp_pima_devinfo_create(ptp_pima_device_info **info)
{
	ptp_pima_device_info *inf;
	
	if (!info)
	{
		return PTP_ERROR_PARAM;
	}
	
	inf = calloc(sizeof(ptp_pima_device_info), 1);
	
	if (!inf)
	{
		return PTP_ERROR_MEMORY;
	}
	
	inf->buf = dynbuf_create(4, ptp_pima_adjust_device_info, inf);
	
	if (!(inf->buf))
	{
		free(inf);
		return PTP_ERROR_MEMORY;
	}
	
	*info = inf;
	
	return PTP_OK;
}

void ptp_pima_devinfo_free(ptp_pima_device_info *info)
{
	if (info)
	{
		dynbuf_free(info->buf);
		free(info);
	}
}

int ptp_pima_objinfo_create(ptp_pima_object_info **info)
{
	ptp_pima_object_info *inf;
	
	if (!info)
	{
		return PTP_ERROR_PARAM;
	}
	
	inf = calloc(sizeof(ptp_pima_object_info), 1);
	
	if (!inf)
	{
		return PTP_ERROR_MEMORY;
	}
	
	inf->buf = dynbuf_create(4, ptp_pima_adjust_object_info, inf);
	
	if (!(inf->buf))
	{
		free(inf);
		return PTP_ERROR_MEMORY;
	}
	
	*info = inf;
	
	return PTP_OK;
}

void ptp_pima_objinfo_free(ptp_pima_object_info *info)
{
	if (info)
	{
		dynbuf_free(info->buf);
		free(info);
	}
}

int ptp_pima_proplist_create(ptp_pima_prop_desc_list **list)
{
	ptp_pima_prop_desc_list *l;
	
	if (!list)
	{
		return PTP_ERROR_PARAM;
	}
	
	l = calloc(sizeof(ptp_pima_prop_desc_list), 1);
	
	if (!l)
	{
		return PTP_ERROR_MEMORY;
	}
	
	l->buf = dynbuf_create(4, ptp_pima_adjust_prop_desc_list, l);
	
	if (!(l->buf))
	{
		free(l);
		return PTP_ERROR_MEMORY;
	}
	
	*list = l;
	
	return PTP_OK;
}

void ptp_pima_proplist_free(ptp_pima_prop_desc_list *list)
{
	if (list)
	{
		dynbuf_free(list->buf);
		free(list);
	}
}

void ptp_pima_proplist_clear(ptp_pima_prop_desc_list *list)
{
	if (list)
	{
		list->count = 0;
		list->desc = NULL;
		dynbuf_clear(list->buf);
	}
}

ptp_pima_prop_desc * ptp_pima_proplist_get_prop(ptp_pima_prop_desc_list *list, ptp_pima_prop_code code)
{
	int i;
	
	if (!list || !list->desc)
	{
		return NULL;
	}
	
	for (i = 0; i < list->count; i++)
	{
		if (list->desc[i].code == code)
		{
			return &list->desc[i];
		}
	}
	
	return NULL;
}

const char *ptp_pima_get_code_name(uint16_t code, const ptp_pima_code_name *names)
{
	while (names->code != 0x0000)
	{
		if (names->code == code)
		{
			return names->name;
		}
		
		names++;
	}
	
	return NULL;
}

const char *ptp_pima_get_prop_name(ptp_pima_prop_code code)
{
	return ptp_pima_get_code_name(code, g_prop_names);
}

const char *ptp_pima_get_op_name(ptp_pima_op_code code)
{
	return ptp_pima_get_code_name(code, g_op_names);
}

const char *ptp_pima_get_type_name(ptp_pima_type_code type)
{
	if (type == PTP_DTC_STR)
	{
		return "STRING";
	}
	
	type &= ~PTP_DTC_ARRAY_MASK;
	
	switch (type)
	{
	case PTP_DTC_UINT8:
		return "UINT8";
		
	case PTP_DTC_INT8:
		return "INT8";
		
	case PTP_DTC_UINT16:
		return "UINT16";
		
	case PTP_DTC_INT16:
		return "INT16";
		
	case PTP_DTC_UINT32:
		return "UINT32";
		
	case PTP_DTC_INT32:
		return "INT32";
		
	case PTP_DTC_UINT64:
		return "UINT64";
		
	case PTP_DTC_INT64:
		return "INT64";
		
	case PTP_DTC_UINT128:
		return "UINT128";
		
	case PTP_DTC_INT128:
		return "INT128";
		
	default:
		return "UNKNOWN";
	}
}

void ptp_pima_print_prop_value(ptp_pima_type_code type, const ptp_pima_prop_value *value)
{
	if (!value)
	{
		return;
	}
	
	if (type == PTP_DTC_STR)
	{
		int i;
		
		printf("\"");
		
		for (i = 0; i < value->count; i++)
		{
			wprintf(L"%s", &value->value[i].str);
		}
		
		printf("\"");
		
		return;
	}
	
	type &= ~PTP_DTC_ARRAY_MASK;
	
	switch (type)
	{
	case PTP_DTC_UINT8:
		printf("%" PRIu8 " (%02" PRIX8 "h)", value->value->u8, value->value->u8);
		break;
		
	case PTP_DTC_INT8:
		printf("%" PRId8 " (%02" PRIX8 "h)", value->value->s8, value->value->s8);
		break;
		
	case PTP_DTC_UINT16:
		printf("%" PRIu16 " (%04" PRIX16 "h)", value->value->u16, value->value->u16);
		break;
		
	case PTP_DTC_INT16:
		printf("%" PRId16 " (%04" PRIX16 "h)", value->value->s16, value->value->s16);
		break;
		
	case PTP_DTC_UINT32:
		printf("%" PRIu32 " (%08" PRIX32 "h)", value->value->u32, value->value->u32);
		break;
		
	case PTP_DTC_INT32:
		printf("%" PRId32 " (%08" PRIX32 "h)", value->value->s32, value->value->s32);
		break;
		
	case PTP_DTC_UINT64:
		printf("%" PRIu64 " (%016" PRIX64 "h)", value->value->u64, value->value->u64);
		break;
		
	case PTP_DTC_INT64:
		printf("%" PRId64 " (%016" PRIX64 "h)", value->value->s64, value->value->s64);
		break;
		
	case PTP_DTC_UINT128:
		printf("L=%" PRIu64 " (%016" PRIX64 "h) H=%" PRIu64 " (%016" PRIX64 "h)", 
			value->value->u128.low, 
			value->value->u128.low,
			value->value->u128.high,
			value->value->u128.high);
		break;
		
	case PTP_DTC_INT128:
		printf("L=%" PRId64 " (%016" PRIX64 "h) H=%" PRId64 " (%016" PRIX64 "h)", 
			value->value->s128.low, 
			value->value->s128.low,
			value->value->s128.high,
			value->value->s128.high);
		break;
	}
}

void ptp_pima_print_object_info(const ptp_pima_object_info *info)
{
	if (!info)
	{
		return;
	}
	
	printf("Storage ID: %08Xh\n", info->storage_id);
	printf("Object format: %04Xh\n", info->object_format);
	printf("Protection status: %04Xh\n", info->protection_status);
	printf("Object compressed size: %u\n", info->object_compressed_size);
	printf("\n");
	printf("Thumb format: %04Xh\n", info->thumb_format);
	printf("Thumb compressed size: %u\n", info->thumb_compressed_size);
	printf("Thumb pixel width: %u\n", info->thumb_pix_width);
	printf("Thumb pixel height: %u\n", info->thumb_pix_height);
	printf("Image pixel width: %u\n", info->image_pix_width);
	printf("Image pixel height: %u\n", info->image_pix_height);
	printf("Image bit depth: %u\n", info->image_bit_depth);
	printf("\n");
	printf("Parent object: %08Xh\n", info->parent_object);
	printf("Association type: %04Xh\n", info->assoc_type);
	printf("Association desc: %08Xh\n", info->assoc_desc);
	printf("Sequence number: %u\n", info->seq_number);
	printf("\n");
	printf("File name: %S\n", info->filename ? info->filename : L"<None>");
	printf("Capture date: %S\n", info->capture_date ? info->capture_date : L"<None>");
	printf("Modification date: %S\n", info->modification_date ? info->modification_date : L"<None>");
	printf("Keywords: %S\n", info->keywords ? info->keywords : L"<None>");
}

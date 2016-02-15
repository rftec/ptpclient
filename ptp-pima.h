#ifndef __PTP_PIMA_H__
#define __PTP_PIMA_H__

#include "ptp.h"
#include "dynbuf.h"
#include <wchar.h>

#pragma pack(push, 1)

typedef struct _int128
{
	int64_t low;
	int64_t high;
} int128_t;

typedef struct _uint128
{
	uint64_t low;
	uint64_t high;
} uint128_t;

#pragma pack(pop)

typedef uint16_t	ptp_pima_prop_code;
typedef uint16_t	ptp_pima_type_code;
typedef uint16_t	ptp_pima_op_code;

#define PTP_RC_OK					0x2001

#define PTP_OP_PIMA_GetDeviceInfo		0x1001
#define PTP_OP_PIMA_OpenSession			0x1002
#define PTP_OP_PIMA_CloseSession		0x1003
#define PTP_OP_PIMA_GetStorageIDs		0x1004
#define PTP_OP_PIMA_GetStorageInfo		0x1005
#define PTP_OP_PIMA_GetNumObjects		0x1006
#define PTP_OP_PIMA_GetObjectHandles	0x1007
#define PTP_OP_PIMA_GetObjectInfo		0x1008
#define PTP_OP_PIMA_GetObject			0x1009
#define PTP_OP_PIMA_GetThumb			0x100A
#define PTP_OP_PIMA_DeleteObject		0x100B
#define PTP_OP_PIMA_SendObjectInfo		0x100C
#define PTP_OP_PIMA_SendObject			0x100D
#define PTP_OP_PIMA_InitiateCapture		0x100E
#define PTP_OP_PIMA_FormatStore			0x100F
#define PTP_OP_PIMA_ResetDevice			0x1010
#define PTP_OP_PIMA_SelfTest			0x1011
#define PTP_OP_PIMA_SetObjectProtection	0x1012
#define PTP_OP_PIMA_PowerDown			0x1013
#define PTP_OP_PIMA_GetDevicePropDesc	0x1014
#define PTP_OP_PIMA_GetDevicePropValue	0x1015
#define PTP_OP_PIMA_SetDevicePropValue	0x1016
#define PTP_OP_PIMA_ResetDevicePropValue	0x1017
#define PTP_OP_PIMA_TerminateOpenCapture	0x1018
#define PTP_OP_PIMA_MoveObject			0x1019
#define PTP_OP_PIMA_CopyObject			0x101A
#define PTP_OP_PIMA_GetPartialObject	0x101B
#define PTP_OP_PIMA_InitiateOpenCapture	0x101C

#define PTP_DPC_Undefined			0x5000
#define PTP_DPC_BatteryLevel		0x5001
#define PTP_DPC_FunctionalMode		0x5002
#define PTP_DPC_ImageSize			0x5003
#define PTP_DPC_CompressionSetting	0x5004
#define PTP_DPC_WhiteBalance		0x5005
#define PTP_DPC_RGBGain				0x5006
#define PTP_DPC_FNumber				0x5007
#define PTP_DPC_FocalLength			0x5008
#define PTP_DPC_FocusDistance		0x5009
#define PTP_DPC_FocusMode			0x500A
#define PTP_DPC_ExposureMeteringMode	0x500B
#define PTP_DPC_FlashMode			0x500C
#define PTP_DPC_ExposureTime		0x500D
#define PTP_DPC_ExposureProgramMode	0x500E
#define PTP_DPC_ExposureIndex		0x500F
#define PTP_DPC_ExposureBiasCompensation	0x5010
#define PTP_DPC_DateTime			0x5011
#define PTP_DPC_CaptureDelay		0x5012
#define PTP_DPC_StillCaptureMode	0x5013
#define PTP_DPC_Contrast			0x5014
#define PTP_DPC_Sharpness			0x5015
#define PTP_DPC_DigitalZoom			0x5016
#define PTP_DPC_EffectMode			0x5017
#define PTP_DPC_BurstNumber			0x5018
#define PTP_DPC_BurstInterval		0x5019
#define PTP_DPC_TimelapseNumber		0x501A
#define PTP_DPC_TimelapseInterval	0x501B
#define PTP_DPC_FocusMeteringMode	0x501C
#define PTP_DPC_UploadURL			0x501D
#define PTP_DPC_Artist				0x501E
#define PTP_DPC_CopyrightInfo		0x501F

#define PTP_DTC_UNDEF		0x0000
#define PTP_DTC_INT8		0x0001
#define PTP_DTC_UINT8		0x0002
#define PTP_DTC_INT16		0x0003
#define PTP_DTC_UINT16		0x0004
#define PTP_DTC_INT32		0x0005
#define PTP_DTC_UINT32		0x0006
#define PTP_DTC_INT64		0x0007
#define PTP_DTC_UINT64		0x0008
#define PTP_DTC_INT128		0x0009
#define PTP_DTC_UINT128		0x000A

#define PTP_DTC_ARRAY_MASK	0x4000

#define PTP_DTC_AINT8		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT8)
#define PTP_DTC_AUINT8		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT8)
#define PTP_DTC_AINT16		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT16)
#define PTP_DTC_AUINT16		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT16)
#define PTP_DTC_AINT32		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT32)
#define PTP_DTC_AUINT32		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT32)
#define PTP_DTC_AINT64		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT64)
#define PTP_DTC_AUINT64		(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT64)
#define PTP_DTC_AINT128		(PTP_DTC_ARRAY_MASK | PTP_DTC_INT128)
#define PTP_DTC_AUINT128 	(PTP_DTC_ARRAY_MASK | PTP_DTC_UINT128)

#define PTP_DTC_STR			0xFFFF

#define PTP_GETSET_GET		0x00
#define PTP_GETSET_GETSET	0x01

#define PTP_FORM_NONE		0x00
#define PTP_FORM_RANGE		0x01
#define PTP_FORM_ENUM		0x02

#define PTP_VAL_SCM_UNDEFINED	0x0000
#define PTP_VAL_SCM_NORMAL		0x0001
#define PTP_VAL_SCM_BURST		0x0002
#define PTP_VAL_SCM_TIMELAPSE	0x0003

typedef struct _ptp_pima_decode_context
{
	void *ptr;
	size_t size;
	dynbuf *buf;
	void *adjlist[3];
} ptp_pima_decode_context;

typedef struct _ptp_pima_code_name
{
	uint16_t code;
	const char *name;
} ptp_pima_code_name;

typedef union _ptp_pima_basic_value
{
	uint8_t u8;
	int8_t s8;
	uint16_t u16;
	int16_t s16;
	uint32_t u32;
	int32_t s32;
	uint64_t u64;
	int64_t s64;
	uint128_t u128;
	int128_t s128;
	wchar_t str;
} ptp_pima_basic_value;

typedef struct _ptp_pima_prop_value
{
	uint32_t count;
	ptp_pima_basic_value *value;
} ptp_pima_prop_value;

typedef struct _ptp_pima_prop_form_range
{
	ptp_pima_prop_value min;
	ptp_pima_prop_value max;
	ptp_pima_prop_value step;
} ptp_pima_prop_form_range;

typedef struct _ptp_pima_prop_form_enum
{
	int count;
	ptp_pima_prop_value *values;
} ptp_pima_prop_form_enum;

typedef struct _ptp_pima_prop_form
{
	uint8_t type;
	union
	{
		ptp_pima_prop_form_range range;
		ptp_pima_prop_form_enum penum;
	};
} ptp_pima_prop_form;

typedef struct _ptp_pima_prop_desc
{
	ptp_pima_prop_code code;
	ptp_pima_type_code type;
	uint8_t get_set;
	ptp_pima_prop_value def;
	ptp_pima_prop_value val;
	ptp_pima_prop_form *form;
} ptp_pima_prop_desc;

typedef struct _ptp_pima_prop_desc_list
{
	int count;
	ptp_pima_prop_desc *desc;
	dynbuf *buf;
} ptp_pima_prop_desc_list;

typedef struct _ptp_pima_uint16_array
{
	int count;
	uint16_t *values;
} ptp_pima_uint16_array;

typedef struct _ptp_pima_device_info
{
	uint16_t version;
	uint32_t vendor_extension_id;
	uint16_t vendor_extension_version;
	wchar_t *vendor_extension_desc;
	uint16_t functional_mode;
	ptp_pima_uint16_array operations;
	ptp_pima_uint16_array events;
	ptp_pima_uint16_array properties;
	ptp_pima_uint16_array capture_formats;
	ptp_pima_uint16_array image_formats;
	wchar_t *manufacturer;
	wchar_t *model;
	wchar_t *device_version;
	wchar_t *serial_number;
	dynbuf *buf;
} ptp_pima_device_info;

typedef struct _ptp_pima_object_info
{
	uint32_t storage_id;
	uint16_t object_format;
	uint16_t protection_status;
	uint32_t object_compressed_size;
	uint16_t thumb_format;
	uint32_t thumb_compressed_size;
	uint32_t thumb_pix_width;
	uint32_t thumb_pix_height;
	uint32_t image_pix_width;
	uint32_t image_pix_height;
	uint32_t image_bit_depth;
	uint32_t parent_object;
	uint16_t assoc_type;
	uint32_t assoc_desc;
	uint32_t seq_number;
	wchar_t *filename;
	wchar_t *capture_date;
	wchar_t *modification_date;
	wchar_t *keywords;
	dynbuf *buf;
} ptp_pima_object_info;

int ptp_pima_open_session(ptp_device *dev, uint32_t session_id);
int ptp_pima_close_session(ptp_device *dev);
int ptp_pima_get_device_info(ptp_device *dev, ptp_pima_device_info *info);
int ptp_pima_get_object_info(ptp_device *dev, uint32_t object_handle, ptp_pima_object_info *info);
int ptp_pima_get_object(ptp_device *dev, uint32_t object_handle, void **object_data);

int ptp_pima_decode_int(ptp_pima_decode_context *ctx, void *p, size_t size);
int ptp_pima_decode_string(ptp_pima_decode_context *ctx, wchar_t **str);
int ptp_pima_decode_int_array(ptp_pima_decode_context *ctx, size_t elem_size, int *count, void **p);

int ptp_pima_decode_basic_value(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_basic_value *value);
int ptp_pima_decode_prop_value(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_prop_value *value);
int ptp_pima_decode_prop_form(ptp_pima_decode_context *ctx, ptp_pima_type_code type, ptp_pima_prop_form **form);
int ptp_pima_decode_prop_desc(ptp_pima_decode_context *ctx, ptp_pima_prop_desc *desc);
int ptp_pima_decode_device_info(ptp_pima_decode_context *ctx, ptp_pima_device_info *info);
int ptp_pima_decode_object_info(ptp_pima_decode_context *ctx, ptp_pima_object_info *info);

int ptp_pima_devinfo_create(ptp_pima_device_info **info);
void ptp_pima_devinfo_free(ptp_pima_device_info *info);
int ptp_pima_objinfo_create(ptp_pima_object_info **info);
void ptp_pima_objinfo_free(ptp_pima_object_info *info);
int ptp_pima_proplist_create(ptp_pima_prop_desc_list **list);
void ptp_pima_proplist_free(ptp_pima_prop_desc_list *list);
void ptp_pima_proplist_clear(ptp_pima_prop_desc_list *list);
ptp_pima_prop_desc * ptp_pima_proplist_get_prop(ptp_pima_prop_desc_list *list, ptp_pima_prop_code code);
ptp_pima_prop_desc * ptp_pima_proplist_get_prop_name(ptp_pima_prop_desc_list *desc, ptp_pima_prop_code code);
const char *ptp_pima_get_code_name(uint16_t code, const ptp_pima_code_name *names);
const char *ptp_pima_get_prop_name(ptp_pima_prop_code code);
const char *ptp_pima_get_op_name(ptp_pima_op_code code);
const char *ptp_pima_get_type_name(ptp_pima_type_code type);
void ptp_pima_print_prop_value(ptp_pima_type_code type, const ptp_pima_prop_value *value);
void ptp_pima_print_object_info(const ptp_pima_object_info *info);

#endif /* __PTP_PIMA_H__ */


#ifndef __PTP_SONY_H__
#define __PTP_SONY_H__

#include "ptp.h"
#include "ptp-pima.h"

#define PTP_OP_SONY_SDIOCONNECT			0x9201
#define PTP_OP_SONY_GETSDIOEXTDEVINFO	0x9202
#define PTP_OP_SONY_GETDEVICEPROPDESC	0x9203
#define PTP_OP_SONY_SETCONTROLDEVICEA	0x9205
#define PTP_OP_SONY_SETCONTROLDEVICEB	0x9207
#define PTP_OP_SONY_GETALLDEVPROPDATA	0x9209

#define PTP_DPC_SONY_DPCCompensation	0xD200
#define PTP_DPC_SONY_DRangeOptimize		0xD201
#define PTP_DPC_SONY_ImageSize			0xD203
#define PTP_DPC_SONY_ShutterSpeed		0xD20D
#define PTP_DPC_SONY_ColorTemp			0xD20F
#define PTP_DPC_SONY_CCFilter			0xD210
#define PTP_DPC_SONY_AspectRatio		0xD211
#define PTP_DPC_SONY_PendingImages		0xD215
#define PTP_DPC_SONY_ExposeIndex		0xD216
#define PTP_DPC_SONY_BatteryLevel		0xD218
#define PTP_DPC_SONY_PictureEffect		0xD21B
#define PTP_DPC_SONY_ABFilter			0xD21C
#define PTP_DPC_SONY_ISO				0xD21E
#define PTP_DPC_SONY_CTRL_AFLock		0xD2C1
#define PTP_DPC_SONY_CTRL_Shutter		0xD2C2
#define PTP_DPC_SONY_CTRL_AELock		0xD2C3
#define PTP_DPC_SONY_CTRL_StillImage	0xD2C7
#define PTP_DPC_SONY_CTRL_Movie			0xD2C8

#define PTP_EC_SONY_ObjectAdded			0xC201
#define PTP_EC_SONY_PropertyChanged		0xC203

#define PTP_VAL_SONY_SCM_SINGLE			0x0001
#define PTP_VAL_SONY_SCM_HIGH			0x0002
#define PTP_VAL_SONY_SCM_MID			0x8015
#define PTP_VAL_SONY_SCM_LOW			0x8012

typedef struct _ptp_sony_shutter_speed
{
	uint16_t num;
	uint16_t denom;
} ptp_sony_shutter_speed;

int ptp_sony_sdio_connect(ptp_device *dev, uint32_t param1, uint32_t param2, uint32_t param3);
int ptp_sony_get_sdio_ext_devinfo(ptp_device *dev, uint32_t version, ptp_pima_device_info *info);
int ptp_sony_get_all_dev_prop_data(ptp_device *dev, ptp_pima_prop_desc_list *list);
int ptp_sony_set_control_device_a(ptp_device *dev, uint32_t propcode, void *value, int size);
int ptp_sony_set_control_device_a_u16(ptp_device *dev, uint32_t propcode, uint16_t value);
int ptp_sony_set_control_device_a_u32(ptp_device *dev, uint32_t propcode, uint32_t value);
int ptp_sony_set_control_device_b(ptp_device *dev, uint32_t propcode, void *value, int size);
int ptp_sony_set_control_device_b_u16(ptp_device *dev, uint32_t propcode, uint16_t value);
int ptp_sony_set_control_device_b_u32(ptp_device *dev, uint32_t propcode, uint32_t value);
int ptp_sony_adjust_property(ptp_device *dev, ptp_pima_prop_code propcode, int up);
ptp_pima_prop_desc *ptp_sony_get_property(ptp_device *dev, ptp_pima_prop_desc_list *list, ptp_pima_prop_code propcode);
int ptp_sony_wait_object(ptp_device *dev, uint32_t *object_handle, int timeout);
int ptp_sony_wait_property(ptp_device *dev, ptp_pima_prop_code *code, int timeout);
int ptp_sony_wait_pending_object(ptp_device *dev);
int ptp_sony_get_pending_objects(ptp_device *dev);
int ptp_sony_handshake(ptp_device *dev);
int ptp_sony_set_drive_mode(ptp_device *dev, uint16_t mode);
int ptp_sony_set_shutter_speed(ptp_device *dev, const ptp_sony_shutter_speed *speed);
int ptp_sony_set_fnumber(ptp_device *dev, uint16_t fnumber);
int ptp_sony_set_iso(ptp_device *dev, uint32_t iso);

const char *ptp_sony_get_prop_name(ptp_pima_prop_code code);
const char *ptp_sony_get_op_name(ptp_pima_op_code code);

int ptp_sony_decode_prop_desc(ptp_pima_decode_context *ctx, ptp_pima_prop_desc *desc);
int ptp_sony_decode_prop_desc_list(ptp_pima_decode_context *ctx, ptp_pima_prop_desc_list *list);
void ptp_sony_print_prop_desc(const ptp_pima_prop_desc *desc);

int ptp_sony_decode_device_info(ptp_pima_decode_context *ctx, ptp_pima_device_info *info);
void ptp_sony_print_device_info(const ptp_pima_device_info *info);

#endif /* __PTP_SONY_H__ */

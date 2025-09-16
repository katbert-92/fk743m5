#ifndef __USBD_H
#define __USBD_H

#include "platform.h"
#include "usbd_cdc.h"

typedef int8_t (*USB_CDC_Init_Func)(void);
typedef int8_t (*USB_CDC_DeInit)(void);
typedef int8_t (*USB_CDC_Control)(uint8_t cmd, uint8_t* pbuf, uint16_t length);
typedef int8_t (*USB_CDC_Receive)(uint8_t* Buf, uint32_t* Len);
typedef int8_t (*USB_CDC_TransmitCplt)(uint8_t* Buf, uint32_t* Len, uint8_t epnum);

typedef int8_t (*USB_MSC_Init_FS_Func)(uint8_t lun);
typedef int8_t (*USB_MSC_GetCapacity_FS_Func)(uint8_t lun, uint32_t* block_num,
											  uint16_t* block_size);
typedef int8_t (*USB_MSC_IsReady_FS_Func)(uint8_t lun);
typedef int8_t (*USB_MSC_IsWriteProtected_FS_Func)(uint8_t lun);
typedef int8_t (*USB_MSC_Read_FS_Func)(uint8_t lun, uint8_t* buf, uint32_t blk_addr,
									   uint16_t blk_len);
typedef int8_t (*USB_MSC_Write_FS_Func)(uint8_t lun, uint8_t* buf, uint32_t blk_addr,
										uint16_t blk_len);
typedef int8_t (*USB_MSC_GetMaxLun_FS_Func)(void);

#define APP_RX_DATA_SIZE 128
#define APP_TX_DATA_SIZE 128

#define APP_RAM_MSC_BLOCK_SIZE 512

typedef enum {
	USBD_CLASS_UNDEF = 0x00,
	USBD_CLASS_CDC,
	USBD_CLASS_MSC,
	// USBD_CLASS_HID,
} USBD_CLASS_t;

bool USB_Init(USBD_CLASS_t class, Pl_Usb_RxClbk_t pRxClbk_USB, u8* pRxBuff);
bool USB_DeInit(void);
RET_STATE_t USB_SerialTransmit(char* pBuff, u16 len);
bool USB_CDC_IsReady(void);
USBD_CLASS_t USB_GetDeviceClass(void);

#endif /* __USBD_H */

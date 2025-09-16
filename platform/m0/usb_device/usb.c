#include "usb.h"
#include "debug.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"

static USBD_CLASS_t USB_DeviceClass = USBD_CLASS_UNDEF;
static USBD_HandleTypeDef hUsbDeviceHS;
extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

// -----------------------------------------------------------------------------

static USBD_StatusTypeDef USB_CDC_Init_HS(void);
static USBD_StatusTypeDef USB_CDC_DeInit_HS(void);
static USBD_StatusTypeDef USB_CDC_Control_HS(u8 cmd, u8* pBuf, u16 len);
static USBD_StatusTypeDef USB_CDC_Receive_HS(u8* pBuf, u32* pLen);
static USBD_StatusTypeDef USB_CDC_Transmit_HS(u8* pBuff, u16 len);
static USBD_StatusTypeDef USB_CDC_TransmitCplt_HS(u8* pBuf, u32* pLen, u8 epnum);

USBD_CDC_ItfTypeDef USBD_SerialInterface = {
	.Init		  = (USB_CDC_Init_Func)USB_CDC_Init_HS,
	.DeInit		  = (USB_CDC_DeInit)USB_CDC_DeInit_HS,
	.Control	  = (USB_CDC_Control)USB_CDC_Control_HS,
	.Receive	  = (USB_CDC_Receive)USB_CDC_Receive_HS,
	.TransmitCplt = (USB_CDC_TransmitCplt)USB_CDC_TransmitCplt_HS,
};

static u8 USBD_RxBuffHS[APP_RX_DATA_SIZE];
static u8 USBD_TxBuffHS[APP_TX_DATA_SIZE];
static u8* USB_CDC_RxBuffPtr   = USBD_RxBuffHS;
static bool USB_CDC_ReadyState = false;

void UsbRxClbkStub(u8* pBuff, u32 len) {
	DISCARD_UNUSED(pBuff);
	DISCARD_UNUSED(len);
}
static Pl_Usb_RxClbk_t RxClbk_USB = UsbRxClbkStub;

static USBD_StatusTypeDef USB_CDC_Init_HS(void) {
	USBD_CDC_SetTxBuffer(&hUsbDeviceHS, USBD_TxBuffHS, 0);
	USBD_CDC_SetRxBuffer(&hUsbDeviceHS, USBD_RxBuffHS);
	USB_CDC_ReadyState = true;
	return USBD_OK;
}

static USBD_StatusTypeDef USB_CDC_DeInit_HS(void) {
	USB_CDC_ReadyState = false;
	return USBD_OK;
}

static USBD_StatusTypeDef USB_CDC_Control_HS(u8 cmd, u8* pBuf, u16 len) {

	DISCARD_UNUSED(pBuf);
	DISCARD_UNUSED(len);

	switch (cmd) {
		case CDC_SEND_ENCAPSULATED_COMMAND:
			break;
		case CDC_GET_ENCAPSULATED_RESPONSE:
			break;
		case CDC_SET_COMM_FEATURE:
			break;
		case CDC_GET_COMM_FEATURE:
			break;
		case CDC_CLEAR_COMM_FEATURE:
			break;
		/*******************************************************************************/
		/* Line Coding Structure                                                       */
		/*-----------------------------------------------------------------------------*/
		/* Offset | Field       | Size | Value  | Description                          */
		/* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
		/* 4      | bCharFormat |   1  | Number | Stop bits                            */
		/*                                        0 - 1 Stop bit                       */
		/*                                        1 - 1.5 Stop bits                    */
		/*                                        2 - 2 Stop bits                      */
		/* 5      | bParityType |  1   | Number | Parity                               */
		/*                                        0 - None                             */
		/*                                        1 - Odd                              */
		/*                                        2 - Even                             */
		/*                                        3 - Mark                             */
		/*                                        4 - Space                            */
		/* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
		/*******************************************************************************/
		case CDC_SET_LINE_CODING:
			break;
		case CDC_GET_LINE_CODING:
			break;
		case CDC_SET_CONTROL_LINE_STATE:
			break;
		case CDC_SEND_BREAK:
			break;
		default:
			break;
	}

	return USBD_OK;
}

static USBD_StatusTypeDef USB_CDC_Receive_HS(u8* pBuf, u32* pLen) {
	USBD_CDC_SetRxBuffer(&hUsbDeviceHS, &pBuf[0]);
	USBD_CDC_ReceivePacket(&hUsbDeviceHS);

	RxClbk_USB(pBuf, *pLen);

	return USBD_OK;
}

static USBD_StatusTypeDef USB_CDC_Transmit_HS(u8* pBuf, u16 len) {
	USBD_StatusTypeDef result	  = USBD_OK;
	USBD_CDC_HandleTypeDef* pHcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceHS.pClassData;

	if (pHcdc->TxState != 0)
		return USBD_BUSY;

	USBD_CDC_SetTxBuffer(&hUsbDeviceHS, pBuf, len);
	result = USBD_CDC_TransmitPacket(&hUsbDeviceHS);

	return result;
}

static USBD_StatusTypeDef USB_CDC_TransmitCplt_HS(u8* pBuf, u32* pLen, u8 epnum) {
	DISCARD_UNUSED(pBuf);
	DISCARD_UNUSED(pLen);
	DISCARD_UNUSED(epnum);

	return USBD_OK;
}

RET_STATE_t USB_SerialTransmit(char* pBuff, u16 len) {
	RET_STATE_t retState = RET_STATE_ERROR;

	if (USB_DeviceClass != USBD_CLASS_CDC)
		return retState;

	USBD_StatusTypeDef status = USB_CDC_Transmit_HS((u8*)pBuff, len);
	switch (status) {
		case USBD_OK:
			retState = RET_STATE_SUCCESS;
			break;
		case USBD_BUSY:
			retState = RET_STATE_ERR_BUSY;
			break;
		default:
			break;
	}

	return retState;
}

bool USB_CDC_IsReady(void) {
	return USB_CDC_ReadyState;
}

// -----------------------------------------------------------------------------
// MSC Device
extern USBD_StorageTypeDef USBD_StorageInterface;

// -----------------------------------------------------------------------------

bool USB_Init(USBD_CLASS_t class, Pl_Usb_RxClbk_t pRxClbk_USB, u8* pRxBuff) {

	if (!Pl_IsInit.Sys || !Pl_IsInit.Hsi48Clk) {
		PANIC();
		return false;
	}

	ASSIGN_NOT_NULL_VAL_TO_PTR(RxClbk_USB, pRxClbk_USB);

	if (class == USBD_CLASS_CDC && pRxBuff != NULL)
		USB_CDC_RxBuffPtr = pRxBuff;

	if (USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS) == USBD_OK) {
		if (USBD_RegisterClass(&hUsbDeviceHS, class == USBD_CLASS_CDC ? &USBD_CDC : &USBD_MSC) ==
			USBD_OK) {

			USBD_StatusTypeDef stat = USBD_FAIL;
			switch (class) {
				case USBD_CLASS_CDC:
					stat = USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_SerialInterface);
					break;
				case USBD_CLASS_MSC:
					stat = USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_StorageInterface);
					break;
			}

			if (stat == USBD_OK)
				if (USBD_Start(&hUsbDeviceHS) == USBD_OK) {
					USB_DeviceClass = class;
					return true;
				}
		}

		PANIC();
	}

	PANIC();

	return false;
}

bool USB_DeInit(void) {

	if (!Pl_IsInit.Sys || !Pl_IsInit.Hsi48Clk) {
		PANIC();
		return false;
	}

	USBD_StatusTypeDef stat = USBD_DeInit(&hUsbDeviceHS);
	if (stat == USBD_OK) {
		USB_DeviceClass = USBD_CLASS_UNDEF;
		return true;
	}

	return false;
}

USBD_CLASS_t USB_GetDeviceClass(void) {
	return USB_DeviceClass;
}

void OTG_HS_IRQHandler(void) {
	HAL_PCD_IRQHandler(&hpcd_USB_OTG_HS);
}
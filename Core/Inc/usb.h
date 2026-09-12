#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stddef.h>

/*==========================================================
 * USB Configuration
 *==========================================================*/

#define USB_RX_BUFFER_SIZE    256U
#define USB_TX_BUFFER_SIZE    512U


/*==========================================================
 * USB Status
 *==========================================================*/

typedef enum
{
    USB_OK = 0,
    USB_ERROR,
    USB_BUSY,
    USB_NOT_READY,
    USB_NO_DATA

} USB_Status;


/*==========================================================
 * Initialization
 *==========================================================*/

/**
 * @brief Initialize application USB layer.
 *
 * USB device hardware itself is initialized by
 * MX_USB_DEVICE_Init().
 */
void USB_Init(void);


/*==========================================================
 * TX API
 *==========================================================*/

/**
 * @brief Send raw bytes over USB CDC.
 *
 * @param data Pointer to data
 * @param length Number of bytes
 *
 * @return USB_Status
 */
USB_Status USB_Send(const uint8_t *data, uint16_t length);


/**
 * @brief Send a null-terminated string over USB CDC.
 *
 * @param string String to transmit
 *
 * @return USB_Status
 */
USB_Status USB_SendString(const char *string);


/*==========================================================
 * RX API
 *==========================================================*/

/**
 * @brief Check whether USB data is available.
 *
 * @return Number of bytes currently available.
 */
uint16_t USB_DataAvailable(void);


/**
 * @brief Read received USB data.
 *
 * @param buffer Destination buffer
 * @param buffer_size Maximum number of bytes to read
 *
 * @return Number of bytes actually read
 */
uint16_t USB_Read(uint8_t *buffer, uint16_t buffer_size);


/**
 * @brief Clear all received USB data.
 */
void USB_FlushRX(void);


/*==========================================================
 * Connection Status
 *==========================================================*/

/**
 * @brief Check whether USB is configured by the host.
 *
 * @return 1 if connected/configured, 0 otherwise.
 */
uint8_t USB_IsConnected(void);


/*==========================================================
 * Internal CDC Callbacks
 *
 * These are called from usbd_cdc_if.c.
 * Application code normally should not call them.
 *==========================================================*/

void USB_RxCallback(uint8_t *data, uint32_t length);

void USB_TxCompleteCallback(void);

#endif /* USB_H */

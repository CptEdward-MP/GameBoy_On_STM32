#include "usb.h"

#include <string.h>

#include "usbd_cdc_if.h"
#include "usbd_def.h"
#include "usbd_cdc.h"

#include "usbd_cdc_if.h"
#include "usb_device.h"

/*==========================================================
 * Private Variables
 *==========================================================*/

/*
 * RX ring buffer.
 *
 * USB callback writes into this buffer.
 * Application reads from this buffer.
 */
static uint8_t rx_buffer[USB_RX_BUFFER_SIZE];

static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;


/*
 * USB initialization state.
 */
static volatile uint8_t usb_initialized = 0;


/*==========================================================
 * Private Helper Functions
 *==========================================================*/

static uint16_t RX_NextIndex(uint16_t index)
{
    index++;

    if (index >= USB_RX_BUFFER_SIZE)
    {
        index = 0;
    }

    return index;
}


static uint16_t RX_Available(void)
{
    uint16_t head = rx_head;
    uint16_t tail = rx_tail;

    if (head >= tail)
    {
        return head - tail;
    }

    return USB_RX_BUFFER_SIZE - tail + head;
}


/*==========================================================
 * Initialization
 *==========================================================*/

void USB_Init(void)
{
    rx_head = 0;
    rx_tail = 0;

    usb_initialized = 1;
}


/*==========================================================
 * TX
 *==========================================================*/

USB_Status USB_Send(const uint8_t *data, uint16_t length)
{
    uint8_t result;

    if (data == NULL || length == 0)
    {
        return USB_ERROR;
    }

    if (!usb_initialized)
    {
        return USB_NOT_READY;
    }

    result = CDC_Transmit_FS((uint8_t *)data, length);

    if (result == USBD_OK)
    {
        return USB_OK;
    }

    if (result == USBD_BUSY)
    {
        return USB_BUSY;
    }

    return USB_ERROR;
}


USB_Status USB_SendString(const char *string)
{
    uint16_t length;

    if (string == NULL)
    {
        return USB_ERROR;
    }

    length = (uint16_t)strlen(string);

    return USB_Send((const uint8_t *)string, length);
}


/*==========================================================
 * RX
 *==========================================================*/

void USB_RxCallback(uint8_t *data, uint32_t length)
{
    uint32_t i;

    if (data == NULL)
    {
        return;
    }

    for (i = 0; i < length; i++)
    {
        uint16_t next_head;

        next_head = RX_NextIndex(rx_head);

        /*
         * If next head equals tail, buffer is full.
         * Drop the incoming byte.
         */
        if (next_head == rx_tail)
        {
            break;
        }

        rx_buffer[rx_head] = data[i];

        rx_head = next_head;
    }
}


uint16_t USB_DataAvailable(void)
{
    return RX_Available();
}


uint16_t USB_Read(uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t count = 0;

    if (buffer == NULL || buffer_size == 0)
    {
        return 0;
    }

    while ((rx_tail != rx_head) &&
           (count < buffer_size))
    {
        buffer[count] = rx_buffer[rx_tail];

        rx_tail = RX_NextIndex(rx_tail);

        count++;
    }

    return count;
}


void USB_FlushRX(void)
{
    rx_tail = rx_head;
}


/*==========================================================
 * Connection Status
 *==========================================================*/

uint8_t USB_IsConnected(void)
{
    return usb_initialized;
}


/*==========================================================
 * TX Complete
 *==========================================================*/

void USB_TxCompleteCallback(void)
{
    /*
     * Reserved for future use.
     *
     * Later we can use this to wake the USB task,
     * release a semaphore, etc.
     */
}

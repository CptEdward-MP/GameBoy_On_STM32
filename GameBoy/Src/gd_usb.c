#include "gd_usb.h"
#include "usb.h"

#define GD_PACKET_HEADER_SIZE  5
#define GD_PACKET_CRC_SIZE     2



static void GD_USB_SendBlocking(const uint8_t *data, uint16_t length)
{
    while (USB_Send(data, length) == USB_BUSY)
    {
        /* Wait for previous USB transfer to complete */
    }
}



/* ============================================================
 * CRC
 * ============================================================ */

static uint16_t GD_USB_CalculateCRC(uint16_t crc,
                                    const uint8_t *data,
                                    uint32_t length)
{
    for (uint32_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}


/* ============================================================
 * Send Game Boy framebuffer
 * ============================================================ */
void GD_USB_SendFrame16(const uint16_t *frame)
{
    uint8_t header[GD_PACKET_HEADER_SIZE];
    uint8_t crc_bytes[GD_PACKET_CRC_SIZE];

    const uint32_t length = GD_FRAME_SIZE;

    header[0] = GD_PACKET_MAGIC_0;
    header[1] = GD_PACKET_MAGIC_1;
    header[2] = GD_PACKET_TYPE_FRAME;
    header[3] = (uint8_t)(length & 0xFF);
    header[4] = (uint8_t)((length >> 8) & 0xFF);

    uint16_t crc = 0xFFFF;

    crc = GD_USB_CalculateCRC(
        crc,
        header,
        GD_PACKET_HEADER_SIZE
    );

    crc = GD_USB_CalculateCRC(
        crc,
        (const uint8_t *)frame,
        GD_FRAME_SIZE
    );

    crc_bytes[0] = (uint8_t)(crc & 0xFF);
    crc_bytes[1] = (uint8_t)(crc >> 8);

    GD_USB_SendBlocking(header, GD_PACKET_HEADER_SIZE);

    GD_USB_SendBlocking(
        (const uint8_t *)frame,
        GD_FRAME_SIZE
    );

    GD_USB_SendBlocking(crc_bytes, GD_PACKET_CRC_SIZE);
}

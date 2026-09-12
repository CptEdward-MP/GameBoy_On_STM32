#include "gd_usb.h"
#include "usb.h"

#include "gameboy_bridge.h"

#define GD_PACKET_HEADER_SIZE    5
#define GD_PACKET_CRC_SIZE       2
#define GD_PACKET_OVERHEAD       (GD_PACKET_HEADER_SIZE + GD_PACKET_CRC_SIZE)

static uint8_t framebuffer[GD_FRAME_SIZE];



/*==========================================================
 * USB Transport
 *==========================================================*/

void GD_USB_Send(const uint8_t *data, uint16_t length)
{
    USB_Send(data, length);
}


/*==========================================================
 * CRC
 *==========================================================*/

static uint16_t GD_USB_CalculateCRC(const uint8_t *data,
                                    uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc;
}


/*==========================================================
 * Frame Packet
 *==========================================================*/

static void GD_USB_SendFramePacket(void)
{
    static uint8_t packet[GD_PACKET_OVERHEAD + GD_FRAME_SIZE];

    uint16_t length = GD_FRAME_SIZE;

    /* Header */
    packet[0] = GD_PACKET_MAGIC_0;
    packet[1] = GD_PACKET_MAGIC_1;
    packet[2] = GD_PACKET_TYPE_FRAME;

    /* Payload length - little endian */
    packet[3] = (uint8_t)(length & 0xFF);
    packet[4] = (uint8_t)(length >> 8);


    /* Payload */
    for (uint16_t i = 0; i < GD_FRAME_SIZE; i++)
    {
        packet[GD_PACKET_HEADER_SIZE + i] = framebuffer[i];
    }


    /* CRC covers header + payload */
    uint16_t crc = GD_USB_CalculateCRC(
        packet,
        GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE
    );


    /* Append CRC - little endian */
    packet[GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE] =
        (uint8_t)(crc & 0xFF);

    packet[GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE + 1] =
        (uint8_t)(crc >> 8);


    /* Send complete packet */
    GD_USB_Send(packet, sizeof(packet));
}

void GD_USB_SendFrame(const uint8_t *framebuffer)
{
    static uint8_t packet[GD_PACKET_OVERHEAD + GD_FRAME_SIZE];

    uint16_t length = GD_FRAME_SIZE;

    packet[0] = GD_PACKET_MAGIC_0;
    packet[1] = GD_PACKET_MAGIC_1;
    packet[2] = GD_PACKET_TYPE_FRAME;

    packet[3] = (uint8_t)(length & 0xFF);
    packet[4] = (uint8_t)(length >> 8);

    for (uint16_t i = 0; i < GD_FRAME_SIZE; i++)
    {
        packet[GD_PACKET_HEADER_SIZE + i] = framebuffer[i];
    }

    uint16_t crc = GD_USB_CalculateCRC(
        packet,
        GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE
    );

    packet[GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE] =
        (uint8_t)(crc & 0xFF);

    packet[GD_PACKET_HEADER_SIZE + GD_FRAME_SIZE + 1] =
        (uint8_t)(crc >> 8);

    GD_USB_Send(packet, sizeof(packet));
}


//
//
//void GD_USB_Init(void)
//{
//    for (uint16_t y = 0; y < GD_USB_HEIGHT; y++)
//    {
//        for (uint16_t x = 0; x < GD_USB_WIDTH; x++)
//        {
//            uint8_t pixel;
//
//            /* 1-pixel border */
//            if (x == 0 || x == GD_USB_WIDTH - 1 ||
//                y == 0 || y == GD_USB_HEIGHT - 1)
//            {
//                pixel = 3;
//            }
//
//            /* Top-left: dark */
//            else if (x < GD_USB_WIDTH / 2 &&
//                     y < GD_USB_HEIGHT / 2)
//            {
//                pixel = 0;
//            }
//
//            /* Top-right: light */
//            else if (x >= GD_USB_WIDTH / 2 &&
//                     y < GD_USB_HEIGHT / 2)
//            {
//                pixel = 3;
//            }
//
//            /* Bottom-left: light gray */
//            else if (x < GD_USB_WIDTH / 2 &&
//                     y >= GD_USB_HEIGHT / 2)
//            {
//                pixel = 2;
//            }
//
//            /* Bottom-right: checkerboard */
//            else
//            {
//                if (((x / 8) + (y / 8)) % 2 == 0)
//                    pixel = 0;
//                else
//                    pixel = 3;
//            }
//
//            framebuffer[y * GD_USB_WIDTH + x] = pixel;
//        }
//    }
//}
//
//
//
//void GD_USB_SendTestPattern(void)
//{
//    uint8_t test_data[256];
//
//    for (uint16_t i = 0; i < 256; i++)
//    {
//        test_data[i] = (uint8_t)i;
//    }
//
//    USB_Send(test_data, 256);
//}
//
//
//
//void GD_USB_SendTestFrame(void)
//{
//    static uint8_t frame = 0;
//
//    for (uint16_t y = 0; y < GD_USB_HEIGHT; y++)
//    {
//        for (uint16_t x = 0; x < GD_USB_WIDTH; x++)
//        {
//            uint8_t pixel = 0;
//
//            /*
//             * Border
//             */
//            if (x == 0 || x == GD_USB_WIDTH - 1 ||
//                y == 0 || y == GD_USB_HEIGHT - 1)
//            {
//                pixel = 3;
//            }
//
//            /*
//             * Frame 0
//             * GD logo - one position
//             */
//            else if (frame == 0)
//            {
//                /*
//                 * G
//                 */
//                if ((x >= 50 && x < 80 && y >= 50 && y < 94) &&
//                    (x < 56 ||
//                     (y < 56 && x < 75) ||
//                     (y >= 88 && x < 75) ||
//                     (y >= 70 && y < 76 && x < 72) ||
//                     (x >= 70 && y >= 70 && y < 76)))
//                {
//                    pixel = 3;
//                }
//
//                /*
//                 * D
//                 */
//                if ((x >= 85 && x < 115 && y >= 50 && y < 94) &&
//                    (x < 91 ||
//                     (y < 56 && x < 108) ||
//                     (y >= 88 && x < 108) ||
//                     (x >= 108 && y >= 56 && y < 88)))
//                {
//                    pixel = 3;
//                }
//            }
//
//            /*
//             * Frame 1
//             * GD logo shifted slightly
//             */
//            else
//            {
//                /*
//                 * G
//                 */
//                if ((x >= 55 && x < 85 && y >= 50 && y < 94) &&
//                    (x < 61 ||
//                     (y < 56 && x < 80) ||
//                     (y >= 88 && x < 80) ||
//                     (y >= 70 && y < 76 && x < 77) ||
//                     (x >= 75 && y >= 70 && y < 76)))
//                {
//                    pixel = 3;
//                }
//
//                /*
//                 * D
//                 */
//                if ((x >= 90 && x < 120 && y >= 50 && y < 94) &&
//                    (x < 96 ||
//                     (y < 56 && x < 113) ||
//                     (y >= 88 && x < 113) ||
//                     (x >= 113 && y >= 56 && y < 88)))
//                {
//                    pixel = 3;
//                }
//            }
//
//            framebuffer[y * GD_USB_WIDTH + x] = pixel;
//        }
//    }
//
//    GD_USB_Send(framebuffer, GD_USB_FRAME_SIZE);
//
//    /*
//     * Switch between the two frames
//     */
//    frame ^= 1;
//}




//void GD_USB_SendTestFrame(void)
//{
//    USB_Send(framebuffer, GD_USB_FRAME_SIZE);
//}


//void GD_USB_SendTestFrame(void)
//{
//    static uint8_t phase = 0;
//
//    for (uint16_t y = 0; y < GD_USB_HEIGHT; y++)
//    {
//        for (uint16_t x = 0; x < GD_USB_WIDTH; x++)
//        {
//            uint8_t pixel;
//
//            /* Moving vertical band */
//            uint16_t band = (x + phase) % GD_USB_WIDTH;
//
//            if (band < 40)
//            {
//                pixel = 3;
//            }
//            else if (band < 80)
//            {
//                pixel = 2;
//            }
//            else if (band < 120)
//            {
//                pixel = 1;
//            }
//            else
//            {
//                pixel = 0;
//            }
//
//            /* Add a moving bright square */
//            uint16_t square_x = (phase * 2) % 130;
//            uint16_t square_y = 55 + ((phase / 4) % 35);
//
//            if (x >= square_x &&
//                x < square_x + 30 &&
//                y >= square_y &&
//                y < square_y + 30)
//            {
//                pixel = 3;
//            }
//
//            framebuffer[y * GD_USB_WIDTH + x] = pixel;
//        }
//    }
//
//    GD_USB_Send(framebuffer, GD_USB_FRAME_SIZE);
//
//    phase++;
//}

#ifndef GD_USB_H
#define GD_USB_H

#include <stdint.h>

#define GD_PACKET_MAGIC_0       0x47
#define GD_PACKET_MAGIC_1       0x42
#define GD_PACKET_TYPE_FRAME    0x01

#define GD_FRAME_WIDTH          160
#define GD_FRAME_HEIGHT         144
#define GD_FRAME_SIZE           (GD_FRAME_WIDTH * GD_FRAME_HEIGHT * 2)

void GD_USB_Init(void);

void GD_USB_Send(const uint8_t *data, uint16_t length);

void GD_USB_SendFrame16(const uint16_t *frame);

#endif /* GD_USB_H */

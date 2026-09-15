import serial
import pygame

# ============================================================
# Configuration
# ============================================================

PORT = "COM13"
BAUDRATE = 115200

WIDTH = 160
HEIGHT = 144
SCALE = 5

FRAME_SIZE = WIDTH * HEIGHT*2

# ============================================================
# Packet Configuration
# ============================================================

MAGIC = b"GB"
PACKET_TYPE_FRAME = 0x01

HEADER_SIZE = 5
CRC_SIZE = 2

# ============================================================
# Game Boy Palette
# ============================================================

PALETTE = [
    (15, 56, 15),
    (48, 98, 48),
    (139, 172, 15),
    (155, 188, 15),
]


# ============================================================
# USB
# ============================================================

ser = serial.Serial(
    PORT,
    BAUDRATE,
    timeout=2
)

print("Connected to", PORT)


# ============================================================
# Pygame
# ============================================================

pygame.init()

screen = pygame.display.set_mode(
    (WIDTH * SCALE, HEIGHT * SCALE)
)

pygame.display.set_caption(
    "Game Boy - STM32 Framebuffer"
)


# ============================================================
# CRC
# ============================================================

def calculate_crc(data):

    crc = 0xFFFF

    for byte in data:

        crc ^= byte

        for _ in range(8):

            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1

    return crc


# ============================================================
# Packet Receive
# ============================================================

def receive_frame():

    # --------------------------------------------------------
    # Find packet magic: "GB"
    # --------------------------------------------------------

    while True:

        byte = ser.read(1)

        if not byte:
            return None

        if byte == b"G":

            byte = ser.read(1)

            if byte == b"B":
                break


    # --------------------------------------------------------
    # Read packet header
    #
    # Header:
    #
    # Byte 0 = Packet Type
    # Byte 1 = Length Low
    # Byte 2 = Length High
    # --------------------------------------------------------

    header = ser.read(3)

    if len(header) != 3:
        return None

    packet_type = header[0]

    length = (
        header[1]
        | (header[2] << 8)
    )


    # --------------------------------------------------------
    # Check packet type
    # --------------------------------------------------------

    if packet_type != PACKET_TYPE_FRAME:

        print(
            "Unknown packet type:",
            hex(packet_type)
        )

        return None


    # --------------------------------------------------------
    # Check payload size
    # --------------------------------------------------------

    if length != FRAME_SIZE:

        print(
            "Invalid frame size:",
            length
        )

        return None


    # --------------------------------------------------------
    # Receive framebuffer
    # --------------------------------------------------------

    payload = ser.read(length)

    if len(payload) != length:

        print(
            "Incomplete payload:",
            len(payload),
            "/",
            length
        )

        return None


    # --------------------------------------------------------
    # Receive CRC
    # --------------------------------------------------------

    crc_data = ser.read(CRC_SIZE)

    if len(crc_data) != CRC_SIZE:
        return None

    received_crc = (
        crc_data[0]
        | (crc_data[1] << 8)
    )


    # --------------------------------------------------------
    # Calculate CRC
    # --------------------------------------------------------

    packet_data = (
        MAGIC
        + header
        + payload
    )

    calculated_crc = calculate_crc(packet_data)


    # --------------------------------------------------------
    # Validate CRC
    # --------------------------------------------------------

    if calculated_crc != received_crc:

        print(
            "CRC ERROR:",
            hex(calculated_crc),
            "!=",
            hex(received_crc)
        )

        return None


    # --------------------------------------------------------
    # CRC Passed
    # --------------------------------------------------------

    print(
        "CRC OK:",
        hex(received_crc)
    )

    return payload


# ============================================================
# Main Loop
# ============================================================

running = True

while running:

    # --------------------------------------------------------
    # Handle Pygame events
    # --------------------------------------------------------

    for event in pygame.event.get():

        if event.type == pygame.QUIT:
            running = False


    # --------------------------------------------------------
    # Receive complete Game Boy frame
    # --------------------------------------------------------

    data = receive_frame()

    if data is None:
        continue


    print(
        "Received frame:",
        len(data),
        "bytes"
    )

    # --------------------------------------------------------
    # Draw framebuffer
    # --------------------------------------------------------

    for y in range(HEIGHT):

        for x in range(WIDTH):

            index = (y * WIDTH + x) * 2

            pixel = (
                data[index]
                | (data[index + 1] << 8)
            )

            # RGB555-style Game Boy pixel
            r = (pixel >> 10) & 0x1F
            g = (pixel >> 5) & 0x1F
            b = pixel & 0x1F

            # 5-bit -> 8-bit
            r = (r * 255) // 31
            g = (g * 255) // 31
            b = (b * 255) // 31

            pygame.draw.rect(
                screen,
                (r, g, b),
                (
                    x * SCALE,
                    y * SCALE,
                    SCALE,
                    SCALE
                )
            )


    # --------------------------------------------------------
    # Update display
    # --------------------------------------------------------

    pygame.display.flip()


# ============================================================
# Cleanup
# ============================================================

ser.close()

pygame.quit()
#include "gameboy_bridge.h"

#include "gb.h"
#include "loop.h"

static GB gameboy;

static float cycle_accumulator = 0.0f;

static constexpr float MCYCLES_PER_FRAME = 17556.0f;


volatile uint32_t gb_loop_count = 0;
extern "C" volatile uint32_t gb_debug_counter = 0;
extern "C" volatile uint32_t gb_debug_stage = 0;

extern "C"
void GameBoy_Init(const uint8_t *rom, uint32_t size)
{
    gameboy.cart.init(&gameboy, rom, size);
    gameboy.gb_init();
}


extern "C"
volatile uint32_t gb_frame_cycles = 0;



extern "C"
uint32_t GameBoy_GetDebugStage(void)
{
    return gb_debug_stage;
}

extern "C"
void GameBoy_RunFrame(void)
{
    uint32_t frame_cycles = 0;

    while (cycle_accumulator <= MCYCLES_PER_FRAME)
    {
        float cycles = static_cast<float>(gb_loop(&gameboy));

        cycle_accumulator += cycles;
        frame_cycles += (uint32_t)cycles;
    }

    cycle_accumulator -= MCYCLES_PER_FRAME;

    gb_frame_cycles = frame_cycles;
}
//extern "C"
//void GameBoy_RunFrame(void)
//{
//    while (cycle_accumulator <= MCYCLES_PER_FRAME)
//    {
//        float cycles = static_cast<float>(gb_loop(&gameboy));
//        cycle_accumulator += cycles;
//    }
//
//    cycle_accumulator -= MCYCLES_PER_FRAME;
//}

extern "C"
void GameBoy_GetFrame(uint8_t *buffer)
{
    for (uint32_t i = 0; i < 160 * 144; i++)
    {
        uint16_t pixel = gameboy.mmu.gb_framebuffer[i];

        if (pixel == 0xFFFF)
            buffer[i] = 0;
        else if (pixel == 0xAD6B)
            buffer[i] = 1;
        else if (pixel == 0x5295)
            buffer[i] = 2;
        else if (pixel == 0x0001)
            buffer[i] = 3;
        else
            buffer[i] = 0;
    }
}
//void GameBoy_GetFrame(uint8_t *buffer)
//{
//    for (uint32_t i = 0; i < 160 * 144; i++)
//    {
//        uint16_t pixel = gameboy.mmu.gb_framebuffer[i];
//
//        switch (pixel)
//        {
//            case 0xFFFF:
//                buffer[i] = 0;
//                break;
//
//            case 0xAD6B:
//                buffer[i] = 1;
//                break;
//
//            case 0x5295:
//                buffer[i] = 2;
//                break;
//
//            case 0x0001:
//                buffer[i] = 3;
//                break;
//
//            default:
//                buffer[i] = 0;
//                break;
//        }
//    }
//}



//extern "C"
//void GameBoy_GetFrame(uint8_t *buffer)
//{
//    for (uint32_t i = 0; i < 160 * 144; i++)
//    {
//        uint16_t pixel = gameboy.mmu.gb_framebuffer[i];
//
//        switch (pixel)
//        {
//            case 0xFFFF:
//                buffer[i] = 0;
//                break;
//
//            case 0xAD6B:
//                buffer[i] = 1;
//                break;
//
//            case 0x5295:
//                buffer[i] = 2;
//                break;
//
//            case 0x0001:
//                buffer[i] = 3;
//                break;
//
//            default:
//                buffer[i] = 0;
//                break;
//        }
//    }
//}

extern "C"
uint16_t GameBoy_GetPixel(void)
{
    return gameboy.mmu.gb_framebuffer[0];
}



extern "C"
void GameBoy_GetTestPixels(uint16_t *buffer)
{
    buffer[0] = gameboy.mmu.gb_framebuffer[0];
    buffer[1] = gameboy.mmu.gb_framebuffer[159];
    buffer[2] = gameboy.mmu.gb_framebuffer[143 * 160];
    buffer[3] = gameboy.mmu.gb_framebuffer[143 * 160 + 159];
    buffer[4] = gameboy.mmu.gb_framebuffer[72 * 160 + 80];
}




extern "C"
void GameBoy_TestCartInit(const uint8_t *rom, uint32_t size)
{
    gameboy.cart.init(&gameboy, rom, size);
}

extern "C"
void GameBoy_TestGBInit(void)
{
    gameboy.gb_init();
}




extern "C"
void GameBoy_GetFrameStats(uint16_t *min_value,
                           uint16_t *max_value,
                           uint32_t *non_white)
{
    uint16_t min = 0xFFFF;
    uint16_t max = 0x0000;
    uint32_t count = 0;

    for (uint32_t i = 0; i < 160 * 144; i++)
    {
        uint16_t pixel = gameboy.mmu.gb_framebuffer[i];

        if (pixel < min)
            min = pixel;

        if (pixel > max)
            max = pixel;

        if (pixel != 0xFFFF)
            count++;
    }

    *min_value = min;
    *max_value = max;
    *non_white = count;
}


extern "C"
void GameBoy_GetPaletteStats(uint32_t *white,
                             uint32_t *light_gray,
                             uint32_t *dark_gray,
                             uint32_t *black)
{
    *white = 0;
    *light_gray = 0;
    *dark_gray = 0;
    *black = 0;

    for (uint32_t i = 0; i < 160 * 144; i++)
    {
        uint16_t p = gameboy.mmu.gb_framebuffer[i];

        if (p == 0xFFFF)
            (*white)++;
        else if (p == 0xAD6B)
            (*light_gray)++;
        else if (p == 0x5295)
            (*dark_gray)++;
        else if (p == 0x0001)
            (*black)++;
    }
}

#ifndef GAMEBOY_BRIDGE_H
#define GAMEBOY_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void GameBoy_Init(const uint8_t *rom, uint32_t size);
void GameBoy_RunFrame(void);
void GameBoy_GetFrame(uint8_t *buffer);


//===============  Test  ==========================//
uint16_t GameBoy_GetPixel(void);
void GameBoy_GetTestPixels(uint16_t *buffer);

void GameBoy_TestCartInit(const uint8_t *rom, uint32_t size);
void GameBoy_TestGBInit(void);
extern volatile uint32_t gb_debug_counter;
extern volatile uint32_t gb_frame_cycles;
extern volatile uint32_t gb_debug_stage;

uint32_t GameBoy_GetDebugStage(void);

void GameBoy_GetFrameStats(uint16_t *min_value,
                           uint16_t *max_value,
                           uint32_t *non_white);


void GameBoy_GetPaletteStats(uint32_t *white,
                             uint32_t *light_gray,
                             uint32_t *dark_gray,
                             uint32_t *black);
//=================================================//


#ifdef __cplusplus
}
#endif

#endif

#ifndef GAMEBOY_BRIDGE_H
#define GAMEBOY_BRIDGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void GameBoy_Init(const uint8_t *rom, uint32_t size);
void GameBoy_RunFrame(void);

const uint16_t *GameBoy_GetFrameBuffer(void);

#ifdef __cplusplus
}
#endif

#endif

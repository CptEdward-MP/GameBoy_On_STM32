#ifndef PPU_PROFILER_H
#define PPU_PROFILER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void ppu_set_cycle_counter(uint32_t (*counter)(void));

#ifdef __cplusplus
}
#endif

#endif

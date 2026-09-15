#pragma once

#include <stdint.h>

auto ppu_stat_interrupt_service(GB* gb) -> void;

auto ppu_oam_dma_transfer(GB* gb) -> void;

auto ppu_OAM_scan_mode(GB* gb) -> void;

auto ppu_drawing_mode(GB* gb) -> void;

void ppu_set_cycle_counter(uint32_t (*counter)(void));

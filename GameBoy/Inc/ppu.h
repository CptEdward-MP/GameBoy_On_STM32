#pragma once

auto ppu_stat_interrupt_service(GB* gb) -> void;

auto ppu_oam_dma_transfer(GB* gb) -> void;
auto ppu_OAM_scan_mode(GB* gb) -> void;

auto ppu_drawing_mode(GB* gb) -> void;

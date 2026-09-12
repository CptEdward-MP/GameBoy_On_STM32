#pragma once

#include "typedefs.h"

typedef struct GB GB;

auto mmu_init_map(GB* gb) -> void;

auto bus_default_read(GB* gb, u16 address) -> u8;
auto bus_read(GB* gb, u16 address) -> u8;

auto bus_default_lockless_write(GB* gb, u16 address, u8 byte) -> void;
auto bus_default_write(GB* gb, u16 address, u8 byte) -> void;
auto bus_write(GB* gb, u16 address, u8 byte) -> void;

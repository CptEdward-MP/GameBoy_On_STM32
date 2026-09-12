#pragma once

#include "typedefs.h"

#include "gb.h"

// typedef struct GB GB;

auto gb_update_tima(GB* gb) -> void;

auto gb_loop(GB* gb) -> u8;

auto dbg_test(GB* gb) -> void;

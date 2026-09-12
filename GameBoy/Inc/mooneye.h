#pragma once

#include "gb.h"

f_inline
static auto gb_mooneye_test(GB* gb) -> void
{
  auto& cpu = gb->cpu;

  if (cpu.IR == 0x40)
  {
    if (cpu.B == 3  &&
        cpu.C == 5  &&
        cpu.D == 8  &&
        cpu.E == 13 &&
        cpu.H == 21 &&
        cpu.L == 34)
    {
      gb::print("Mooneye: Test passed!\n");
      exit(0);
    }
    else
    {
      gb::print("Mooneye: Test failed!\n");
      gb::print("B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} L: {:02X} ticks: {}\n",
                 // banking_mode:{} 5_bits:{} 2_bits:{} ram_bank:{} rom_bank:{}\n",
                 cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, gb->ticks
                 // gb->cart.mbc.banking_mode, gb->cart.mbc.five_bits, gb->cart.mbc.two_bits, gb->cart.mbc.ram_bank, gb->cart.mbc.rom_bank);
                );
      exit(1);
    }
  }
}


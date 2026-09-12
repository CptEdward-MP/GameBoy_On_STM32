#pragma once

enum class INTR
{
  VBLANK = 0,
  LCD = 1,
  TIMER = 2,
  SERIAL = 3,
  JOYP = 4
};

f_inline auto gb_set_IF_interrupt(u8* io, INTR interrupt) -> void
{
  io[0xF] |= (1 << static_cast<u8>(interrupt));
}

f_inline auto gb_get_IF_interrupt(u8* io, INTR interrupt) -> bool
{
  return static_cast<bool>((io[0xF] >> static_cast<u8>(interrupt)) & 1);
}



auto gb_interrupt_handler(GB* gb) -> void;

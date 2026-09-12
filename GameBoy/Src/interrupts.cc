#include "gb.h"
#include "mmu.h"



/* used in gb_interrupt_jump() */
static constexpr u8  flags[]   = { 0x01,   0x02,   0x04,   0x08,   0x10   };
static constexpr u16 vectors[] = { 0x0040, 0x0048, 0x0050, 0x0058, 0x0060 };



// f_inline static
// auto gb_interrupt_jump(GB* gb) -> void
// {
//   for (int i = 0; i < 5; ++i)
//   {
//     if (gb->cpu.get_intrr_flags(gb->mmu) & flags[i])
//     {
//       gb->cpu.pc = vectors[i];
//       gb->mmu.io[IF] = (gb->mmu.io[IF] & ~flags[i]) | 0xE0;
//       return;
//     }
//   }
//   // else { gb->cpu.pc = 0x0000; }
// }


static auto gb_interrupt_cycles(GB* gb) -> void
{
  // ---------- Determine initial pending interrupt (highest priority) ----------
  u8 IF_var = gb->mmu.io[IF];
  u8 IE_var = gb->cpu.IE;
  u8 pending = IE_var & IF_var;


  u8 selected_mask = 0;
  u8 selected_bit = 0;

  for (int i = 0; i < 5; i++)
  {
    if (pending & flags[i])
    {
      selected_mask = flags[i];
      selected_bit = i;
      break;
    }
  }


  // ---------- M1 ----------
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  // ---------- M2 ----------
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  // ---------- M3 ----------
  gb->cpu.sp--;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  // ---------- M4 ----------
  u8 pc_msb = (gb->cpu.pc >> 8) & 0xFF;
  bus_default_write(gb, gb->cpu.sp, pc_msb);

  u8 IE_after = gb->cpu.IE;
  u8 IF_after = gb->mmu.io[IF];
  u8 pending_after = IE_after & IF_after;

  bool cancel = false;
  if (pending_after == 0)
  {
    cancel = true;
  }
  else
  {
    selected_mask = 0;
    selected_bit = 0;
    for (int i = 0; i < 5; i++)
    {
      if (pending_after & flags[i])
      {
        selected_mask = flags[i];
        selected_bit = i;
        break;
      }
    }
  }

  gb->cpu.instruction_state = 5;
  gb->gb_step();
  gb->cpu.cycles_passed++;



  // ---------- M5 ----------
  gb->cpu.sp--;
  gb->cpu.ime = false;
  gb->cpu.ime_scheduled = false;

  if (cancel) {
    gb->cpu.pc = 0x0000;
    gb->cpu.instruction_state = 0;
    return;
  }

  u8 pc_lsb = gb->cpu.pc & 0xFF;
  bus_default_write(gb, gb->cpu.sp, pc_lsb);

  gb->cpu.pc = vectors[selected_bit];
  gb->mmu.io[IF] = (gb->mmu.io[IF] & ~selected_mask) | 0xE0;

  gb->cpu.instruction_state = 6;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  // ---------- M6/M0 ----------
  gb->cpu.instruction_state = 0;
  return;
}


/**
 * From Documentation :
 * https://gist.github.com/SonoSooS/c0055300670d678b5ae8433e20bea595#isr-and-nmi
 */
auto gb_interrupt_handler(GB* gb) -> void
{

  /* Launches the interrupt if true, or else, tests for opcode, or returns doing nothing */
  // TODO: Some weird STOP edgecase to implement which jumps to 0x00 (CGB)
  if ((gb->cpu.get_intrr_flags(gb->mmu) != 0)    &&
      (gb->cpu.ime == 1))
  {
    gb_interrupt_cycles(gb);
  }


  return;
}

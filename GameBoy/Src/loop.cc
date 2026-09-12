#include "gb.h"
#include "mmu.h"
#include "opcodes.h"
#include "interrupts.h"
#include "ttd.h"

// #include "../../deps/magic_enum/magic_enum.hpp"

f_inline
static auto gb_fetch_opcode(GB* gb) -> void
{
  /* fetch opcode & 0xCB instruction */
  if (gb->cpu.instruction_state == 0)
  {
    /* EI delay */
    if (gb->cpu.ime_scheduled > 0)
    {
      gb->cpu.ime_scheduled--;
      if (gb->cpu.ime_scheduled == 0)
        gb->cpu.ime = true;
    }

    /* Read the new opcode */
    gb->cpu.IR = bus_read(gb, gb->cpu.pc);

    gb->cpu.pc++;

    // gb::print("PC: {:04x} SYSCLK: {:04x} LY: {:02x}\n", gb->cpu.pc, gb->timer.SYSCLK, gb->mmu.io[LY]);

    if (gb->cpu.halt_bug)
    {
      gb->cpu.halt_bug = false;
      gb->cpu.pc--;
    }

    gb->cpu.instruction_state = 1;

    if (gb->cpu.IR == 0xCB)
      gb->cpu.cb_prefix = true;
    else
      gb->cpu.cb_prefix = false;

    gb->gb_step();
    gb->cpu.cycles_passed++;

    // fmt::print("AF: {:04x} BC: {:04x} DE: {:04x} HL: {:04x} SP: {:04x} PC: {:04x} opcode: {:02x} SYSCLK: {:04x} LY: {:02x} TIMA: {:02x}\n",
    //   //  dots: {}\n",
    //   gb->cpu.get_af(), gb->cpu.get_bc(), gb->cpu.get_de(), gb->cpu.get_hl(), gb->cpu.sp, gb->cpu.pc, gb->cpu.IR
    //   , gb->timer.SYSCLK
    //   , gb->mmu.io[LY], gb->mmu.io[TIMA]
    //   // , this->ppu_ptr->line_ticks
    //   );


  }

  /* fetch 0xCB instruction */
  if ((gb->cpu.instruction_state == 1) && (gb->cpu.IR == 0xCB))
  {
    gb->cpu.IR = bus_read(gb, gb->cpu.pc);
    gb->cpu.pc++;
    gb->cpu.instruction_state = 2;
    gb->gb_step();
    gb->cpu.cycles_passed++;
  }

  return;
}

f_inline auto gb_execute_opcode(GB* gb) -> void
{
  if (gb->cpu.cb_prefix == false)
  {
    cpu_optable[gb->cpu.IR](gb);
  }
  else
  {
    cpu_0xCB_optable[gb->cpu.IR](gb);
    gb->cpu.cb_prefix = 0;
  }
}


/* main gameboy loop, the cpu controls the entire gameboy */
auto gb_loop(GB* gb) -> u8
{
  gb->cpu.cycles_passed = 0;


  /* fetch opcode */
  gb_fetch_opcode(gb);

  /* execute opcode */
  gb_execute_opcode(gb);


  // /**
  //  * Highly Advanced logger
  //  */
  // if (gb->ttd.enabled)
  //   ttd_store_snapshot(gb, &(gb->ttd), gb->ttd.number_of_writes_occured);


  /**
   * handle interrupts if true; takes 5 M cycles.
   * when it wakes up from HALT mode, it does a fetch (with increment)
   * And that new value of pc+1 is pushed instead of the one with HALT opcode
   */
  gb_interrupt_handler(gb);


  return gb->cpu.cycles_passed;
}

#include "gb.h"
#include "ppu.h"
#include "mmu.h"
#include "interrupts.h"


/* Columns: [Normal, Double Speed] */
static constexpr u8 tima_bits[4][2] = {
  {9, 8}, {3, 2}, {5, 4}, {7, 6}
};

static constexpr u8 timer_is_doublespeed[2] = {
  8, 7
};


// f_inline
// static auto gb_apu_step(GB* gb) -> void
// {

// }

f_inline
static auto ppu_change_data_in_oam_mode(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;

  ppu.fifo.first_fetch = true;
  ppu.fifo.fetcher_x = 0;
  ppu.fifo.bg_fifo.fifo_clear();
  ppu.fifo.sp_fifo.fifo_clear();

  ppu.LX = 0;

  ppu.fifo.scx_fine_scroll = gb->mmu.io[SCX] & 7;

  ppu.fetch_source = FetchSource::background;

  ppu.fifo.sprite_fetch_index = 0;

  // ppu.latched_scx = mmu.io[SCX];
  // ppu.latched_scy = mmu.io[SCY];

  for (u8 i = 0; i < 10; i++) {
    ppu.oam_data.consumed[i] = false; }

  if ((mmu.io[WY] == mmu.io[LY]) &&
      (ppu.get_LCDC_bit(LCDC_bit::window_enable, mmu)))
    ppu.wy_triggered = true;

  return;
}

f_inline
static auto ppu_change_data_in_hblank_mode(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;

  if(ppu.window_triggered)
  {
    ppu.window_triggered = false;
    ppu.window_line_counter++;
  }

  // check again in hblank mode. ASHIEPAWS/fairylake.gb depends on this
  if ((mmu.io[WY] == mmu.io[LY]) &&
      (ppu.get_LCDC_bit(LCDC_bit::window_enable, mmu)))
    ppu.wy_triggered = true;

  ppu.oam_data.is_OAM_scan_active = true;
  ppu.oam_data.total_sprites = 0;
  ppu.oam_data.current_oam_sprite = 0;

  for (u8 i = 0; i < 10; ++i) {
    ppu.oam_data.oam_index[i] = 0; }

  return;
}


f_inline
static auto ppu_change_data_in_vblank_mode(GB* gb) -> void
{
  auto& ppu = gb->ppu;

  gb_set_IF_interrupt(gb->mmu.io, INTR::VBLANK);

  ppu.wy_triggered = false;
  ppu.window_line_counter = 0;
}


f_inline
static auto ppu_update_modes(GB* gb) -> void
{
  /* if PPU is turned off, return */
  u8 lcdc = bus_default_read(gb, 0xFF40);
  if ((lcdc & 0x80) == 0)
  {
    gb->ppu.dots = 0;
    gb->mmu.io[LY] = 0;
    gb->ppu.set_mode_STAT(SCANLINE_MODES::HBLANK, gb->mmu);
    return;
  }

  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;


  for (u8 i = 0; i < 4; i++)
  {

    u8 current_ly = mmu.io[LY];

    // VBLANK
    if (current_ly >= 144)
      ppu.set_mode_STAT(SCANLINE_MODES::VBLANK, mmu);
    else
    {
      if (ppu.dots < 80) // OAM
        ppu.set_mode_STAT(SCANLINE_MODES::OAM, gb->mmu);

      else if (ppu.LX < 160) // DRAWING
        ppu.set_mode_STAT(SCANLINE_MODES::DRAWING, mmu);

      else // HBLANK
        ppu.set_mode_STAT(SCANLINE_MODES::HBLANK, mmu);
    }

    SCANLINE_MODES ppu_mode = ppu.get_mode_STAT(mmu);

    switch(ppu_mode)
    {
      case SCANLINE_MODES::OAM:

        // run oam scan mode once per even lines
        if ((ppu.dots & 1) == 0)
          ppu_OAM_scan_mode(gb);

        if (ppu.dots == 79)
          ppu_change_data_in_oam_mode(gb);

        break;

      case SCANLINE_MODES::DRAWING:
        ppu_drawing_mode(gb);
        break;

      case SCANLINE_MODES::HBLANK:
        if (ppu.dots == 455)
          ppu_change_data_in_hblank_mode(gb);
        break;

      case SCANLINE_MODES::VBLANK:
        if ((current_ly == 144) && (ppu.dots == 0))
          ppu_change_data_in_vblank_mode(gb);

        break;
    }

    // gb::print("opcode: {:02x}, PC: {:02x} DIV: {:02x}, ticks = {:03}, dots = {:03} LY = {:03} mode: {}\n",
    //  gb->cpu.IR, gb->cpu.pc, gb->mmu.io[DIV], gb->ticks,
    //   ppu.dots, gb->mmu.io[LY], magic_enum::enum_name(ppu_mode));


    ppu.dots++;

    const u16 target_ticks = gb->ppu.first_scanline ? 452 : 456;

    if (ppu.dots == target_ticks)
    {
      ppu.first_scanline = false;
      ppu.dots = 0;
      mmu.io[LY]++;

      // Frame Rollover
      if (mmu.io[LY] >= 154)
        mmu.io[LY] = 0;
    }

    // ppu interrupts check
    if (mmu.io[LY] == gb->mmu.io[LYC])
      mmu.io[STAT] |= 0x4;
    else
      mmu.io[STAT] &= ~0x4;

    ppu_stat_interrupt_service(gb);
  }
}


f_inline
static auto ppu_oam_dma_check(GB* gb) -> void
{
  /* OAM DMA copy switch */
  if (gb->ppu.oam_data.dma_wrote && gb->ppu.oam_data.dma_delay > 0) {
    gb->ppu.oam_data.dma_delay--; }

  if (gb->ppu.oam_data.dma_wrote && gb->ppu.oam_data.dma_delay == 0)
  {
    gb->ppu.oam_data.dma_wrote = false;
    gb->ppu.oam_data.is_active = true;
    gb->ppu.oam_data.current_cycle = 0;
    // logging
    // gb::print("DMA Started at current_cycle: {} CPU tick: {}\n", gb->ppu.oam_data.current_cycle, gb->ticks);
  }
  if (gb->ppu.oam_data.is_active) { ppu_oam_dma_transfer(gb); }
}


f_inline
static auto gb_ppu_step(GB* gb) -> void
{
  ppu_update_modes(gb);

  ppu_oam_dma_check(gb);
}



auto gb_update_tima(GB* gb) -> void
{
  /*
   * Falling edge dectector via simulating an AND gate.
   * Explanation still needs to be written
   */
  gb->timer.TAC_timer_enabled = (gb->mmu.io[TAC] & 0x04) >> 2;
  gb->timer.tima_bit_enabled = ((gb->timer.SYSCLK >> gb->timer.tima_bit) & 1);
  gb->timer.current_signal = gb->timer.TAC_timer_enabled & gb->timer.tima_bit_enabled;

  if ((gb->timer.current_signal == 0) && (gb->timer.previous_signal == 1))
  {
    if (gb->mmu.io[TIMA] == 0xFF)
    {
      gb->timer.tima_reload_state = 1;
    }
    gb->mmu.io[TIMA]++;

  }

  gb->timer.previous_signal = gb->timer.current_signal;
}

f_inline
static auto gb_timer_step(GB* gb) -> void
{
  // 0xFF04 DIV
  gb->timer.SYSCLK += 4;

  gb->mmu.io[DIV] =
    ((gb->timer.SYSCLK >> timer_is_doublespeed[gb->timer.is_doublespeed]) & 0xFF);


  // 0xFF05 TIMA
  u8 clock_select = gb->mmu.io[TAC] & 0x3;

  gb->timer.tima_bit = tima_bits[clock_select][gb->timer.is_doublespeed];

  if (gb->timer.tima_reload_state == 1)
  {
    gb->mmu.io[TIMA] = gb->mmu.io[TMA];
    // enable timer jump!
    gb_set_IF_interrupt(gb->mmu.io, INTR::TIMER);
    gb->timer.tima_reload_state = 2;
    return;
  }

  else if (gb->timer.tima_reload_state == 2)
  {
    gb->timer.tima_reload_state = 0;
  }


  gb_update_tima(gb);
}


/* steps the gameboy every four ticks */
auto GB::gb_step() -> void
{
  ticks += 4;
  gb_ppu_step(this);
  gb_timer_step(this);
  // gb_apu_step(this);
}

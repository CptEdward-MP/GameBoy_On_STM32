#include "gb.h"
#include "mbc.h"
#include "loop.h"
#include "ttd.h"

struct Region
{
  u8* base;
  u16 offset;
  u8 (*read)(GB* gb, u16);
};

/* page table for faster access */
static Region raw_map[0x100];

static constexpr uint8_t io_read_dmgABCmgb_masks[0x80] =
{
  0xCF, 0x00, 0x7E, 0xFF, 0x00, 0x00, 0x00, 0xF8,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, // FF00
  0x80, 0x3F, 0x00, 0xFF, 0xBF, 0xFF, 0x3F, 0x00,
  0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF, // FF10
  0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // FF20
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // FF30
  0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, // FF40
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // FF50
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // FF60
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // FF70
};


static f_inline auto read_FF00_helper(GB* gb) -> u8
{
  // is it dpad selection, or button selection, or both
  u8 joyp = gb->mmu.io[JOYP] & 0x30;
  u8 result = 0xF;

  u8 dpad = gb->joyp.joyp_dpad;

  // Prevent opposing directions.
  // Right (bit 0) wins over Left (bit 1).
  if (!(dpad & 0x01))
    dpad |= 0x02;

  // Up (bit 2) wins over Down (bit 3).
  if (!(dpad & 0x04))
    dpad |= 0x08;

  u8 dpad_mask   = ((joyp >> 4) & 1) * 0x0F;
  u8 button_mask = ((joyp >> 5) & 1) * 0x0F;

  result = (joyp | 0xC0)                 |
                 ((dpad                  | dpad_mask)   &
                  (gb->joyp.joyp_buttons | button_mask));

  return result;
}

static f_inline auto io_read(GB* gb, u16 address) -> u8
{
  switch(address)
  {
    case 0xFF00: return read_FF00_helper(gb);
    default: return gb->mmu.io[address - 0xFF00] | io_read_dmgABCmgb_masks[address - 0xFF00];
  }
}

static u8 read_mbc_helper(GB* gb, u16 address) { return mbc_readbyte(gb, address); }

static u8 read_FEXX_helper(GB* gb, u16 address)
{
  if (address >= 0xFE00 && address <= 0xFE9F) {
    return gb->mmu.oam[address - 0xFE00]; }

  else if (address >= 0xFEA0 && address <= 0xFEFF) {
    return 0xFF; }

  else if (address >= 0xFF00 && address <= 0xFF7F) {
    return io_read(gb, address); }

  else if (address >= 0xFF80 && address <= 0xFFFE) {
    return gb->mmu.hram[address - 0xFF80]; }

  /* assumed as returning IE from map 0xFFFF */
  else {
    return gb->cpu.IE; }
}

// called in initialization in core.cc
auto mmu_init_map(GB* gb) -> void
{
  for (u16 address = 0; address < 256; address++)
  {
    if      (address <= 0x7F)                      {
      raw_map[address] = { nullptr, 0, read_mbc_helper}; }

    else if (address >= 0x80 && address <= 0x9F) {
      raw_map[address] = { (u8*)gb->mmu.vram, 0x8000, nullptr }; }

    else if (address >= 0xA0 && address <= 0xBF) {
      raw_map[address] = { nullptr, 0, read_mbc_helper }; }

    else if (address >= 0xC0 && address <= 0xDF) {
      raw_map[address] = { (u8*)gb->mmu.wram, 0xC000, nullptr }; }

    else if (address >= 0xE0 && address <= 0xFD) {
      raw_map[address] = { (u8*)gb->mmu.wram, 0xE000, nullptr }; }

    else if (address >= 0xFE && address <= 0xFF) {
      raw_map[address] = { nullptr, 0, read_FEXX_helper}; }
  }
}

/**
 * 'default' bus read function. Used in vram,
 * and everywhere else in the emulator besides opcodes.
 * Doesn't simulate bus conflicts, doesn't have modes
 * Freely reads data from anywhere at any time.
 */
auto bus_default_read(GB* gb, u16 address) -> u8
{
  u8 shifted_address = address >> 8;
  auto& entry = raw_map[shifted_address];

  if (entry.base != nullptr)
  {
    return entry.base[address - entry.offset];
  }

  return entry.read(gb, address);
}

static f_inline auto dma_in_range(u16 address, u16 start, u16 end) -> bool
{
  return ((address >= start) && (address <= end));
}

static auto bus_dma_oam_read(GB* gb, u16 address) -> u8
{
  // OAM always returns 0xFF
  if (address >= 0xFE00 && address <= 0xFE9F)
    return 0xFF;

  if (address < 0xFE00)
  {
    u16 dma_source_base = gb->mmu.io[DMA_byte] << 8;

    // If bus conflicts, return 0xFF
    for (u8 i = 0; i < 5; i++)
    {
      u16 start = mem_map[i].start;
      u16 end = mem_map[i].end;
      if (dma_in_range(dma_source_base, start, end) && dma_in_range(address, start, end))
        return 0xFF;
    }
  }

  // if not, return normal reads. (includes HRAM too)
  return bus_default_read(gb, address);
}

/**
 * Main bus_read function, exposed to cpu.
 * It emulates boot rom as well as dma bus conflicts
 * Used in opcodes.
 */
auto bus_read(GB* gb, u16 address) -> u8
{

  u8 ppu_mode = static_cast<u8>(gb->ppu.get_mode_STAT(gb->mmu));

  /**
   * Allow access even during mode 3 if dot <= 84.
   * Because of how the CPU and PPU are executed, a write at dot == 80 would have occurred
   * on dot 78 (single-speed) or dot 79 (double-speed) on actual hardware and would not have
   * been blocked.
   * Allowing writes on dots 81-84 (probably 81-83 in actual hardware) is a hack to fix
   * what seems to be a timing issue elsewhere, possibly interrupt-related. The Stunt Race FX
   * demo depends on allowing these through
   */
  if ((ppu_mode == 3) &&
      (gb->ppu.dots >= 83) &&
      ((address >= 0x8000) && (address <= 0x9FFF)))
  {
    return 0xFF;
  }

  // // logging
  // gb::print("BUS READ: Addr: 0x{:04X}\n", address);

  // // boot rom
  // if (gb->cart.boot_enabled == true && address <= 0xFF)
  //  return gb->cart.boot_rom[address];

  if (gb->ppu.oam_data.is_active == true)
    return bus_dma_oam_read(gb, address);

  return bus_default_read(gb, address);
}

static f_inline auto io_write(GB* gb, u16 address, u8 byte) -> void
{
  switch(address)
  {
  case 0xFF00:
    gb->mmu.io[JOYP] = (gb->mmu.io[JOYP] & ~0x30) | (byte & 0x30);
    break;

  case 0xFF04:
    gb->timer.SYSCLK = 0;
    gb->mmu.io[DIV] = 0;
    break;

  case 0xFF05:
    if (gb->timer.tima_reload_state == 2) break;
    if (gb->timer.tima_reload_state == 1) gb->timer.tima_reload_state = 0;
    gb->mmu.io[TIMA] = byte;
    break;

  case 0xFF06:
    if (gb->timer.tima_reload_state == 2) gb->mmu.io[TIMA] = byte;
    gb->mmu.io[TMA] = byte;
    break;

  case 0xFF07:
    gb->mmu.io[TAC] = byte | 0xF8;
    gb_update_tima(gb);
    break;

  case 0xFF0F:
    gb->mmu.io[IF] = (byte & 0x1F) | 0xE0;
    break;

  case 0xFF40:
  {
    u8 old_LCDC = gb->mmu.io[LCDC];

    bool old_lcd_on = ((old_LCDC & 0x80) != 0);
    gb->mmu.io[LCDC] = byte;
    bool new_lcd_on =     ((byte & 0x80) != 0);

    if (old_lcd_on != new_lcd_on)
    {
      if (new_lcd_on) gb->ppu.first_scanline = true;

      gb->ppu.dots = 0;
      gb->mmu.io[LY] = 0;
      gb->ppu.set_mode_STAT(SCANLINE_MODES::OAM, gb->mmu);
    }
    break;
  }

  case 0xFF44: /* Read Only */ break;

  case 0xFF41:
    gb->mmu.io[0x41] = (byte & 0xF8) | (gb->mmu.io[0x41] & 0x7);
    break;

  case 0xFF46:
    gb->mmu.io[address - 0xFF00] = byte;
    gb->ppu.oam_data.dma_wrote = true;
    gb->ppu.oam_data.dma_delay = 2;
    // gb::print("OAM byte set: {:02X} ticks: {}\n", byte, gb->ticks);
    break;

  // case 0xFF50:
  //  if ((byte & 0x01) != 0)
  //    gb->cart.boot_enabled = false;
  //  break;

  default: gb->mmu.io[address - 0xFF00] = byte; break;
  }
}

/**
 * This function is used only for time travel debugger, as to avoid
 * -> locking up the system after being rewound
 * -> delocking but overflowing the stack via recursive calls
 *    -- bus_default_write -> ttd_restore_snapshot -> bus_default_write
 */
/* Enable in LTO builds */
// f_inline
auto bus_default_lockless_write(GB* gb, u16 address, u8 byte) -> void
{
  if      (address <= 0x7FFF)                      { mbc_writebyte(gb, address, byte); }
  else if (address >= 0x8000 && address <= 0x9FFF) { gb->mmu.vram[address - 0x8000] = byte; }
  else if (address >= 0xA000 && address <= 0xBFFF) { mbc_writebyte(gb, address, byte); }
  else if (address >= 0xC000 && address <= 0xDFFF) { gb->mmu.wram[address - 0xC000] = byte; }
  else if (address >= 0xE000 && address <= 0xFDFF) { gb->mmu.wram[address - 0xE000] = byte; }
  else if (address >= 0xFE00 && address <= 0xFE9F) { gb->mmu.oam[address - 0xFE00] = byte; }
  else if (address >= 0xFF00 && address <= 0xFF7F) { io_write(gb, address, byte); }
  else if (address >= 0xFF80 && address <= 0xFFFE) { gb->mmu.hram[address - 0xFF80] = byte; }
  else if (address == 0xFFFF) { gb->cpu.IE = byte; }
  else { return; }
}

/* Enable in LTO builds */
// f_inline
auto bus_default_write(GB* gb, u16 address, u8 byte) -> void
{
  /**
   * TTD Memory Logger
   */
  // if (gb->ttd.enabled)
  // {
  //   ttd_log_ram(gb, address, byte);
  // }

  bus_default_lockless_write(gb, address, byte);
}

auto bus_write(GB* gb, u16 address, u8 byte) -> void
{

  if (address >= 0x8000 && address <= 0x9FFF)
  {
    u8 ppu_mode = static_cast<u8>(gb->ppu.get_mode_STAT(gb->mmu));
    if (ppu_mode == 3) return;
  }


  if (address >= 0xFE00 && address <= 0xFE9F)
  {
    if (gb->ppu.oam_data.is_active) return;

    /* TODO: OAM memory corruption needs to be implemented */

    u8 ppu_mode = static_cast<u8>(gb->ppu.get_mode_STAT(gb->mmu));
    if ((ppu_mode == 2) || (ppu_mode == 3)) return;
  }

  else if (address >= 0xFEA0 && address <= 0xFEFF) { return; }

  bus_default_write(gb, address, byte);
}

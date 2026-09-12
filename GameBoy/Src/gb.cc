#include "general.h"
#include "mbc.h"
#include "mmu.h"
#include "gb.h"

typedef struct GB GB;


//===================================================//
extern "C" volatile uint32_t gb_debug_stage;
//===================================================//




extern "C" volatile uint32_t gb_debug_stage;

auto CART::init(GB* gb, const u8* source_data, u32 source_size) -> void
{
    gb_debug_stage = 1;

    allocate_rom_data(source_data, source_size);

    gb_debug_stage = 2;

    load_header_data();

    gb_debug_stage = 3;

    allocate_sram();

    gb_debug_stage = 4;

    mbc_determine_function(gb);

    gb_debug_stage = 5;

    mbc_initialize_mbc_functions(rom_t);

    gb_debug_stage = 6;

    mmu_init_map(gb);

    gb_debug_stage = 7;
}



//auto CART::init(GB* gb, const u8* source_data, u32 source_size) -> void
//// auto CART::init(GB* gb, const u8* source_data, u32 source_size, const u8* boot_data, u32 boot_size) -> void
//{
//  // allocate_boot_rom(boot_data, boot_size);
//  allocate_rom_data(source_data, source_size);
//  load_header_data();
//  allocate_sram();
//  mbc_determine_function(gb);
//  mbc_initialize_mbc_functions(rom_t); /* defined in mbc.cc */
//
//  mmu_init_map(gb);
//
//  gb::print("Name: {:s}\n", gb->cart.head.title);
//  gb::print("Rom Type: {}\n", gb->cart.head.cart_type);
//  gb::print("Number of banks: {}\n", gb->cart.mbc.max_rom_banks);
//  gb::print("Number of ram banks: {}\n", gb->cart.mbc.max_ram_banks);
//}


// auto CART::allocate_boot_rom(const u8* boot_data, u32 boot_size) -> void
// {
//  size_t element_count = boot_size + 1;

//  this->boot_rom = gb::make_array<u8>(element_count);

//  gb::copy(boot_data, boot_data + boot_size, this->boot_rom.get());

//  this->boot_rom[boot_size] = 0;
// }





auto CART::allocate_rom_data(
    const u8* source_data,
    u32 source_size
) -> void
{
    (void)source_size;

    this->rom_data_ptr = source_data;
}
//auto CART::allocate_rom_data(const u8* source_data, u32 source_size) -> void
//{
//  size_t               element_count = source_size + 1;
//  gb::unique_ptr<u8[]> data          = gb::make_array<u8>(element_count);
//
//  gb::copy(source_data,
//           source_data + source_size,
//           data.get());
//
//  data[source_size] = 0;
//
//  this->rom_data = std::move(data);
//
//}

auto CART::load_header_data() -> void
{
  for (u8 i = 0; i < 4; i++)  { head.entry[i] = rom_data[0x100+i]; }
  for (u8 i = 0; i < 48; i++) { head.logo[i]  = rom_data[0x104+i]; }
  for (u8 i = 0; i < 16; i++) { head.title[i] = rom_data[0x134+i]; }
  for (u8 i = 0; i < 4; i++)  { head.manufacturer_code[i] = rom_data[0x13F+i]; }
  head.cgb_flag = rom_data[0x143];

  if (rom_data[0x14B] == 0x33) {
    head.new_license_code = (rom_data[0x144] << 8) | rom_data[0x145];
    head.old_license_code = rom_data[0x14B]; }
  else { head.old_license_code = rom_data[0x14B]; }

  head.sgb_flag = rom_data[0x146];
  head.cart_type = rom_data[0x147];
  head.rom_size = rom_data[0x148];
  head.ram_size = rom_data[0x149];
  head.dest_code = rom_data[0x14A];
  head.rom_version = rom_data[0x14C];

  head.header_checksum = rom_data[0x14D];
  u8 checksum = 0;
  for (u16 addr = 0x134; addr <= 0x14C; ++addr) {
    checksum = checksum - rom_data[addr] - 1; }
  if ( checksum != head.header_checksum ) {
    gb::print("ERROR: Checksum doesn't match! %i - rom: %i - calculated\n",
      head.header_checksum, checksum); }

  head.global_checksum = (rom_data[0x14E] << 8 | rom_data[0x14F]);
  u16 global_checksum = 0;
  for (u32 i = 0; i < file_size; ++i) {
    if (i == 0x14E || i == 0x14F) { continue; }
    global_checksum += rom_data[i]; }
  if (head.global_checksum != global_checksum) {
    gb::print("ERROR: Global Checksum doesn't match! %i - rom: %i - calculated\n",
      head.global_checksum, global_checksum);  }

  //mbc.max_rom_banks = head.rom_size;
  switch(head.rom_size)
  {
    case 0x00: mbc.max_rom_banks = 2 - 1;   break;
    case 0x01: mbc.max_rom_banks = 4 - 1;   break;
    case 0x02: mbc.max_rom_banks = 8 - 1;   break;
    case 0x03: mbc.max_rom_banks = 16 - 1;  break;
    case 0x04: mbc.max_rom_banks = 32 - 1;  break;
    case 0x05: mbc.max_rom_banks = 64 - 1;  break;
    case 0x06: mbc.max_rom_banks = 128 - 1; break;
    case 0x07: mbc.max_rom_banks = 256 - 1; break;
    case 0x08: mbc.max_rom_banks = 512 - 1; break;
    case 0x52: mbc.max_rom_banks = 72 - 1;  break;
    case 0x53: mbc.max_rom_banks = 80 - 1;  break;
    case 0x54: mbc.max_rom_banks = 96 - 1;  break;
  }
}

auto CART::allocate_sram() -> void
{
  switch (head.ram_size) {
  case 0x00:
    sram_data = nullptr;
    mbc.max_ram_banks = 0;
    break;

  case 0x01:
    mbc.max_ram_banks = 1;
    sram_data = gb::unique_ptr<u8[]>(new u8[1024 * 2]);
    break;

  case 0x02:
    mbc.max_ram_banks = 1;
    sram_data = gb::unique_ptr<u8[]>(new u8[1024 * 8]);
    break;

  case 0x03:
    mbc.max_ram_banks = 4;
    sram_data = gb::unique_ptr<u8[]>(new u8[1024 * 32]);
    break;

  case 0x04:
    mbc.max_ram_banks = 16;
    sram_data = gb::unique_ptr<u8[]>(new u8[1024 * 128]);
    break;

  case 0x05:
    mbc.max_ram_banks = 8;
    sram_data = gb::unique_ptr<u8[]>(new u8[1024 * 64]);
    break;

  default:
    gb::print("ERROR: Failed to Allocate SRAM! Corrupted rom header!\n");
    sram_data = nullptr; }
}

auto CART::mbc_mbc1m_check() -> void
{
  rom_t.mbc_t = mbc1M;

  for (u8 i = 0; i < 48; i++)
  {
    if (rom_data[0x104+i] != rom_data[0x40104+i])
    {
      rom_t.mbc_t = mbc1;
      break;
    }
  }
}

static f_inline auto mbc1_size_check(GB* gb) -> void
{
  if (gb->cart.file_size >= 0x100000)
  {
    gb->cart.mbc.is_rom_1mb = true;
    if (gb->cart.file_size >= 0x200000)
      gb->cart.mbc.contains_ram_banks = false;
  }
}

auto CART::mbc_determine_function(GB* gb) -> void
{
  rom_t.mbc_t  = ROM_ONLY;
  rom_t.ram_t  = 0;
  rom_t.bat_t  = 0;
  rom_t.time_t = 0;

  switch(head.cart_type) {

  //NO_mbc
  case 0x00:
    rom_t.mbc_t = ROM_ONLY;
    break;
  //END OF ROM_ONLY

  //mbc1
  case 0x01:
    rom_t.mbc_t = mbc1;

    mbc1_size_check(gb);
    if (file_size >= MIN_ROM_SIZE_FOR_mbc1M) {
      mbc_mbc1m_check(); }
    break;
  case 0x02:
    rom_t.mbc_t = mbc1;
    rom_t.ram_t = 1;
    mbc1_size_check(gb);

    if (file_size >= MIN_ROM_SIZE_FOR_mbc1M) {
      mbc_mbc1m_check(); }
    break;
  case 0x03:
    rom_t.mbc_t = mbc1;
    rom_t.bat_t = 1;
    rom_t.ram_t = 1;

    mbc1_size_check(gb);

    if (file_size >= MIN_ROM_SIZE_FOR_mbc1M) {
      mbc_mbc1m_check(); }
    break;
  //END OF mbc1

  case 0x04:
    gb::print("ERROR! Bad header! No such mbc exists!\n"); break;

  //mbc2
  case 0x05:
    rom_t.mbc_t = mbc2;
    break;
  case 0x06:
    rom_t.mbc_t = mbc2;
    rom_t.bat_t = 1;
    break;
  //END OF mbc2

  // mbc3
  case 0x0F:
    rom_t.mbc_t = mbc3;
    rom_t.time_t = 1;
    rom_t.bat_t = 1;
    break;
  case 0x10:
    rom_t.mbc_t = mbc3;
    rom_t.time_t = 1;
    rom_t.bat_t = 1;
    rom_t.ram_t = 1;
    break;
  case 0x11:
    rom_t.mbc_t = mbc3;
    break;
  case 0x12:
    rom_t.mbc_t = mbc3;
    rom_t.ram_t = 1;
    break;
  case 0x13:
    rom_t.mbc_t = mbc3;
    rom_t.ram_t = 1;
    rom_t.bat_t = 1;
    break;
  }
}

static constexpr u8 boot_vram_tiles[400] = {
  0xF0, 0xF0, 0xFC, 0xFC, 0xFC, 0xFC, 0xF3, 0xF3,
  0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
  0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0xF3, 0xF3,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCF, 0xCF,
  0x00, 0x00, 0x0F, 0x0F, 0x3F, 0x3F, 0x0F, 0x0F,
  0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0x0F, 0x0F,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xF3,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0,
  0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF,
  0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xC3,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC,
  0xF3, 0xF3, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
  0x3C, 0x3C, 0xFC, 0xFC, 0xFC, 0xFC, 0x3C, 0x3C,
  0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3,
  0xF3, 0xF3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3,
  0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF,
  0x3C, 0x3C, 0x3F, 0x3F, 0x3C, 0x3C, 0x0F, 0x0F,
  0x3C, 0x3C, 0xFC, 0xFC, 0x00, 0x00, 0xFC, 0xFC,
  0xFC, 0xFC, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
  0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF0, 0xF0,
  0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF,
  0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xC3, 0xC3,
  0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0xFC, 0xFC,
  0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C
};

static constexpr u8 boot_vram_map[44] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x19, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18
};

auto GB::gb_init() -> void
{
  mmu.io[0x00] = 0xCF;
  mmu.io[0x02] = 0x7E;
  mmu.io[0x04] = 0xAB;
  mmu.io[0x07] = 0xF8;
  mmu.io[0x0F] = 0xE1;
  mmu.io[0x10] = 0x80;
  mmu.io[0x11] = 0xBF;
  mmu.io[0x12] = 0xF3;
  mmu.io[0x13] = 0xFF;
  mmu.io[0x14] = 0xBF;
  mmu.io[0x16] = 0x3F;
  mmu.io[0x17] = 0x00;
  mmu.io[0x18] = 0xFF;
  mmu.io[0x19] = 0xBF;
  mmu.io[0x1A] = 0x7F;
  mmu.io[0x1B] = 0xFF;
  mmu.io[0x1C] = 0x9F;
  mmu.io[0x1D] = 0xFF;
  mmu.io[0x1E] = 0xBF;
  mmu.io[0x20] = 0xFF;
  mmu.io[0x23] = 0xBF;
  mmu.io[0x24] = 0x77;
  mmu.io[0x25] = 0xF3;
  mmu.io[0x26] = 0xF1;
  mmu.io[0x40] = 0x91;
  mmu.io[0x41] = 0x85;
  mmu.io[0x46] = 0xFF;
  mmu.io[0x47] = 0xFC;

  timer.SYSCLK = 0xABCC;

  // timer.SYSCLK = 0;


  cpu.A = 0x01;
  cpu.B = 0x00;
  cpu.C = 0x13;
  cpu.D = 0x00;
  cpu.E = 0xD8;
  cpu.H = 0x01;
  cpu.L = 0x4D;
  cpu.F = (cart.head.header_checksum == 0x00) ? 0x80 : 0xB0;
  cpu.pc = 0x100;
  cpu.sp = 0xFFFE;

  // ppu.dots = 132;

  // copy gameboy tilemap and tiledata
  u8 *dst = mmu.vram + static_cast<uintptr_t>(0x0010);
  for (size_t i = 0; i < sizeof(boot_vram_tiles); i++)
  {
    *dst++ = boot_vram_tiles[i];
    *dst++ = 0x00;
  }
  gb::memcpy(mmu.vram + 0x1904, boot_vram_map, sizeof(boot_vram_map));
}

auto GB::gb_reset() -> void
{
  mmu = MMU{};
  ppu = PPU{};
  cart.mbc = MBC{};
  ticks = 0;
  cpu = CPU{};
  timer = gb_timers{};
  // ttd = ttd_context{};
  joyp = JOYPAD{};
  hw_mode = hardware_mode{};

  gb_init();
}

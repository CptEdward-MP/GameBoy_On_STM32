#pragma once


#include "typedefs.h"
#include "general.h"

typedef struct GB GB;


extern "C" {
    extern volatile uint32_t gb_debug_stage;
}


/* Minimum size to check for mbc1M (0x40104 + 48 bytes) */
#define MIN_ROM_SIZE_FOR_mbc1M 0x40134
static constexpr u16 dmg_to_rgb555[4] = {
  0xFFFF, // 0: White      (R:31, G:31, B:31, A:1) -> 11111 11111 11111 1
  0xAD6B, // 1: Light Gray (R:21, G:21, B:21, A:1) -> 10101 10101 10101 1
  0x5295, // 2: Dark Gray  (R:10, G:10, B:10, A:1) -> 01010 01010 01010 1
  0x0001  // 3: Black      (R:0,  G:0,  B:0,  A:1) -> 00000 00000 00000 1
};

enum IO_data : u8
{
  JOYP            = 0x00,
  DIV             = 0x04,
  TIMA            = 0x05,
  TMA             = 0x06,
  TAC             = 0x07,
  IF              = 0x0F,
  LCDC            = 0x40,
  STAT            = 0x41,
  SCY             = 0x42,
  SCX             = 0x43,
  LY              = 0x44,        // readonly
  LYC             = 0x45,
  DMA_byte        = 0x46,
  BGP             = 0x47,
  OBP0            = 0x48,
  OBP1            = 0x49,
  BCPS            = 0x68,
  BCPD            = 0x69,
  OCPS            = 0x6A,
  OCPI            = 0x6B,
  WY              = 0x4A,
  WX              = 0x4B,
};

enum class hardware_mode
{
  DMG,
  CGB,
};

enum class OAM_byte : u8
{
  OAM_ypos = 0,
  OAM_xpos = 1,
  OAM_tile_no = 2,
  OAM_sp_flags = 3
};


enum class OAM_sp_flags : u8
{
  // OAM_spbit_ = 0,
  // OAM_spbit_ = 1,
  // OAM_spbit_ = 2,
  // OAM_spbit_ = 3,
  /* above are CGB specific */
  OAM_spbit_palette = 4,
  OAM_spbit_xflip = 5,
  OAM_spbit_yflip = 6,
  OAM_spbit_obj_bg = 7,
};


enum mbc_type : u8
{
  ROM_ONLY,
  mbc1,
  mbc1M,
  mbc2,
  mbc3,
  // mbc30,
  // mbc5,
  // MMM01,
  // TAMA5,
  // HUC3,
  // HUC1,
};


/*
 Documentation:
   http://iceboy.a-singer.de/doc/mem_patterns.html
   https://github.com/Gekkio/mooneye-gb/issues/39
 not sure if this is accurate but it is merged from mooneye
 and sparse reading of a-singer
 */
enum BUS_id : u8
{
  EXTERNAL_bus = 0,       // (0x0000->0x7FFF) cartridge area
  EXTERNAL_vram = 1,      // vram area
  EXTERNAL_bus_extra = 2, // includes cartridge banking+wram
  INTERNAL_vram = 3,      // OAM area
  INTERNAL_bus = 4,       // io, hram, and IE register
};

struct mem_region
{
  u16 start = 0;
  u16 end = 0;
  BUS_id id{};
};

constexpr mem_region mem_map[5]
{
  {0x0000, 0x7FFF, EXTERNAL_bus},
  {0x8000, 0x9FFF, EXTERNAL_vram},
  {0xA000, 0xFDFF, EXTERNAL_bus_extra},
  {0xFE00, 0xFE9F, INTERNAL_vram},
  {0xFF00, 0xFFFF, INTERNAL_bus},
};

enum class LCDC_bit : u8
{
  window_priority = 0,          // [Different meaning in CGB Mode]: 0 = Off; 1 = On
  obj_enable = 1,               // 0 = Off; 1 = On
  obj_size = 2,                 // 0 = 8×8; 1 = 8×16
  bg_TILEMAP = 3,               // 0 = 9800–9BFF; 1 = 9C00–9FFF
  unsigned_tiles = 4,           // 0 = 8800–97FF; 1 = 8000–8FFF
  window_enable = 5,            // 0 = Off; 1 = On
  window_TILEMAP = 6,           // 0 = 9800–9BFF; 1 = 9C00–9FFF
  ppu_enable = 7,               // 0 = Off; 1 = On
};

enum class STAT_bit : u8
{
  /* 00: HBlank, 01: VBlank, 10: OAM Search, 11: Drawing, READONLY */
  mode_bit0 = 0,
  mode_bit1 = 1,
  LYC_LY = 2,

  interrupt_mode0_hblank = 3,
  interrupt_mode1_vblank = 4,
  interrupt_mode2_oamsearch = 5,
  interruptLYC_LY = 6,

  unused = 7
};

enum class SCANLINE_MODES : u8
{
  HBLANK = 0,
  VBLANK = 1,
  OAM = 2,
  DRAWING = 3
};



/* USED ONCE IN CART */
struct rom_type
{
  mbc_type mbc_t{};
  u8 ram_t = 0;
  u8 bat_t = 0;
  u8 time_t = 0;
};


/*USED ONCE IN MBC */
struct LATCH
{
  u64 latch_ticks = 0;

  bool latched = false;

  u8 sec = 0;
  u8 min = 0;
  u8 hour = 0;
  u8 day_low = 0;
  u8 rtc_dh = 0;

  f_inline auto get_dayvalue() -> u16 { return (((rtc_dh & 0x01) << 8) | day_low); }
  f_inline auto get_daycarry() -> u8 { return (rtc_dh & 0x80); }
  f_inline auto get_rtc_halt() -> u8 { return (rtc_dh & 0x40); }

  f_inline auto set_dayvalue(u16 day_full) -> void
  {
    if (day_full >= 0x200)
    {
      rtc_dh |= 0x80;
      day_full %= 0x200;
    }
    day_low = static_cast<u8>(day_full);

    rtc_dh &= 0xFE;
    rtc_dh |= ((day_full & 0x100) >> 8);
  }
};

/* USED ONCE IN CART */
struct MBC
{
  LATCH rtc;
  u16 max_rom_banks = 0;
  u8 max_ram_banks = 0;

  u8 ram_enable = 0;

  u8 rom_bank = 0;
  u8 ram_bank = 0;

  bool banking_mode = false;
  bool is_rom_1mb = false;
  bool contains_ram_banks = false;

  u8 five_bits = 0;
  u8 two_bits = 0;
};


/* USED ONCE IN CART */
struct rom_header
{
  u8 logo[48] = {0};
  char title[16] = {0};
  char manufacturer_code[4] = {0};
  u8 entry[4] = {0};
  u16 new_license_code = 0;
  u16 global_checksum = 0;
  u8 sgb_flag = 0;
  u8 cart_type = 0;
  u8 cgb_flag = 0;
  u8 rom_size = 0;
  u8 ram_size = 0;
  u8 dest_code = 0;
  u8 old_license_code = 0;
  u8 rom_version = 0;
  u8 header_checksum = 0;
};


class CART
{
public:

  rom_header head;
  gb::unique_ptr<const u8[]> rom_data;
  gb::unique_ptr<u8[]> sram_data;

  gb::unique_ptr<u8[]> boot_rom;


  const u8* rom_data_ptr = nullptr;



  MBC mbc;
  rom_type rom_t;
  u32 file_size = 0;

  volatile uint32_t debug_stage = 0;
  // // initially true, turned off later via 0xFF50
  // bool boot_enabled = true;

  // auto init(GB* gb, const u8* source_data, u32 source_size, const u8* boot_data, u32 boot_size) -> void;
  auto init(GB* gb, const u8* source_data, u32 source_size) -> void;

private:
  // auto allocate_boot_rom(const u8* boot_data, u32 boot_size) -> void;
  auto allocate_rom_data(const u8* source_data, u32 source_size) -> void;
  auto load_header_data() -> void;
  auto allocate_sram() -> void;
  auto mbc_mbc1m_check() -> void;
  auto mbc_determine_function(GB* gb) -> void;
};


struct MMU
{
  u16 gb_framebuffer[23040] = {0};
  u8  vram[0x2000] = {0};
  u8  wram[0x2000] = {0};
  u8  oam[0xA0] = {0};
  u8  io[0x80] = {0};
  u8  hram[0x7F] = {0};
};



// -------------------- THIS SECTION IS DEDICATED ENTIRELY TO THE PPU -------------------- //
/* initial fetches take 6 dots, and normal fetches take about 8 dots */
enum class FIFO_state : u8
{
  gettileno_t1 = 0,
  gettileno_t2 = 1,

  fetch_bgtiledatalow_t1 = 2,
  fetch_bgtiledatalow_t2 = 3,

  fetch_bgtiledatahigh_t1 = 4,
  fetch_bgtiledatahigh_t2 = 5,

  pushtofifo = 6
};

f_inline FIFO_state operator++(FIFO_state& s, int)
{
  FIFO_state temp = s;
  s = static_cast<FIFO_state>(static_cast<u8>(s) + 1);
  return temp;
}



struct pixel_info
{
  u8 colour = 0;
  u8 palette = 0;
  u8 priority = 0;
  bool bg_priority = 0;
};

// code referred from SameBoy/Core/display.c & display.h
struct bg_pixel_fifo
{
  u8 colour[8] = {0};
  u8 palette[8] = {0};
  u8 priority[8] = {0};
  u8 bg_priority = 0; // has 8 bools!

  u8 read_ptr = 0;
  u8 total_size = 0;

  f_inline void fifo_clear()
  {
    read_ptr = total_size = 0;
    for (u8 i = 0; i < 8; ++i)
    {
      colour[i] = {0};
      palette[i] = {0};
      priority[i] = {0};
      bg_priority = 0;
    }
  }

  f_inline pixel_info fifo_pop() {
    gb_assert(total_size > 0);
    pixel_info popped_pixel = {
      colour[read_ptr],
      palette[read_ptr],
      priority[read_ptr],
      static_cast<bool>((bg_priority >> read_ptr) & 1),
    };
    read_ptr = (read_ptr + 1) & 7;
    total_size--;
    return popped_pixel;
  }

  f_inline void fifo_push(u8 lower, u8 upper, u8 pal, bool prio, bool flip_x) {
    gb_assert(total_size == 0);
    /*
     * ex upper = 0b10110010
     *    lower = 0b00100111
     *    final = 0b[10][00][11][10][00][01][11][01]
     * the lower and upper keep on shifting << 1
     * because the process begins at:
     * 0b10110010
     *   ^
     *   |--  at i == 0
     */
    if (!flip_x)
    {
      for (u8 i = 0; i < 8; ++i)
      {
        colour[i] = ((upper >> 7) << 1) | (lower >> 7);
        palette[i] = pal;
        priority[i] = false;
        bg_priority |= (static_cast<u8>(prio) << i);

        lower <<= 1;
        upper <<= 1;
        total_size++;
      }

    }
    /*
     *the lower and upper keep on shifting >> 1
     * because the process begins at:
     * 0b10110010
     *          ^
     *          |--  at i == 0
     */
    else
    {
      for (u8 i = 0; i < 8; i++)
      {
        colour[i] = (lower & 1) | ((upper & 1) << 1);
        palette[i] = pal;
        priority[i] = false;
        bg_priority |= (static_cast<u8>(prio) << i);
        lower >>= 1;
        upper >>= 1;
        total_size++;
      }
    }
  }

};

struct sp_pixel_fifo
{
  u8 colour[8] = {0};
  u8 palette[8] = {0};
  u8 obj_priority = 0; // has 8 bools!
  u8 occupied = 0;     // has 8 bools!

  // u8 priority[8] = {0};

  u8 read_ptr = 0;
  u8 total_size = 0;

  f_inline void fifo_clear()
  {
    read_ptr = total_size = 0;
    for (u8 i = 0; i < 8; ++i)
    {
      colour[i] = {0};
      palette[i] = {0};
    }
    obj_priority = 0;
  }

  f_inline pixel_info fifo_pop()
  {
    gb_assert(total_size > 0);

    bool occupied_slot = (occupied >> read_ptr) & 1;

    pixel_info popped_pixel{};

    if (occupied_slot)
    {
      popped_pixel.colour = colour[read_ptr];
      popped_pixel.palette = palette[read_ptr];
      popped_pixel.bg_priority = static_cast<bool>((obj_priority >> read_ptr) & 1);

      colour[read_ptr] = 0;
      palette[read_ptr] = 0;
      obj_priority &= ~(1 << read_ptr);

      occupied &= ~(1 << read_ptr);

      read_ptr = (read_ptr + 1) & 7;
      total_size--;
    }

    return popped_pixel;
  }

  f_inline void fifo_push(u8 lower, u8 upper, u8 sprite_x, u8 pal, bool prio, bool flip_x)
  {
    /* How many pixels to skip from the LHS side */
    u8 skip = (sprite_x < 8) ? (8 - sprite_x) : 0;

    if (!flip_x) {
      lower <<= skip;
      upper <<= skip;
    }
    else {
      lower >>= skip;
      upper >>= skip;
    }

    for (u8 i = skip; i < 8; ++i)
    {
      u8 pixel_colour = 0;

      if (!flip_x)
      {
        pixel_colour = ((upper >> 7) << 1) | (lower >> 7);
        upper <<= 1;
        lower <<= 1;
      }
      else
      {
        pixel_colour = (lower & 1) | ((upper & 1) << 1);
        upper >>= 1;
        lower >>= 1;
      }

      // u8 fifo_slot = i - skip;
      u8 fifo_slot = (read_ptr + i - skip) & 7;


      if (((occupied >> fifo_slot) & 1) && colour[fifo_slot] != 0)
        continue;

      colour[fifo_slot] = pixel_colour;
      palette[fifo_slot] = pal;
      occupied |= 1 << fifo_slot;
      obj_priority |= static_cast<u8>(prio) << fifo_slot;

      total_size++;
    }
  }

};

/**
 * Each access to VRAM (B, 0, 1, s) takes 2 cycles to occur.  A "cycle" is
 * exactly 1 period of the main input clock to the gameboy CPU chip.  This
 * is nominally 4.19MHz approximately.
 * This struct is a container all things needed to render fifo.
 * Including sprite_fifo, bg_fifo, fifo_state, and more
 */
struct generic_pixel_fifo
{
  sp_pixel_fifo sp_fifo{};            // sprite fifo line
  bg_pixel_fifo bg_fifo{};            // background & window fifo line

  /**
   * The follow data belongs to bg/window fifo
   */

  bool first_fetch = true;         // the first fetch in a scanline. Used to discard initial fetch

  u16 tiledata_address = 0;        // tiledata_address of the current bg/window tile
  FIFO_state state =
    FIFO_state::gettileno_t1;      // fetch state inside bg/window
  u8 fetcher_x = 0;                // emulator interal, not in the gameboy. Current X tile of bg/window
  u8 scx_fine_scroll = 0;          // second fetch of the first tile. pixels to discard

  u8 tiledata_low = 0;             // tiledata_low
  u8 tiledata_high = 0;            // tiledata_high


  /**
   * The follow data belongs to sprite fifo
   */

  FIFO_state sprite_state =
    FIFO_state::gettileno_t1;      // fetch state inside sprite

  u16 sprite_tiledata_address = 0; // tiledata_address, but for sprite
  u8 sprite_fetch_index = 0;       // what sprite is it pointing to in the oam mode3
  u8 sprite_tiledata_low = 0;
  u8 sprite_tiledata_high = 0;
};


/*
 * USED ONCE IN PPU
 * Contains all things needed for OAM.
 * Including,
 *    -> dma related contents
 *    -> oam sprite mode scan related content
 */
struct OAM_data
{
  u8 oam_index[10] = {0};         /* Sprite index number */
  bool consumed[10] = {0};        /* stores whether the current sprite is rendered or not */

  /* USED IN DMA TRANSFER */
  u8   current_cycle = 0;           // current cycle of oam transfer. Can go up to 160
  bool dma_wrote = 0;               // to check if DMA has been written into
  bool is_active = 0;               // to check if the 160 M transfer loop is active
  u8   dma_delay = 0;               // one cycle delay before transferring data to dma


  /* used for OAM scan mode */
  u8 total_sprites = 0;          // sprite pointing to the data inside the SPRITE:fifo-pipeline
  u8 current_oam_sprite = 0;      // sprite pointing to the data inside the oam ram

  // first mode of the first scanline is always oam scan, thus =true;
  bool is_OAM_scan_active = true; // internal check to make sure if OAM is active

  /**
   * used for sprite byte. sprite_number is the raw number of the current sprite.
   */
  f_inline auto get_sp_byte(MMU& mmu, u8 sprite_number, OAM_byte i_flags) -> u8 {
    return mmu.oam[ (sprite_number * 4) + static_cast<u8>(i_flags) ];
  }

  /**
   * used for sprite flags
   * Since it is used during fetch tile, for convenience's sake it uses oam_index array.
   */
  f_inline auto get_sp_flags(MMU& mmu, u8 sprite_number, OAM_sp_flags i_attr_flags) -> bool {
    u8 flags_byte = mmu.oam[ (oam_index[sprite_number] * 4) + 3 ];
    return ((flags_byte >> static_cast<u8>(i_attr_flags)) & 1);
  }
};

enum class FetchSource: u8
{
  background,
  window
};

struct PPU
{
  generic_pixel_fifo fifo;
  OAM_data oam_data;

  u16 dots = 0;
  u8  LX = 0;                   /* Counting the number of pixels in scanline.
                                   When it reaches 160, gb enters HBLANK mode */

  bool old_STAT_signal = false; // Used for STAT interrupts

  bool window_triggered = false;

  u8 latched_scx = 0;
  u8 latched_scy = 0;
  bool wy_triggered = false;

  u8 window_line_counter = 0;            // internal line counter for window. Increments per scanline

  FetchSource fetch_source =
    FetchSource::background;             // fetch source, whether tile is fetched from bg or window

  bool sprite_fifo_is_running = false;   // bool to check if sprite fetch is progressing

  u8 current_tile = 0;
  u8 current_tile_attributes = 0;

  bool first_scanline = false;

  f_inline auto set_mode_STAT(SCANLINE_MODES mode, MMU& mmu) -> void {
    mmu.io[STAT] &= 0xFC;
    mmu.io[STAT] |= (static_cast<u8>(mode) & 0x03); }

  f_inline auto get_mode_STAT(MMU& mmu) -> SCANLINE_MODES {
    return static_cast<SCANLINE_MODES>(mmu.io[STAT] & 0x3); }

  f_inline auto get_LCDC_bit(LCDC_bit bit, MMU& mmu) -> bool {
    return static_cast<bool>((mmu.io[LCDC] >> static_cast<u8>(bit)) & 1); }

  f_inline auto get_STAT_bit(STAT_bit bit, MMU& mmu) -> bool {
    return ((mmu.io[STAT] >> static_cast<u8>(bit)) & 1); }
};
// -------------------- THIS SECTION IS DEDICATED ENTIRELY TO THE PPU -------------------- //


struct CPU
{
  u16 sp = 0, pc = 0;
  u8 A = 0, B = 0, C = 0, D = 0, E = 0 , H = 0, L = 0;
  u8 F = 0;

  /* halt */
  bool halt = false;
  bool halt_bug = false;

  /* Interrupts */
  bool ime = false;

  // has three states, (0, 1, 2). 0=fire 1=almost_there 2=EI_just_set
  u8 ime_scheduled = 0;

  bool joypad_accessed = false;

  bool bypass_interrupt_check = false;

  /**
   * this is to set the internal interrupt_flags u8 which is then looped
   * through all the jump locations, and the first interrupt is picked.
   * (for example, the VBLANK interrupt comes before LCD interrupt,
   * therefore is serviced, and LCD interrupt is ignored and cleared.
   */
  u8 IE = 0;                     // 0xFFFF - Interrupt enable

  /*
   * state machine variables
   * for M-cycle accuracy
   */
  u8 instruction_state = 0;      /* What M cycle is the instruction currently on
                                   (exceptions being RET_flag and some more) */
  u8 cycles_passed = 0;          // number of cycles passed, used for clocks

  u8 IR = 0;                     // Instruction Register: What opcode is the CPU executing
  bool cb_prefix = false;        // Whether or not the opcode is CB prefixed or not

  f_inline auto get_z() -> u8 { return ((F & 0x80) >> 7); }
  f_inline auto get_n() -> u8 { return ((F & 0x40) >> 6); }
  f_inline auto get_h() -> u8 { return ((F & 0x20) >> 5); }
  f_inline auto get_c() -> u8 { return ((F & 0x10) >> 4); }

  f_inline auto get_af() -> u16 { return ((A << 8) | (F & 0xF0)); }
  f_inline auto get_bc() -> u16 { return ((B << 8) | C); }
  f_inline auto get_de() -> u16 { return ((D << 8) | E); }
  f_inline auto get_hl() -> u16 { return ((H << 8) | L); }

  f_inline auto set_af(u16 val) -> void { A = val >> 8; F = val & 0xF0; }
  f_inline auto set_bc(u16 val) -> void { B = val >> 8; C = val & 0xFF; }
  f_inline auto set_de(u16 val) -> void { D = val >> 8; E = val & 0xFF; }
  f_inline auto set_hl(u16 val) -> void { H = val >> 8; L = val & 0xFF; }

  f_inline auto set_z(bool value) -> void {
    F = (F & 0x70) | (value << 7); }

  f_inline auto set_n(bool value) -> void {
    F = (F & 0xB0) | (value << 6); }

  f_inline auto set_h(bool value) -> void {
    F = (F & 0xD0) | (value << 5); }

  f_inline auto set_c(bool value) -> void {
    F = (F & 0xE0) | (value << 4); }

  f_inline auto get_intrr_flags(MMU& mmu) -> u8 {
    return (IE & mmu.io[IF] & 0x1F); }
};


struct gb_timers
{
  u16 SYSCLK = 0;

  bool is_doublespeed = false;
  u8   tima_reload_state = 0;

  bool TAC_timer_enabled = false;
  bool tima_bit_enabled = false;
  bool current_signal = false;
  bool previous_signal = false;

  u8 tima_bit = 0;
};

struct JOYPAD
{
  u8 joyp_dpad = 0xF;
  u8 joyp_buttons = 0xF;
};



/**
 * ------------ THIS SECTION IS DEDICATED TO THE TIME TRAVEL DEBUGGER ------------
 */
// struct ttd_snapshot
// {
//   PPU ppu;
//   MBC mbc;
//   u64 ticks = 0;
//   CPU cpu;
//   gb_timers timer;
//   JOYPAD joyp;
// };

// struct ttd_memory
// {
//   u16 addr;
//   u8 old_byte;
//   u8 byte;
// };

// struct ttd_context
// {
//   gb::unique_ptr<ttd_snapshot[]> snapshots;
//   gb::unique_ptr<ttd_memory[]> memory;

//   size_t capacity = 0;
//   size_t head = 0;
//   size_t current = 0;

//   u8 number_of_writes_occured = 0;

//   bool is_rewound = false;
//   bool enabled = false;
//   bool has_data = false;

//   f_inline auto initialize_snapshot(size_t init_size) -> void
//   {
//     capacity = init_size;
//     snapshots = gb::make_array<ttd_snapshot>(capacity+1);
//     memory = gb::make_array<ttd_memory>(capacity+1);
//     return;
//   }
// };

/**
 * ------------ THIS SECTION IS DEDICATED TO THE TIME TRAVEL DEBUGGER ------------
 */

struct GB
{
  MMU mmu;
  PPU ppu;
  CART cart;
  u64 ticks = 0;
  CPU cpu;
  gb_timers timer;
  // ttd_context ttd;
  JOYPAD joyp;

  hardware_mode hw_mode;       // Dmg or CGB !! TODO: implement cgb stuff !!

  /* Defined in the file loop.cc */
  auto gb_step() -> void;
  auto gb_init() -> void;
  auto gb_reset() -> void;
};

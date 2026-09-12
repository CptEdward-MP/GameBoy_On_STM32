#include "gb.h"
#include "mmu.h"
#include "interrupts.h"

// #include "../../deps/magic_enum/magic_enum.hpp"

auto ppu_stat_interrupt_service(GB* gb) -> void
{
  bool previous_stat_signal = gb->ppu.old_STAT_signal;
  u8 STAT_byte = gb->mmu.io[STAT];

  u8 current_mode = STAT_byte & 0x03;
  bool ly_lyc_coincidence = (STAT_byte & (1 << 2)) != 0;

  bool ly_lyc_signal = (STAT_byte & (1 << 6)) &&  ly_lyc_coincidence;
  bool mode2_signal  = (STAT_byte & (1 << 5)) && (current_mode == 2);
  bool mode1_signal  = (STAT_byte & (1 << 4)) && (current_mode == 1);
  bool mode0_signal  = (STAT_byte & (1 << 3)) && (current_mode == 0);

  bool current_stat_signal = (ly_lyc_signal | mode0_signal | mode1_signal | mode2_signal);

  if ((previous_stat_signal == 0) && (current_stat_signal == 1))
    gb_set_IF_interrupt(gb->mmu.io, INTR::LCD);

  gb->ppu.old_STAT_signal = current_stat_signal;
  return;
}

auto ppu_oam_dma_transfer(GB* gb) -> void
{
  u8 oam_byte = gb->mmu.io[DMA_byte];

  u8 oam_current_cycles = gb->ppu.oam_data.current_cycle;

  if (oam_current_cycles < 160)
  {
    gb->mmu.oam[oam_current_cycles]
      = bus_default_read(gb, ((oam_byte << 8) + oam_current_cycles));

    oam_current_cycles++;

    gb->ppu.oam_data.current_cycle = oam_current_cycles;
  }

  else if (oam_current_cycles == 160)
  {
    oam_current_cycles = 0;
    gb->ppu.oam_data.current_cycle = oam_current_cycles;
    gb->ppu.oam_data.is_active = false;
  }
  return;
}

/**
 * OAM scan mode: scans ppu for 80 cycles to collect sprites based on specific parameters
 */
auto ppu_OAM_scan_mode(GB* gb) -> void
{

  if (gb->ppu.oam_data.is_OAM_scan_active == false)
    return;

  u8 total_sprites = gb->ppu.oam_data.total_sprites;
  u8 current_oam_sprite = gb->ppu.oam_data.current_oam_sprite;

  /* If the OAM already collected 10 sprites, then quit */
  if (current_oam_sprite >= 40 || total_sprites >= 10)
  {
    gb->ppu.oam_data.is_OAM_scan_active = false;
    gb->ppu.oam_data.total_sprites = total_sprites;
    gb->ppu.oam_data.current_oam_sprite = current_oam_sprite;

    return;
  }

  u8 sprite_height = (gb->ppu.get_LCDC_bit(LCDC_bit::obj_size, gb->mmu)) ? 16 : 8;

  u8 LY_byte = gb->mmu.io[LY];

  u8 sprite_x =
    gb->ppu.oam_data.get_sp_byte(gb->mmu, current_oam_sprite, OAM_byte::OAM_xpos);

  u8 sprite_y =
    gb->ppu.oam_data.get_sp_byte(gb->mmu, current_oam_sprite, OAM_byte::OAM_ypos);

  /**
   * Specific paramaters to collect sprites.
   * Source: https://github.com/Ashiepaws/GBEDG/blob/master/ppu/index.md#oam-scan-mode-2
   */
  if
  ((sprite_x > 0) &&
  ((LY_byte + 16) >= sprite_y) &&
  ((LY_byte + 16) < (static_cast<u16>(sprite_y) + sprite_height)) &&
   (total_sprites < 10))
  {
    gb->ppu.oam_data.oam_index[total_sprites] = current_oam_sprite;
    total_sprites++;
  }
  current_oam_sprite++;

  gb->ppu.oam_data.total_sprites = total_sprites;
  gb->ppu.oam_data.current_oam_sprite = current_oam_sprite;
  return;
}

static auto ppu_bg_fetch_tile(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;

  bool bg_map_select =
    (ppu.get_LCDC_bit(LCDC_bit::bg_TILEMAP, mmu) & 0x1);

  // extracts LCDC_bit::unsigned_tiles (its a bool despite being u8)
  bool unsigned_tiles =
    (ppu.get_LCDC_bit(LCDC_bit::unsigned_tiles, mmu) & 0x1);

  // adds to current_tile based on the signed tiles based on tiledata_area
  u16 bgtiledata_offset = bg_map_select ? 0x9C00 : 0x9800;

  u8 course_scroll_x = mmu.io[SCX] / 8;
  u16 tile_map_x = (ppu.fifo.fetcher_x + course_scroll_x) & 31;

  u8 bg_y = mmu.io[LY] + mmu.io[SCY];
  u16 tile_map_y = ( bg_y / 8 ) & 31;

  u16 tile_map_addr = (bgtiledata_offset) + (tile_map_y << 5) + (tile_map_x);
  u16 tile_number = bus_default_read(gb, tile_map_addr);

  // u16 attributes_map_addr = 0x2000 | tile_map_addr;
  // ppu.current_tile_attributes = bus_default_read(attributes_map_addr);
  // u32 bank_addr =

  u16 tile_data_addr = 0;
  u8 tile_row = bg_y & 7;


  tile_data_addr = unsigned_tiles
    ? 0x8000 + (static_cast<u16>(tile_number) * 16) + (tile_row * 2)
    : static_cast<u16>(0x9000 + (static_cast<int8_t>(tile_number) * 16) + (tile_row * 2));

  ppu.fifo.tiledata_address = tile_data_addr;

  return;
}

static auto ppu_window_fetch_tile(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;


  bool bg_map_select =
    (ppu.get_LCDC_bit(LCDC_bit::window_TILEMAP, mmu) & 0x1);

  // extracts LCDC_bit::unsigned_tiles (its a bool despite being u8)
  bool unsigned_tiles =
    (ppu.get_LCDC_bit(LCDC_bit::unsigned_tiles, mmu) & 0x1);

  // adds to current_tile based on the signed tiles based on tiledata_area
  u16 bgtiledata_offset = bg_map_select ? 0x9C00 : 0x9800;

  u16 tile_map_x = ppu.fifo.fetcher_x;
  u16 tile_map_y = ppu.window_line_counter / 8;

  u16 tile_map_addr = bgtiledata_offset + (tile_map_y << 5) + (tile_map_x);
  u8 tile_number = bus_default_read(gb, tile_map_addr);

  /*
   * TODO: Implement CGB and attributes
   */
  u8 tile_row = (ppu.window_line_counter & 7);

  u16 tile_data_addr = unsigned_tiles
    ? 0x8000 + (static_cast<u16>(tile_number) * 16) + (tile_row * 2)
    : static_cast<u16>(0x9000 + (static_cast<int8_t>(tile_number) * 16) + (tile_row * 2));

  ppu.fifo.tiledata_address = tile_data_addr;
  return;
}


static constexpr void (*ppu_gettile_func[2])(GB* gb) = {
  ppu_bg_fetch_tile, ppu_window_fetch_tile
};

static f_inline auto ppu_bg_fifo_emulator(GB* gb) -> void
{

  auto& ppu = gb->ppu;

  switch(ppu.fifo.state)
  {
    // 1
    case FIFO_state::gettileno_t1:
    {
      u8 fetch_source = static_cast<u8>(ppu.fetch_source);
      ppu_gettile_func[fetch_source](gb);

      ppu.fifo.state++;
      return;
    }
    // 2
    case FIFO_state::gettileno_t2:
    {

      ppu.fifo.state++;
      return;
    }
    // 3
    case FIFO_state::fetch_bgtiledatalow_t1:
    {
      ppu.fifo.tiledata_low = bus_default_read(gb, ppu.fifo.tiledata_address);
      ppu.fifo.state++;
      return;

    }
    // 4
    case FIFO_state::fetch_bgtiledatalow_t2:
    {
      gb->ppu.fifo.state++;
      return;
    }
    // 5
    case FIFO_state::fetch_bgtiledatahigh_t1:
    {
      ppu.fifo.tiledata_high = bus_default_read(gb, ppu.fifo.tiledata_address + 1);
      ppu.fifo.state++;
      return;
    }
    // 6
    case FIFO_state::fetch_bgtiledatahigh_t2:
    {
      gb->ppu.fifo.state++;
      return;
    }
    // 7
    case FIFO_state::pushtofifo:
    {
      // don't increment fetcher_x during the first fetch
      if (ppu.fifo.first_fetch == true)
      {
        if (ppu.fifo.bg_fifo.total_size > 0) return;

        ppu.fifo.bg_fifo.fifo_push(ppu.fifo.tiledata_low,
                                   ppu.fifo.tiledata_high, 0, 0, 0);

        ppu.fifo.state = FIFO_state::gettileno_t1;
        return;
      }

      if (ppu.fifo.bg_fifo.total_size > 0) return;

      /**
       * TODO: implement the window misaligned border bug
       * See: https://github.com/LIJI32/SameBoy/issues/278
       */

      ppu.fifo.bg_fifo.fifo_push(ppu.fifo.tiledata_low,
                                 ppu.fifo.tiledata_high, 0, 0, 0);



      ppu.fifo.fetcher_x++;

      /**
       * TODO: Implement CGB attributes
       */
      // ppu.fifo.bg_fifo.fifo_push( ppu.fifo.tiledata_low,
      //          ppu.fifo.tiledata_high,
      //          ppu.current_tile_attributes & 7,
      //          ppu.current_tile_attributes & 0x80,
      //          ppu.current_tile_attributes & 0x20 );

      ppu.fifo.state = FIFO_state::gettileno_t1;
      return;

    }
  }
}

static f_inline auto ppu_sprite_fetch_tile(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;
  auto& oam = ppu.oam_data;

  /**
   * Index of the sprite stored in oam_index.
   * Needed for oam_sp_byte function, as it does
   * raw oam array indexing
   */
  u8 sprite_index = oam.oam_index[ppu.fifo.sprite_fetch_index];

  u8 sprite_y = oam.get_sp_byte(mmu, sprite_index, OAM_byte::OAM_ypos);

  // u8 sprite_row = static_cast<u8>(mmu.io[LY] - (sprite_y + 16));
  u8 sprite_row = (mmu.io[LY] + 16) - sprite_y;

  u8 tile_number = oam.get_sp_byte(mmu, sprite_index, OAM_byte::OAM_tile_no);

  if (ppu.get_LCDC_bit(LCDC_bit::obj_size, mmu))
  {
    tile_number &= ~0x01;

    bool lower_tile =
      ((sprite_row & 0x08) != 0) ^
      oam.get_sp_flags(mmu, ppu.fifo.sprite_fetch_index, OAM_sp_flags::OAM_spbit_yflip);

    tile_number |= lower_tile;
  }

  bool vertical_flip =
    oam.get_sp_flags(mmu, ppu.fifo.sprite_fetch_index, OAM_sp_flags::OAM_spbit_yflip);

  u8 tile_row = vertical_flip
    ? 7 - (sprite_row & 0x07)
    : (sprite_row & 0x07);

  u16 tile_data_addr =
    0x8000 + (static_cast<u16>(tile_number) * 16) + (tile_row * 2);

  ppu.fifo.sprite_tiledata_address = tile_data_addr;

  return;
}


static f_inline auto ppu_sprite_fifo_emulator(GB* gb) -> void
{
  auto& ppu = gb->ppu;

  switch(ppu.fifo.sprite_state)
  {
    //1
    case FIFO_state::gettileno_t1:
    {
      ppu_sprite_fetch_tile(gb);
      ppu.fifo.sprite_state++;
      return;
    }
    //2
    case FIFO_state::gettileno_t2:
    {
      ppu.fifo.sprite_state++;
      return;
    }
    //3
    case FIFO_state::fetch_bgtiledatalow_t1:
    {
      ppu.fifo.sprite_tiledata_low =
        bus_default_read(gb, ppu.fifo.sprite_tiledata_address);
      ppu.fifo.sprite_state++;
      return;
    }
    //4
    case FIFO_state::fetch_bgtiledatalow_t2:
    {
      ppu.fifo.sprite_state++;
      return;
    }
    //5
    case FIFO_state::fetch_bgtiledatahigh_t1:
    {
      ppu.fifo.sprite_tiledata_high =
        bus_default_read(gb, ppu.fifo.sprite_tiledata_address + 1);
      ppu.fifo.sprite_state++;
      return;
    }
    //6
    case FIFO_state::fetch_bgtiledatahigh_t2:
    {
      ppu.fifo.sprite_state++;
      return;
    }
    //7
    case FIFO_state::pushtofifo:
    {
      u8 sprite_index = ppu.oam_data.oam_index[ppu.fifo.sprite_fetch_index];
      u8 sprite_x =
        ppu.oam_data.get_sp_byte(gb->mmu, sprite_index, OAM_byte::OAM_xpos);


      bool x_flip =
        ppu.oam_data.get_sp_flags(gb->mmu,
                                  ppu.fifo.sprite_fetch_index,
                                  OAM_sp_flags::OAM_spbit_xflip);
      bool priority =
        ppu.oam_data.get_sp_flags(gb->mmu,
                                  ppu.fifo.sprite_fetch_index,
                                  OAM_sp_flags::OAM_spbit_obj_bg);
      bool palette_bool =
        ppu.oam_data.get_sp_flags(gb->mmu,
                                  ppu.fifo.sprite_fetch_index,
                                  OAM_sp_flags::OAM_spbit_palette);
      u8 palette = static_cast<u8>(palette_bool);

      ppu.fifo.sp_fifo.fifo_push(ppu.fifo.sprite_tiledata_low,
                                 ppu.fifo.sprite_tiledata_high,
                                 sprite_x,
                                 palette, priority, x_flip);

      ppu.sprite_fifo_is_running = false;
      // ppu.fifo.sprite_fetch_index++;
      ppu.fifo.sprite_state = FIFO_state::gettileno_t1;
      return;
    }
  }
}

static f_inline auto ppu_sendto_framebuffer(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;

  u8 LX_byte = ppu.LX;
  u8 LY_byte = mmu.io[LY];
  u16 final_colour = 0;

  // never sends data to framebuffer if fifo is empty
  if (ppu.fifo.bg_fifo.total_size == 0) return;

  pixel_info bg_pixel = ppu.fifo.bg_fifo.fifo_pop();

  pixel_info sp_pixel{};

  // if it is the first fetch, pop and return
  if (ppu.fifo.first_fetch == true)
  {
    // once first fetch has been exchausted, refetch the same tile again
    if (ppu.fifo.bg_fifo.total_size == 0) ppu.fifo.first_fetch = false;
    return;
  }

  // discard mid scroll pixels
  if (ppu.fifo.scx_fine_scroll > 0)
  {
    ppu.fifo.scx_fine_scroll--;
    return;
  }

  /* If BG/Window is disabled in LCDC, change colour to white */
  if ((mmu.io[LCDC] & 1) == 0)
    bg_pixel.colour = 0;


  // pop sprite pixel
  if (ppu.fifo.sp_fifo.total_size > 0)
  {
    sp_pixel = ppu.fifo.sp_fifo.fifo_pop();
  }
  else
  {
    sp_pixel.colour = 0;
    sp_pixel.bg_priority = false;
    sp_pixel.palette = 0;
  }

  bool sprite_wins =
    (sp_pixel.colour != 0) && (!sp_pixel.bg_priority || bg_pixel.colour == 0);


  if(gb->hw_mode == hardware_mode::CGB)
  {
    /* TODO: implement cgb stuff */
  }
  else
  {
    if (!sprite_wins)
    {
      u8 bgp = mmu.io[BGP];
      u8 colour_shade = (bgp >> (bg_pixel.colour * 2)) & 0x03;
      final_colour = dmg_to_rgb555[colour_shade];
    }
    else
    {
      u8 obp = sp_pixel.palette ? mmu.io[OBP1] : mmu.io[OBP0];
      u8 colour_shade = (obp >> (sp_pixel.colour * 2)) & 0x03;
      final_colour = dmg_to_rgb555[colour_shade];
    }
  }

  mmu.gb_framebuffer[(LY_byte * 160) + LX_byte] = final_colour;

  ppu.LX++;

  return;
}

auto ppu_drawing_mode(GB* gb) -> void
{
  auto& ppu = gb->ppu;
  auto& mmu = gb->mmu;

  ppu_bg_fifo_emulator(gb);

  if (ppu.sprite_fifo_is_running)
    ppu_sprite_fifo_emulator(gb);

  /* check for window */
  if ((ppu.fetch_source == FetchSource::background) &&
      (ppu.wy_triggered) &&
      (ppu.get_LCDC_bit(LCDC_bit::window_enable, mmu)) &&
      (ppu.LX >= (mmu.io[WX] - 7)))
  {
    ppu.fetch_source = FetchSource::window;
    ppu.window_triggered = true;
    ppu.fifo.state = FIFO_state::gettileno_t1;
    ppu.fifo.scx_fine_scroll = 0;
    ppu.fifo.bg_fifo.fifo_clear();
    ppu.fifo.fetcher_x = 0;
  }

  /* check for sprite */
  for (u8 i = 0; i < 10; ++i)
  {
    u8 sprite_index = ppu.oam_data.oam_index[i];
    u8 sprite_x =
      ppu.oam_data.get_sp_byte(gb->mmu, sprite_index, OAM_byte::OAM_xpos);

    if ((i < ppu.oam_data.total_sprites) &&
        (!ppu.oam_data.consumed[i]) &&
        (ppu.get_LCDC_bit(LCDC_bit::obj_enable, mmu)) &&
        (sprite_x <= ppu.LX + 8))
    {
      ppu.oam_data.consumed[i] = true;
      ppu.fifo.sprite_fetch_index = i;
      ppu.sprite_fifo_is_running = true;
      break;
    }
  }

  if (!ppu.sprite_fifo_is_running)
    ppu_sendto_framebuffer(gb);

  return;
}

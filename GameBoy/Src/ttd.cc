// #include "gb.h"
// #include "mmu.h"


// static f_inline auto ttd_store_memory(GB* gb, ttd_context* c, u16 addr, u8 byte) -> void
// {
//   if (c->is_rewound == true)
//   {
//     c->current++;
//     c->number_of_writes_occured++;

//     if ((c->current + 1) == (c->head))
//     {
//       c->is_rewound = false;
//     }

//     return;
//   }

//   size_t head = c->head;

//   c->memory[head].old_byte = bus_default_read(gb, addr);
//   c->memory[head].byte = byte;
//   c->memory[head].addr = addr;

//   c->current = c->head;

//   c->head++;
//   c->number_of_writes_occured++;
// }

// auto ttd_store_snapshot(GB* gb, ttd_context* c, u8 opcodes_cycles) -> void
// {

//   for (u8 i = 0; i < opcodes_cycles; ++i)
//   {
//     if (c->is_rewound)
//     {
//       c->number_of_writes_occured--;
//       continue;
//     }


//     size_t head = c->current - i;

//     c->snapshots[head].ppu       = gb->ppu;
//     c->snapshots[head].mbc       = gb->cart.mbc;
//     c->snapshots[head].ticks     = gb->ticks;
//     c->snapshots[head].cpu       = gb->cpu;
//     c->snapshots[head].timer     = gb->timer;
//     c->snapshots[head].joyp      = gb->joyp;


//     c->number_of_writes_occured--;
//   }

//   return;
// }

// // auto ttd_restore_snapshot(GB* gb, ttd_context* c, size_t location) -> void
// // {

// //   if (location == c->current) return;

// //   gb->ppu       = c->snapshots[location].ppu;
// //   gb->cart.mbc  = c->snapshots[location].mbc;
// //   gb->ticks     = c->snapshots[location].ticks;
// //   gb->cpu       = c->snapshots[location].cpu;
// //   gb->timer     = c->snapshots[location].timer;
// //   gb->joyp      = c->snapshots[location].joyp;


// //   /**
// //    * Unroll memory
// //    */
// //   if (location < c->current)
// //   {
// //     for (size_t i = c->current; i > location; --i)
// //     {
// //       const auto& write_struct = c->memory[i - 1];
// //       bus_default_lockless_write(gb, write_struct.addr, write_struct.old_byte);
// //     }
// //   }
// //   else // location > c->current
// //   {
// //     for (size_t i = c->current + 1; i <= location; ++i)
// //     {
// //       auto& write_struct = c->memory[i - 1];
// //       bus_default_lockless_write(gb, write_struct.addr, write_struct.byte);
// //     }
// //   }

// //   c->current = location;


// //   return;
// // }

// auto ttd_log_ram(GB* gb, u16 address, u8 byte) -> void
// {
//   if (gb->ttd.head >= gb->ttd.capacity)
//   {
//     gb->ttd.enabled = false;
//   }
//   else
//   {
//     ttd_store_memory(gb, &(gb->ttd), address, byte);
//   }
// }

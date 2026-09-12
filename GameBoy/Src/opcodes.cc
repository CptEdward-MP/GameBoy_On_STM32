#include "gb.h"
#include "opcodes.h"
#include "mmu.h"

/*
 * The way helper functions can be interpreted via these notes here
 * r = 8 bit register, like A, B, C, D, E, H, L, and even F in some special cases
 * rr = 16 bit pairs, like bc de and hl af
 * mrr = memory location of pair rr
 * n = the very next byte ahead of the opcode
 * nn = the very next two bytes after the opcode
 * mnn = the memory location of the very next two bytes after the opcode.
 * b = bit, used in the 0xCB opcodes
 * These should help you interpret the function namings, although there are additional ones
 * like sp, meaning the stack pointer.
 * The functions should be read in this manner, OPCODE_whereToWrite_dataLocation
 * For example, LD_r_mrr means "write into register 'r' from the data in the bus_read(gb, rr)"
 */


static auto NOP(GB* gb) ->  void
{
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_r(GB* gb, u8* reg1, u8* reg2) ->  void
{
  (*reg1) = (*reg2);
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mrr_r(GB* gb, u8 reg1, u16 mem) ->  void
{
  bus_write(gb, mem, reg1);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mn_r(GB* gb, u8 reg) ->  void
{
  u16 temp_addr;
  u8 offset = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  temp_addr = 0xFF00 + offset;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, temp_addr, reg);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_mn(GB* gb, u8* reg) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_storage = bus_read(gb, (temp_data + 0xFF00));
  *reg = temp_storage;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mr_r(GB* gb, u8 mem_reg, u8 reg) ->  void
{
  u16 temp_addr;
  temp_addr = 0xFF00 + mem_reg;
  bus_write(gb, temp_addr, reg);

  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_mr(GB* gb, u8* reg, u8 mem_reg) ->  void
{
  u16 temp_addr;
  temp_addr = mem_reg + 0xFF00;
  *reg = bus_read(gb, temp_addr);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_mrr(GB* gb, u8* reg1, u16 mem) -> void
{
  *reg1 = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_rr_sps8(GB* gb, u8* reg1, u8* reg2) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  gb->cpu.set_z(0);
  gb->cpu.set_n(0);
  if (((gb->cpu.sp & 0x0F) + (temp_data & 0x0F) > 0x0F)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (((gb->cpu.sp & 0xFF) + (temp_data & 0xFF) > 0xFF)) {
    gb->cpu.set_c(1); }
  else {
    gb->cpu.set_c(0); }

  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  u16 temp_result = gb->cpu.sp + static_cast<s8>(temp_data);
  *reg1 = ((temp_result >> 8) & 0xFF);
  *reg2 = (temp_result & 0xFF);
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_sp_rr(GB* gb, u16 pair) ->  void
{
  gb->cpu.sp = pair;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_n(GB* gb, u8* reg) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  *reg = temp_data;
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_r_mnn(GB* gb, u8* reg) ->  void
{
  u16 temp_addr;
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_storage = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  temp_addr = ((temp_storage << 8) | temp_data);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_data = bus_read(gb, temp_addr);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  *reg = temp_data;
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_rr_nn(GB* gb, u8* reg1, u8* reg2) ->  void
{
  u8 low_byte = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 high_byte = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  *reg1 = high_byte;
  *reg2 = low_byte;
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mrr_n(GB* gb, u16 pair) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mnn_sp(GB* gb) ->  void
{
  u16 temp_addr;

  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_storage = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  temp_addr = (temp_storage << 8) | temp_data;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, temp_addr, (gb->cpu.sp & 0xFF));
  temp_addr++;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, temp_addr, ((gb->cpu.sp >> 8) & 0xFF));
  gb->cpu.instruction_state = 5;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_sp_nn(GB* gb) ->  void
{
  u16 temp_addr;

  u8 low_byte = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 high_byte = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;

  temp_addr = ((high_byte << 8) | low_byte);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.sp = temp_addr;
  gb->cpu.instruction_state = 0;
  return;
}

static auto LD_mnn_r(GB* gb, u8 reg) ->  void
{
  u16 temp_addr;

  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_storage = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  temp_addr = ((temp_storage << 8) | temp_data);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, temp_addr, reg);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto RLCA(GB* gb) ->  void
{
  u8 byte = gb->cpu.A;
  u8 msb = ((byte >> 7) & 0x1);

  byte = byte << 1;

  byte |= msb;
  gb->cpu.A = byte;

  /* flag manip */
  gb->cpu.set_z(0);
  gb->cpu.set_h(0);
  gb->cpu.set_n(0);
  gb->cpu.set_c(msb);

  gb->cpu.instruction_state = 0;
  return;
}

static auto RRCA(GB* gb) ->  void
{
  u8 byte = gb->cpu.A;
  u8 lsb = byte & 0x1;
  u8 new_msb = lsb << 7;
  byte = byte >> 1;
  byte |= new_msb;
  gb->cpu.A = byte;

  /* flag manip */
  gb->cpu.set_z(0);
  gb->cpu.set_h(0);
  gb->cpu.set_n(0);
  gb->cpu.set_c(lsb);

  gb->cpu.instruction_state = 0;
  return;
}

static auto RLA(GB* gb) ->  void
{
  u8 byte = gb->cpu.A;
  u8 msb = (byte & 0x80) >> 7;
  byte = byte << 1;
  byte |= gb->cpu.get_c();

  gb->cpu.A = byte;

  /* flag manip */
  gb->cpu.set_z(0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(msb);

  gb->cpu.instruction_state = 0;
  return;
}

static auto RRA(GB* gb) ->  void
{
  u8 byte = gb->cpu.A;
  u8 msb = (gb->cpu.get_c() << 7);
  u8 lsb = byte & 0x1;
  byte = byte >> 1;
  byte |= msb;
  gb->cpu.A = byte;

  /* flat manip */
  gb->cpu.set_z(0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(lsb);

  gb->cpu.instruction_state = 0;
  return;
}

static auto DAA(GB* gb) ->  void
{
  u8 offset = 0;
  u16 A = gb->cpu.A;
  bool set_carry = false;

  if (gb->cpu.get_n() == 0)
  {
    /* lower nibble adjuster */
    if (gb->cpu.get_h() || (A & 0xF) > 0x9) { offset |= 0x6; }
    /* upper nibble adjuster */
    if (gb->cpu.get_c() || (A > 0x99))
    {
      offset |= 0x60;
      set_carry = true;
    }
    A += offset;
  }
  else
  {
    if (gb->cpu.get_h()) { offset |= 0x06; }
    if (gb->cpu.get_c())
    {
      offset |= 0x60;
      set_carry = true;
    }
    A -= offset;
  }

  A = static_cast<u8>(A);
  gb->cpu.A = A;
  gb->cpu.set_z(A == 0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(set_carry);

  gb->cpu.instruction_state = 0;
  return;
}

static auto CPL(GB* gb) ->  void
{
  gb->cpu.A = ~gb->cpu.A;

  /* flag manip */
  gb->cpu.set_n(1);
  gb->cpu.set_h(1);

  gb->cpu.instruction_state = 0;
  return;
}

static auto SCF(GB* gb) ->  void
{
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(1);

  gb->cpu.instruction_state = 0;
  return;
}

static auto CCF(GB* gb) ->  void
{
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  bool c = !gb->cpu.get_c();
  gb->cpu.set_c(c);

  gb->cpu.instruction_state = 0;
  return;
}

//reference   H L B C
static auto ADD_rr_rr(GB* gb, u8* high_1, u8* low_1, u8* high_2, u8* low_2) -> void
{
  u8 a_low = *low_1;
  u8 b_low = *low_2;
  u16 temp_addr = a_low + b_low;
  *low_1 = static_cast<u8>(temp_addr);

  gb->cpu.set_n(0);

  gb->cpu.set_h( (((a_low & 0xF) + (b_low & 0xF)) > 0xF) ? 1 : 0);

  gb->cpu.set_c( (temp_addr > 0xFF) ? 1 : 0);

  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  u8 a_high = *high_1;
  u8 b_high = *high_2;
  temp_addr = a_high + b_high + gb->cpu.get_c();
  *high_1 = static_cast<u8>(temp_addr);

  gb->cpu.set_n(0);

  gb->cpu.set_h( (((a_high & 0xF) + (b_high & 0xF) + gb->cpu.get_c()) > 0xF) ? 1 : 0);

  gb->cpu.set_c( (temp_addr > 0xFF) ? 1 : 0);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADD_rr_sp(GB* gb, u8* high_1, u8* low_1, u16* sp) ->  void
{
  u8 a_low = *low_1;
  u8 b_low = static_cast<u8>(*sp & 0xFF);

  u16 temp_addr = a_low + b_low;
  *low_1 = static_cast<u8>(temp_addr);

  gb->cpu.set_n(0);

  gb->cpu.set_h( (((a_low & 0xF) + (b_low & 0xF)) > 0xF) ? 1 : 0);

  gb->cpu.set_c( (temp_addr > 0xFF) ? 1 : 0);

  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  u8 a_high = *high_1;
  u8 b_high = static_cast<u8>(*sp >> 8);
  temp_addr = a_high + b_high + gb->cpu.get_c();
  *high_1 = static_cast<u8>(temp_addr);

  gb->cpu.set_n(0);

  gb->cpu.set_h( (((a_high & 0xF) + (b_high & 0xF) + gb->cpu.get_c()) > 0xF) ? 1 : 0);

  gb->cpu.set_c( (temp_addr > 0xFF) ? 1 : 0);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADD_r_r(GB* gb, u8* reg1, u8* reg2) ->  void
{
  const u8 reg_1 = *reg1;
  const u8 reg_2 = *reg2;
  u16 ans = reg_1 + reg_2;

  /*flag*/
  if ((ans & 0xFF) == 0)
    gb->cpu.set_z(1);
  else
    gb->cpu.set_z(0);

  gb->cpu.set_n(0);

  if (((reg_1 & 0xF) + (reg_2 & 0xF)) > 0xF)
    gb->cpu.set_h(1);
  else
    gb->cpu.set_h(0);

  if (ans > 0xFF)
    gb->cpu.set_c(1);
  else
    gb->cpu.set_c(0);

  (*reg1) = static_cast<u8>(ans & 0xFF);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADD_r_mrr(GB* gb, u8* reg, u16 pair) ->  void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u16 temp_addr = (*reg) + temp_data;

  /*flag*/
  if ((temp_addr & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  if ((((*reg) & 0xF) + (temp_data & 0xF)) > 0xF) { gb->cpu.set_h(1); } else { gb->cpu.set_h(0); }
  if (temp_addr > 0xFF) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }

  (*reg) = static_cast<u8>(temp_addr & 0xFF);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADD_r_n(GB* gb, u8* reg) ->  void
{
  u8 d8;
  u16 temp_addr;

  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  temp_addr = (*reg) + temp_data;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  d8 = *reg;
  (*reg) = static_cast<u8>(temp_addr);
  /* flag */
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  if (((d8 & 0xF) + ((temp_data) & 0xF)) > 0xF) { gb->cpu.set_h(1); } else { gb->cpu.set_h(0); }
  if (temp_addr > 0xFF) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }
  gb->cpu.instruction_state = 0;
  return;
}

// TODO: Fix this opcode to pass blargg tests
// static auto ADD_sp_s8(GB* gb) -> void
// {
//  /* M1 */
//  u8 Z_reg = bus_read(gb, gb->cpu.pc);
//  gb->cpu.pc++;
//  gb->cpu.instruction_state = 2;
//  gb->gb_step();
//  gb->cpu.cycles_passed++;

//  /* M2 */
//  u8 sp_low = (gb->cpu.sp & 0x00FF);

//  u8 result = sp_low + Z_reg;

//  gb->cpu.set_z(0);
//  gb->cpu.set_n(0);

//  gb->cpu.set_h(((sp_low & 0x0F) + (Z_reg & 0x0F)) > 0x0F ? 1 : 0);
//  gb->cpu.set_c(((sp_low & 0xFF) + (Z_reg & 0xFF)) > 0xFF ? 1 : 0);

//  /**
//   * In Gekkio, this is done before flag calculations
//   * But doing so would break this emulator's flag calculation logic,
//   * therefore, it is done after the flag calculations, to ensure it works
//   * This shouldn't effect anything, as Z_reg are internal registers
//   * and the entire thing happens in a single M cycle
//   */
//  Z_reg = result;
//  bool Z_signed = static_cast<bool>((Z_reg >> 7) & 1);

//  gb->cpu.instruction_state = 3;
//  gb->gb_step();
//  gb->cpu.cycles_passed++;

//  /* M3 */
//  u8 sp_high = static_cast<u8>((gb->cpu.sp & 0xFF00) >> 8);

//  u8 adj = Z_signed ? 0xFF : 0x00;
//  result = static_cast<u8>(sp_high + adj + gb->cpu.get_c());
//  u8 W_reg = result;

//  gb->cpu.instruction_state = 4;
//  gb->gb_step();
//  gb->cpu.cycles_passed++;

//  /* M4 */
//  gb->cpu.sp = (static_cast<u16>(W_reg) << 8) | Z_reg;

//  gb->cpu.instruction_state = 0;
//  return;
// }


static auto ADD_sp_s8(GB* gb) -> void
{
  /* M1 */
  u8 byte_val = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  /* M2 */
  u16 old_sp = gb->cpu.sp;
  s16 offset = static_cast<s8>(byte_val);
  u16 new_sp = old_sp + offset;
  u8 sp_low = old_sp & 0xFF;

  gb->cpu.set_z(0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(((sp_low & 0x0F) + (byte_val & 0x0F)) > 0x0F ? 1 : 0);
  gb->cpu.set_c(((sp_low & 0xFF) + (byte_val & 0xFF)) > 0xFF ? 1 : 0);

  // u8 Z_reg = new_sp & 0xFF;
  // u8 W_reg = (new_sp >> 8) & 0xFF;

  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  /* Unused Z and W regs in this implementation, therefore an empty cycle */
  /* M3 */
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  /* M4 */
  gb->cpu.sp = new_sp;

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADC(GB* gb, u8* reg1, u8* reg2) ->  void
{
  const u8 reg_1 = *reg1;
  const u8 reg_2 = *reg2;
  u16 ans = reg_1 + reg_2+ gb->cpu.get_c();

  /* flag */
  if ((ans & 0xFF) == 0)
    gb->cpu.set_z(1);
  else
    gb->cpu.set_z(0);

  gb->cpu.set_n(0);

  if (((reg_1 & 0xF) + (reg_2 & 0xF) + gb->cpu.get_c()) > 0xF)
    gb->cpu.set_h(1);
  else
     gb->cpu.set_h(0);

  if (ans > 0xFF)
    gb->cpu.set_c(1);
  else
    gb->cpu.set_c(0);

  (*reg1) = static_cast<u8>(ans & 0xFF);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADC_r_mrr(GB* gb, u8* reg, u16 pair) ->  void
{
  /* HL mem data */
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  /* U16 data*/
  u16 temp_addr = (*reg) + temp_data + gb->cpu.get_c();

  /*flag*/
  gb->cpu.set_n(0);
  if ((temp_addr & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  if ((((*reg) & 0xF) + (temp_data & 0xF) + gb->cpu.get_c()) > 0xF) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (temp_addr > 0xFF) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }

  (*reg) = static_cast<u8>(temp_addr & 0xFF);

  gb->cpu.instruction_state = 0;
  return;
}

static auto ADC_r_n(GB* gb, u8* reg) ->  void
{
  /* U8 n data */
  u8 temp_data = bus_read(gb, gb->cpu.pc);

  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 old_value = *reg;
  u8 c = gb->cpu.get_c();
  u16 temp_addr = old_value + temp_data + c;

  /* flag */
  *reg = static_cast<u8>(temp_addr);

  if ((*reg) == 0)
    gb->cpu.set_z(1);
  else
    gb->cpu.set_z(0);

  gb->cpu.set_n(0);

  if (((old_value & 0xF) + ((temp_data) & 0xF) + c) > 0xF)
    gb->cpu.set_h(1);
  else
    gb->cpu.set_h(0);

  if (temp_addr > 0xFF)
    gb->cpu.set_c(1);
  else
    gb->cpu.set_c(0);

  (*reg) = static_cast<u8>(temp_addr);
  gb->cpu.instruction_state = 0;
  return;
}

static auto SUB_r(GB* gb, u8* reg) ->  void
{
  const u8 a_copy = gb->cpu.A;
  const u8 val = *reg;

  gb->cpu.A = a_copy - val;

  gb->cpu.set_z(gb->cpu.A == 0);

  gb->cpu.set_n(1);

  gb->cpu.set_h((a_copy & 0x0F) < (val & 0x0F));

  gb->cpu.set_c(a_copy < val);

  gb->cpu.instruction_state = 0;
  return;
}

static auto SUB_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  u16 temp_result = static_cast<u8>(gb->cpu.A - temp_data);

  if ((temp_result & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  gb->cpu.set_n(1);

  if ((gb->cpu.A & 0xF) < (temp_data & 0xF)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (gb->cpu.A < temp_data) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }

  gb->cpu.A = static_cast<u8>(temp_result);
  gb->cpu.instruction_state = 0;
  return;
}

static auto SUB_n(GB* gb) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u16 temp_result = static_cast<u8>(gb->cpu.A - temp_data);

  gb->cpu.set_n(1);
  if ((temp_result & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  if ((gb->cpu.A & 0xF) < (temp_data & 0xF)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (gb->cpu.A < temp_data) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }
  gb->cpu.A = static_cast<u8>(temp_result);

  gb->cpu.instruction_state = 0;
  return;
}

static auto SBC_r_r(GB* gb, u8* reg1, u8 reg2) ->  void
{
  const u8 val1 = *reg1;
  const u8 val2 = reg2;

  s32 result = val1 - val2 - gb->cpu.get_c();

  *reg1 = static_cast<u8>(result);

  gb->cpu.set_z((*reg1 == 0) ? 1 : 0);

  gb->cpu.set_n(1);

  gb->cpu.set_h((static_cast<s16>(val1 & 0x0F) - (val2 & 0x0F) - gb->cpu.get_c()) < 0);

  gb->cpu.set_c(result < 0);

  gb->cpu.instruction_state = 0;
  return;
}

static auto SBC_r_mrr(GB* gb, u8* reg1, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 reg_1 = (*reg1);
  bool c = gb->cpu.get_c();

  s32 temp_result = reg_1 - temp_data - c;
  u8 final_result = static_cast<u8>(temp_result);

  /* N flag */
  gb->cpu.set_n(1);
  /* zero flag  */
  gb->cpu.set_z(final_result == 0 ? 1 : 0);
  /* half carry */
  if (((reg_1 & 0xF) - (temp_data & 0xF) - c) < 0)  gb->cpu.set_h(1);
  else                                              gb->cpu.set_h(0);
  /* full carry */
  if (temp_result < 0)   gb->cpu.set_c(1);
  else                   gb->cpu.set_c(0);

  (*reg1) = temp_result;

  gb->cpu.instruction_state = 0;
  return;
}

static auto SBC_r_n(GB* gb, u8* reg) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  u16 temp_result = static_cast<u8>((*reg) - (temp_data + gb->cpu.get_c()));
  bool c = gb->cpu.get_c();
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  //zero flag
  if (static_cast<u8>(temp_result) == 0)
    gb->cpu.set_z(1);
  else
    gb->cpu.set_z(0);

  //N flag
  gb->cpu.set_n(1);

  //half carry
  if (((*reg) & 0xF) < ((temp_data & 0xF) + c))
    gb->cpu.set_h(1);
  else
    gb->cpu.set_h(0);

  //full carry
  if ((*reg) < (temp_data + c))
    gb->cpu.set_c(1);
  else
    gb->cpu.set_c(0);

  *reg = static_cast<u8>(temp_result);

  gb->cpu.instruction_state = 0;
  return;
}


/*
 * uninitialized variable in a constexpr function is a C++20 extension
 */
static /* constexpr */
auto INC_r(GB* gb, u8* reg) ->  void
{
  bool half_carry;
  ((*reg & 0x0F) == 0x0F) ? half_carry = 1 : half_carry = 0;

  *reg += 1;

  /* flag manip */
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(half_carry);

  gb->cpu.instruction_state = 0;
  return;
}

static auto INC_rr(GB* gb, u8* pair1, u8* pair2) ->  void
{
  u16 temp_addr = ((*pair1) << 8) | (*pair2);
  temp_addr++;
  *pair1 = (temp_addr & 0xFF00) >> 8;
  *pair2 = temp_addr & 0x00FF;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto INC_sp(GB* gb, u16* sp) ->  void
{
  (*sp)++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto INC_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bool half_carry = ((temp_data & 0x0F) == 0x0F);
  temp_data++;
  bus_write(gb, mem, temp_data);

  /* byte manip */
  gb->cpu.set_n(0);
  gb->cpu.set_z(temp_data == 0x00 ? 1 : 0);
  gb->cpu.set_h(half_carry ? 1 : 0);

  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * uninitialized variable in a constexpr function is a C++20 extension
 */
static /* constexpr */
auto DEC_r(GB* gb, u8* reg) ->  void
{
  bool half_carry;
  ((*reg & 0x0F) == 0x00) ? half_carry = 1 : half_carry = 0;

  (*reg)--;

  /* flag manip */
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(1);
  gb->cpu.set_h(half_carry);

  gb->cpu.instruction_state = 0;
  return;
}

static auto DEC_rr(GB* gb, u8* pair1, u8* pair2) ->  void
{
  u16 temp_addr = (*pair1) << 8 | (*pair2);
  temp_addr--;
  *pair1 = (temp_addr & 0xFF00) >> 8;
  *pair2 = temp_addr & 0x00FF;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto DEC_sp(GB* gb, u16* sp) ->  void
{
  (*sp)--;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto DEC_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  u16 temp_result = static_cast<u8>(temp_data);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_data--;
  bus_write(gb, mem, temp_data);
  /* byte manip */
  gb->cpu.set_n(1);
  if (temp_data == 0x00) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  if ((temp_result & 0xF) == 0x00) { gb->cpu.set_h(1); } else { gb->cpu.set_h(0); }
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto JR_s8(GB* gb) ->  void
{
  u8 temp_signed = bus_read(gb, (gb->cpu.pc));
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

/*
 * This cycle manipulates the internal W and Z variables
 * but would be a mere empty cycle in this emulator
 */
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.pc += static_cast<s8>(temp_signed);
  gb->cpu.instruction_state = 0;
  return;
}

/*
 * In normal opcodes, the NZ and Z instructions are divided,
 * but this emulator joins them together in the *_flag helper function.
 * Atleast for this particular function, DO NOT count on the instruction_state
 * to represent the number of cycles, as depending on the flag check,
 * the function will jump to instruction_state 3, which is actually just
 * instruction_state = 2 IF expected != zero.
 * IF expected == zero, then all the five cycles will be counted
 * and you can rely on instruction_state == number of cycles
 *
 * This opcode is 3 cycles if (condition), 2 cycles if (!condition)
 *
 * STILL NEED TO VERIFY!!!
 * |
 * v
 * Adding pc++ is important to skip ahead of the signed byte
 * after the opcode.
 */
static auto JR_flag(GB* gb, bool expected, bool i_flag) ->  void
{

  u8 temp_signed = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;
  /**
   * This cycle manipulates the internal W and Z variables
   * but would be a mere empty cycle in this emulator
   */
  if (expected == i_flag)
  {
    gb->cpu.instruction_state = 3;
    gb->gb_step();
    gb->cpu.cycles_passed++;
  }
  else
  {
    gb->cpu.instruction_state = 0;
    return;
  }

   gb->cpu.pc += static_cast<s8>(temp_signed);
   gb->cpu.instruction_state = 0;
   return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto AND(GB* gb, u8* reg) ->  void
{
  const u8 val = *reg;
  gb->cpu.A &= val;

  gb->cpu.set_z(gb->cpu.A == 0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(1);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto AND_n(GB* gb) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.A = (gb->cpu.A & temp_data);
  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(1);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto AND_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.A = (gb->cpu.A & temp_data);
  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(1);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto XOR(GB* gb, u8* reg) ->  void
{
  const u8 val = *reg;
  gb->cpu.A ^= val;

  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto XOR_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.A = (gb->cpu.A ^ temp_data);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);
  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto XOR_n(GB* gb) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.A = (gb->cpu.A ^ temp_data);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);
  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto OR(GB* gb, u8* reg) ->  void
{
  const u8 val = (*reg);
  gb->cpu.A = (gb->cpu.A | val);

  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto OR_mrr(GB* gb, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.A = (gb->cpu.A | temp_data);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * TODO : fix opcode naming for the future
 */
static auto OR_n(GB* gb) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.A = (gb->cpu.A | temp_data);
  if ((gb->cpu.A & 0xFF) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.set_c(0);

  gb->cpu.instruction_state = 0;
  return;
}

static auto CP(GB* gb, u8* reg) ->  void
{
  if ((gb->cpu.A - (*reg)) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  gb->cpu.set_n(1);

  if ((gb->cpu.A & 0xF) < ((*reg) & 0xF)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (gb->cpu.A < (*reg)) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }


  gb->cpu.instruction_state = 0;
  return;
}

static auto CP_r_mrr(GB* gb, u8*, u16 mem) ->  void
{
  u8 temp_data = bus_read(gb, mem);
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.set_n(1);

  if ((gb->cpu.A - temp_data) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  if ((gb->cpu.A & 0xF) < (temp_data & 0xF)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (gb->cpu.A < temp_data) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }
  gb->cpu.instruction_state = 0;
  return;
}

static auto CP_r_n(GB* gb, u8 reg) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  gb->cpu.set_n(1);
  if ((reg - temp_data) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }

  if ((reg & 0xF) < (temp_data & 0xF)) {
    gb->cpu.set_h(1); }
  else {
    gb->cpu.set_h(0); }

  if (reg < temp_data) { gb->cpu.set_c(1); } else { gb->cpu.set_c(0); }

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * In normal opcodes, the NZ and Z instructions are divided,
 * but this emulator joins them together in the *_flag helper function.
 * Atleast for this particular function, DO NOT count on the instruction_state
 * to represent the number of cycles, as depending on the flag check,
 * the function will jump to instruction_state 5, which is actually just
 * instruction_state = 2 IF expected != zero.
 * IF expected == zero, then all the five cycles will be counted
 * and you can rely on instruction_state == number of cycles
 *
 * This opcode is 5 cycles if (condition), 2 cycles if (!condition)
 *
 */
static auto RET_flag(GB* gb, bool expected, bool zero) ->  void
{
  u16 temp_addr;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if (expected == zero)
  {
    temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.sp));
    gb->cpu.sp++;
    gb->cpu.instruction_state = 3;
    gb->gb_step();
  gb->cpu.cycles_passed++;
  }
  else
  {
    gb->cpu.instruction_state = 0;
    return;
  }

  temp_addr =
    (((static_cast<u8>(bus_read(gb, gb->cpu.sp))) << 8 ) | (temp_addr));
  gb->cpu.sp++;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.pc = temp_addr;
  gb->cpu.instruction_state = 5;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto RET(GB* gb) ->  void
{
  u16 temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.sp));
  gb->cpu.sp++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_addr =
    (((static_cast<u8>(bus_read(gb, gb->cpu.sp))) << 8 ) | (temp_addr));
  gb->cpu.sp++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.pc = temp_addr;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto RETI(GB* gb) ->  void
{
  u16 temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.sp));
  gb->cpu.sp++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_addr =
    (((static_cast<u8>(bus_read(gb, gb->cpu.sp))) << 8 ) | (temp_addr));

  gb->cpu.sp++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.pc = temp_addr;
  gb->cpu.ime = true;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto POP_rr(GB* gb, u8* upper, u8* lower) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.sp);
  gb->cpu.sp++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u16 temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.sp));
  gb->cpu.sp++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  *upper = static_cast<u8>(temp_addr);

  if (lower == &(gb->cpu.F)) {
    *lower = (temp_data & 0xF0); }
  else {
    *lower = temp_data; }

  gb->cpu.instruction_state = 0;
  return;
}

/*
 * This auto is 4 cycles if (condition), 3 cycles if (!condition) ->  opcode
 */
static void JP_nn_flag(GB* gb, bool expected, bool zero)
{

  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u16 temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.pc));
  gb->cpu.pc++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if(zero == expected)
  {
    gb->cpu.pc = static_cast<u16>((temp_addr << 8) | temp_data);
    gb->cpu.instruction_state = 4;
    gb->gb_step();
    gb->cpu.cycles_passed++;
  }
  else
  {
    gb->cpu.instruction_state = 0;
    return;
  }

  gb->cpu.instruction_state = 0;
  return;

}

static auto JP_nn(GB* gb) ->  void
{
  u8 temp_data = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u16 temp_addr = static_cast<u8>(bus_read(gb, gb->cpu.pc));
  gb->cpu.pc++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.pc = static_cast<u16>((temp_addr << 8) | temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto JP_rr(GB* gb, u16 pair) ->  void
{
  gb->cpu.pc = pair;
  gb->cpu.instruction_state = 0;
  return;
}

/*
 * This auto is 6 cycles if (condition), 3 cycles if (!condition) ->  opcode
 */
static void CALL_flag(GB* gb, bool expected, bool zero)
{
  u16 temp_addr = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;


  temp_addr = ((bus_read(gb, gb->cpu.pc) << 8) | temp_addr);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  if(expected == zero)
  {
    gb->cpu.sp--;
    gb->cpu.instruction_state = 4;
    gb->gb_step();
  gb->cpu.cycles_passed++;
  }
  else
  {
    gb->cpu.instruction_state = 0;
    return;
  }

  bus_write(gb, gb->cpu.sp, static_cast<u8>((gb->cpu.pc >> 8) & 0xFF));
  gb->cpu.sp--;
  gb->cpu.instruction_state = 5;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, static_cast<u8>(gb->cpu.pc & 0xFF));
  gb->cpu.pc = temp_addr;
  gb->cpu.instruction_state = 6;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto CALL(GB* gb) ->  void
{
  u16 temp_addr = bus_read(gb, gb->cpu.pc);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_addr = ((bus_read(gb, gb->cpu.pc) << 8) | temp_addr);
  gb->cpu.pc++;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.sp--;
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, static_cast<u8>((gb->cpu.pc >> 8) & 0xFF));
  gb->cpu.sp--;
  gb->cpu.instruction_state = 5;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, static_cast<u8>(gb->cpu.pc & 0xFF));
  gb->cpu.pc = temp_addr;
  gb->cpu.instruction_state = 6;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto PUSH_rr(GB* gb, u16 pair) ->  void
{
  gb->cpu.sp--;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, ((pair & 0xFF00) >> 8));
  gb->cpu.sp--;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, (pair & 0xFF));
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto RST(GB* gb, u8 i_rst) ->  void
{
  gb->cpu.sp--;
  gb->cpu.instruction_state = 2;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, ((gb->cpu.pc >> 8) & 0xFF));
  gb->cpu.sp--;
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  bus_write(gb, gb->cpu.sp, (gb->cpu.pc & 0xFF));
  gb->cpu.pc = static_cast<u16>(i_rst * 8);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;
  return;
}

static auto DI(GB* gb) ->  void
{
  gb->cpu.ime = false;
  gb->cpu.ime_scheduled = 0;

  gb->cpu.instruction_state = 0;
  return;
}

static auto EI(GB* gb) ->  void
{
  if ((gb->cpu.ime == false) && (gb->cpu.ime_scheduled == 0))
    gb->cpu.ime_scheduled = 1;

  gb->cpu.instruction_state = 0;
  return;
}

static auto HALT(GB* gb) -> void
{
  /**
   * Generally halt flag is not used since the HALT helper handles execution,
   */
  const bool pending_interrupt = (gb->cpu.get_intrr_flags(gb->mmu) != 0);

  if (pending_interrupt)
  {
    if ((gb->cpu.ime == 0) &&
        // (gb->cpu.halt_bug == false) &&
        (gb->cpu.instruction_state == 1))
    {
      gb->cpu.halt_bug = true;
    }

    gb->cpu.halt = false;
    gb->cpu.instruction_state = 0;
    return;
  }
  else
  {
    gb->cpu.halt_bug = false;
    gb->cpu.halt = true;
    gb->cpu.instruction_state = 2;
    gb->gb_step();
    gb->cpu.cycles_passed++;
    return;
  }
}

static auto STOP(GB* gb) ->  void
{
  /* Read, but not used, therefore commented out */
  // bus_read(gb, gb->cpu.pc);

  // TODO: Handle speedswitch enabling
  if ((gb->mmu.io[0x00] & 0x30) != 0x30) { gb->cpu.joypad_accessed = true; }
  // bool speed_switch = false;


  u8 interrupt_flags = gb->cpu.get_intrr_flags(gb->mmu);

  // gb->cpu.set_intrr_enable();

  // gb->cpu.interrupt_enable = (interrupt_flags != 0);

  if (interrupt_flags == 0)
  {
    gb->cpu.pc++;
    cpu_optable[0x76](gb);
  }
  else
  {
    gb->cpu.instruction_state = 0;
    return;
    /* DIV doesn't reset */
  }

  // if (speed_switch)
  // {
    /* TODO: Handle this for CGB*/
  // }
}

static auto cpu_op_0x00(GB* gb) -> void { NOP(gb); }
static auto cpu_op_0x01(GB* gb) -> void { LD_rr_nn(gb, &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0x02(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.A, gb->cpu.get_bc()); }
static auto cpu_op_0x03(GB* gb) -> void { INC_rr(gb, &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0x04(GB* gb) -> void { INC_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0x05(GB* gb) -> void { DEC_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0x06(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.B)); }
static auto cpu_op_0x07(GB* gb) -> void { RLCA(gb); }
static auto cpu_op_0x08(GB* gb) -> void { LD_mnn_sp(gb); }
static auto cpu_op_0x09(GB* gb) -> void { ADD_rr_rr(gb, &(gb->cpu.H), &(gb->cpu.L), &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0x0A(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_bc()); }
static auto cpu_op_0x0B(GB* gb) -> void { DEC_rr(gb, &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0x0C(GB* gb) -> void { INC_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0x0D(GB* gb) -> void { DEC_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0x0E(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.C)); }
static auto cpu_op_0x0F(GB* gb) -> void { RRCA(gb); }


static auto cpu_op_0x10(GB* gb) -> void { STOP(gb); }
static auto cpu_op_0x11(GB* gb) -> void { LD_rr_nn(gb, &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0x12(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.A, gb->cpu.get_de()); }
static auto cpu_op_0x13(GB* gb) -> void { INC_rr(gb, &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0x14(GB* gb) -> void { INC_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0x15(GB* gb) -> void { DEC_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0x16(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.D)); }
static auto cpu_op_0x17(GB* gb) -> void { RLA(gb); }
static auto cpu_op_0x18(GB* gb) -> void { JR_s8(gb); }
static auto cpu_op_0x19(GB* gb) -> void { ADD_rr_rr(gb, &(gb->cpu.H), &(gb->cpu.L), &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0x1A(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_de()); }
static auto cpu_op_0x1B(GB* gb) -> void { DEC_rr(gb, &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0x1C(GB* gb) -> void { INC_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0x1D(GB* gb) -> void { DEC_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0x1E(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.E)); }
static auto cpu_op_0x1F(GB* gb) -> void { RRA(gb); }



static auto cpu_op_0x20(GB* gb) -> void { JR_flag(gb, 0, gb->cpu.get_z()); }
static auto cpu_op_0x21(GB* gb) -> void { LD_rr_nn(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0x22(GB* gb) -> void {
  u16 hl = gb->cpu.get_hl();
  LD_mrr_r(gb, gb->cpu.A, hl);
  hl++;
  gb->cpu.set_hl(hl);
}
static auto cpu_op_0x23(GB* gb) -> void { INC_rr(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0x24(GB* gb) -> void { INC_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0x25(GB* gb) -> void { DEC_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0x26(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.H)); }
static auto cpu_op_0x27(GB* gb) -> void { DAA(gb); }
static auto cpu_op_0x28(GB* gb) -> void { JR_flag(gb, 1, gb->cpu.get_z()); }
static auto cpu_op_0x29(GB* gb) -> void { ADD_rr_rr(gb, &(gb->cpu.H), &(gb->cpu.L), &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0x2A(GB* gb) -> void {
  u16 hl = gb->cpu.get_hl();
  LD_r_mrr(gb, &(gb->cpu.A), hl);
  hl++;
  gb->cpu.set_hl(hl);
}
static auto cpu_op_0x2B(GB* gb) -> void { DEC_rr(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0x2C(GB* gb) -> void { INC_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0x2D(GB* gb) -> void { DEC_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0x2E(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.L)); }
static auto cpu_op_0x2F(GB* gb) -> void { CPL(gb); }



static auto cpu_op_0x30(GB* gb) -> void { JR_flag(gb, 0, gb->cpu.get_c()); }
static auto cpu_op_0x31(GB* gb) -> void { LD_sp_nn(gb); }
static auto cpu_op_0x32(GB* gb) -> void {
  u16 hl = gb->cpu.get_hl();
  LD_mrr_r(gb, gb->cpu.A, hl);
  hl--;
  gb->cpu.set_hl(hl);
}
static auto cpu_op_0x33(GB* gb) -> void { INC_sp(gb, &(gb->cpu.sp)); }
static auto cpu_op_0x34(GB* gb) -> void { INC_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0x35(GB* gb) -> void { DEC_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0x36(GB* gb) -> void { LD_mrr_n(gb, gb->cpu.get_hl()); }
static auto cpu_op_0x37(GB* gb) -> void { SCF(gb); }
static auto cpu_op_0x38(GB* gb) -> void { JR_flag(gb, 1, gb->cpu.get_c()); }
static auto cpu_op_0x39(GB* gb) -> void { ADD_rr_sp(gb, &(gb->cpu.H), &(gb->cpu.L), &(gb->cpu.sp)); }
static auto cpu_op_0x3A(GB* gb) -> void {
  u16 hl = gb->cpu.get_hl();
  LD_r_mrr(gb, &(gb->cpu.A), hl);
  hl--;
  gb->cpu.set_hl(hl);
}
static auto cpu_op_0x3B(GB* gb) -> void { DEC_sp(gb, &(gb->cpu.sp)); }
static auto cpu_op_0x3C(GB* gb) -> void { INC_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0x3D(GB* gb) -> void { DEC_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0x3E(GB* gb) -> void { LD_r_n(gb, &(gb->cpu.A)); }
static auto cpu_op_0x3F(GB* gb) -> void { CCF(gb); }



static auto cpu_op_0x40(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.B)); }
static auto cpu_op_0x41(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0x42(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.D)); }
static auto cpu_op_0x43(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.E)); }
static auto cpu_op_0x44(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.H)); }
static auto cpu_op_0x45(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.L)); }
static auto cpu_op_0x46(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.B), gb->cpu.get_hl()); }
static auto cpu_op_0x47(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.B), &(gb->cpu.A)); }
static auto cpu_op_0x48(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.B)); }
static auto cpu_op_0x49(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.C)); }
static auto cpu_op_0x4A(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.D)); }
static auto cpu_op_0x4B(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.E)); }
static auto cpu_op_0x4C(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.H)); }
static auto cpu_op_0x4D(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.L)); }
static auto cpu_op_0x4E(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.C), gb->cpu.get_hl()); }
static auto cpu_op_0x4F(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.C), &(gb->cpu.A)); }



static auto cpu_op_0x50(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.B)); }
static auto cpu_op_0x51(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.C)); }
static auto cpu_op_0x52(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.D)); }
static auto cpu_op_0x53(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0x54(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.H)); }
static auto cpu_op_0x55(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.L)); }
static auto cpu_op_0x56(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.D), gb->cpu.get_hl()); }
static auto cpu_op_0x57(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.D), &(gb->cpu.A)); }
static auto cpu_op_0x58(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.B)); }
static auto cpu_op_0x59(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.C)); }
static auto cpu_op_0x5A(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.D)); }
static auto cpu_op_0x5B(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.E)); }
static auto cpu_op_0x5C(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.H)); }
static auto cpu_op_0x5D(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.L)); }
static auto cpu_op_0x5E(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.E), gb->cpu.get_hl()); }
static auto cpu_op_0x5F(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.E), &(gb->cpu.A)); }



static auto cpu_op_0x60(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.B)); }
static auto cpu_op_0x61(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.C)); }
static auto cpu_op_0x62(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.D)); }
static auto cpu_op_0x63(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.E)); }
static auto cpu_op_0x64(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.H)); }
static auto cpu_op_0x65(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0x66(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.H), gb->cpu.get_hl()); }
static auto cpu_op_0x67(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.H), &(gb->cpu.A)); }
static auto cpu_op_0x68(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.B)); }
static auto cpu_op_0x69(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.C)); }
static auto cpu_op_0x6A(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.D)); }
static auto cpu_op_0x6B(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.E)); }
static auto cpu_op_0x6C(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.H)); }
static auto cpu_op_0x6D(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.L)); }
static auto cpu_op_0x6E(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.L), gb->cpu.get_hl()); }
static auto cpu_op_0x6F(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.L), &(gb->cpu.A)); }



static auto cpu_op_0x70(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.B, gb->cpu.get_hl()); }
static auto cpu_op_0x71(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.C, gb->cpu.get_hl()); }
static auto cpu_op_0x72(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.D, gb->cpu.get_hl()); }
static auto cpu_op_0x73(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.E, gb->cpu.get_hl()); }
static auto cpu_op_0x74(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.H, gb->cpu.get_hl()); }
static auto cpu_op_0x75(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.L, gb->cpu.get_hl()); }
static auto cpu_op_0x76(GB* gb) -> void { HALT(gb); }
static auto cpu_op_0x77(GB* gb) -> void { LD_mrr_r(gb, gb->cpu.A, gb->cpu.get_hl()); }
static auto cpu_op_0x78(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.B)); }
static auto cpu_op_0x79(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.C)); }
static auto cpu_op_0x7A(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.D)); }
static auto cpu_op_0x7B(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.E)); }
static auto cpu_op_0x7C(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.H)); }
static auto cpu_op_0x7D(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.L)); }
static auto cpu_op_0x7E(GB* gb) -> void { LD_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_hl()); }
static auto cpu_op_0x7F(GB* gb) -> void { LD_r_r(gb, &(gb->cpu.A), &(gb->cpu.A)); }



static auto cpu_op_0x80(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.B)); }
static auto cpu_op_0x81(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.C)); }
static auto cpu_op_0x82(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.D)); }
static auto cpu_op_0x83(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.E)); }
static auto cpu_op_0x84(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.H)); }
static auto cpu_op_0x85(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.L)); }
static auto cpu_op_0x86(GB* gb) -> void { ADD_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_hl()); }
static auto cpu_op_0x87(GB* gb) -> void { ADD_r_r(gb, &(gb->cpu.A), &(gb->cpu.A)); }
static auto cpu_op_0x88(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.B)); }
static auto cpu_op_0x89(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.C)); }
static auto cpu_op_0x8A(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.D)); }
static auto cpu_op_0x8B(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.E)); }
static auto cpu_op_0x8C(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.H)); }
static auto cpu_op_0x8D(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.L)); }
static auto cpu_op_0x8E(GB* gb) -> void { ADC_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_hl()); }
static auto cpu_op_0x8F(GB* gb) -> void { ADC(gb, &(gb->cpu.A), &(gb->cpu.A)); }



static auto cpu_op_0x90(GB* gb) -> void { SUB_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0x91(GB* gb) -> void { SUB_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0x92(GB* gb) -> void { SUB_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0x93(GB* gb) -> void { SUB_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0x94(GB* gb) -> void { SUB_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0x95(GB* gb) -> void { SUB_r(gb, &(gb->cpu.L));}
static auto cpu_op_0x96(GB* gb) -> void { SUB_mrr(gb, gb->cpu.get_hl());}
static auto cpu_op_0x97(GB* gb) -> void { SUB_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0x98(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.B)); }
static auto cpu_op_0x99(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.C)); }
static auto cpu_op_0x9A(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.D)); }
static auto cpu_op_0x9B(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.E)); }
static auto cpu_op_0x9C(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.H)); }
static auto cpu_op_0x9D(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.L)); }
static auto cpu_op_0x9E(GB* gb) -> void { SBC_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_hl()); }
static auto cpu_op_0x9F(GB* gb) -> void { SBC_r_r(gb, &(gb->cpu.A), (gb->cpu.A)); }



static auto cpu_op_0xA0(GB* gb) -> void { AND(gb, &(gb->cpu.B)); }
static auto cpu_op_0xA1(GB* gb) -> void { AND(gb, &(gb->cpu.C)); }
static auto cpu_op_0xA2(GB* gb) -> void { AND(gb, &(gb->cpu.D)); }
static auto cpu_op_0xA3(GB* gb) -> void { AND(gb, &(gb->cpu.E)); }
static auto cpu_op_0xA4(GB* gb) -> void { AND(gb, &(gb->cpu.H)); }
static auto cpu_op_0xA5(GB* gb) -> void { AND(gb, &(gb->cpu.L)); }
static auto cpu_op_0xA6(GB* gb) -> void { AND_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xA7(GB* gb) -> void { AND(gb, &(gb->cpu.A)); }
static auto cpu_op_0xA8(GB* gb) -> void { XOR(gb, &(gb->cpu.B)); }
static auto cpu_op_0xA9(GB* gb) -> void { XOR(gb, &(gb->cpu.C)); }
static auto cpu_op_0xAA(GB* gb) -> void { XOR(gb, &(gb->cpu.D)); }
static auto cpu_op_0xAB(GB* gb) -> void { XOR(gb, &(gb->cpu.E)); }
static auto cpu_op_0xAC(GB* gb) -> void { XOR(gb, &(gb->cpu.H)); }
static auto cpu_op_0xAD(GB* gb) -> void { XOR(gb, &(gb->cpu.L)); }
static auto cpu_op_0xAE(GB* gb) -> void { XOR_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xAF(GB* gb) -> void { XOR(gb, &(gb->cpu.A)); }



static auto cpu_op_0xB0(GB* gb) -> void { OR(gb, &(gb->cpu.B)); }
static auto cpu_op_0xB1(GB* gb) -> void { OR(gb, &(gb->cpu.C)); }
static auto cpu_op_0xB2(GB* gb) -> void { OR(gb, &(gb->cpu.D)); }
static auto cpu_op_0xB3(GB* gb) -> void { OR(gb, &(gb->cpu.E)); }
static auto cpu_op_0xB4(GB* gb) -> void { OR(gb, &(gb->cpu.H)); }
static auto cpu_op_0xB5(GB* gb) -> void { OR(gb, &(gb->cpu.L)); }
static auto cpu_op_0xB6(GB* gb) -> void { OR_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xB7(GB* gb) -> void { OR(gb, &(gb->cpu.A)); }
static auto cpu_op_0xB8(GB* gb) -> void { CP(gb, &(gb->cpu.B)); }
static auto cpu_op_0xB9(GB* gb) -> void { CP(gb, &(gb->cpu.C)); }
static auto cpu_op_0xBA(GB* gb) -> void { CP(gb, &(gb->cpu.D)); }
static auto cpu_op_0xBB(GB* gb) -> void { CP(gb, &(gb->cpu.E)); }
static auto cpu_op_0xBC(GB* gb) -> void { CP(gb, &(gb->cpu.H)); }
static auto cpu_op_0xBD(GB* gb) -> void { CP(gb, &(gb->cpu.L)); }
static auto cpu_op_0xBE(GB* gb) -> void { CP_r_mrr(gb, &(gb->cpu.A), gb->cpu.get_hl()); }
static auto cpu_op_0xBF(GB* gb) -> void { CP(gb, &(gb->cpu.A)); }



static auto cpu_op_0xC0(GB* gb) -> void { RET_flag(gb, 0, gb->cpu.get_z()); }
static auto cpu_op_0xC1(GB* gb) -> void { POP_rr(gb, &(gb->cpu.B), &(gb->cpu.C)); }
static auto cpu_op_0xC2(GB* gb) -> void { JP_nn_flag(gb, 0, gb->cpu.get_z()); }
static auto cpu_op_0xC3(GB* gb) -> void { JP_nn(gb); }
static auto cpu_op_0xC4(GB* gb) -> void { CALL_flag(gb, 0, gb->cpu.get_z()); }
static auto cpu_op_0xC5(GB* gb) -> void { PUSH_rr(gb, gb->cpu.get_bc()); }
static auto cpu_op_0xC6(GB* gb) -> void { ADD_r_n(gb, &(gb->cpu.A)); }
static auto cpu_op_0xC7(GB* gb) -> void { RST(gb, 0); }
static auto cpu_op_0xC8(GB* gb) -> void { RET_flag(gb, 1, gb->cpu.get_z()); }
static auto cpu_op_0xC9(GB* gb) -> void { RET(gb); }
static auto cpu_op_0xCA(GB* gb) -> void { JP_nn_flag(gb, 1, gb->cpu.get_z()); }
/* Illegal!  */
static auto cpu_op_0xCC(GB* gb) -> void { CALL_flag(gb, 1, gb->cpu.get_z()); }
static auto cpu_op_0xCD(GB* gb) -> void { CALL(gb); }
static auto cpu_op_0xCE(GB* gb) -> void { ADC_r_n(gb, &(gb->cpu.A)); }
static auto cpu_op_0xCF(GB* gb) -> void { RST(gb, 1); }



static auto cpu_op_0xD0(GB* gb) -> void { RET_flag(gb, 0, gb->cpu.get_c()); }
static auto cpu_op_0xD1(GB* gb) -> void { POP_rr(gb, &(gb->cpu.D), &(gb->cpu.E)); }
static auto cpu_op_0xD2(GB* gb) -> void { JP_nn_flag(gb, 0, gb->cpu.get_c()); }
/* Illegal */
static auto cpu_op_0xD4(GB* gb) -> void { CALL_flag(gb, 0, gb->cpu.get_c()); }
static auto cpu_op_0xD5(GB* gb) -> void { PUSH_rr(gb, gb->cpu.get_de()); }
static auto cpu_op_0xD6(GB* gb) -> void { SUB_n(gb); }
static auto cpu_op_0xD7(GB* gb) -> void { RST(gb, 2); }
static auto cpu_op_0xD8(GB* gb) -> void { RET_flag(gb, 1, gb->cpu.get_c()); }
static auto cpu_op_0xD9(GB* gb) -> void { RETI(gb); }
static auto cpu_op_0xDA(GB* gb) -> void { JP_nn_flag(gb, 1, gb->cpu.get_c()); }
/* Illegal */
static auto cpu_op_0xDC(GB* gb) -> void { CALL_flag(gb, 1, gb->cpu.get_c()); }
/* Illegal */
static auto cpu_op_0xDE(GB* gb) -> void { SBC_r_n(gb, &(gb->cpu.A)); }
static auto cpu_op_0xDF(GB* gb) -> void { RST(gb, 3); }



static auto cpu_op_0xE0(GB* gb) -> void { LD_mn_r(gb, gb->cpu.A); }
static auto cpu_op_0xE1(GB* gb) -> void { POP_rr(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0xE2(GB* gb) -> void { LD_mr_r(gb, gb->cpu.C, gb->cpu.A); }
/* Illegal */
/* Illegal */
static auto cpu_op_0xE5(GB* gb) -> void { PUSH_rr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xE6(GB* gb) -> void { AND_n(gb); }
static auto cpu_op_0xE7(GB* gb) -> void { RST(gb, 4); }
static auto cpu_op_0xE8(GB* gb) -> void { ADD_sp_s8(gb); }
static auto cpu_op_0xE9(GB* gb) -> void { JP_rr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xEA(GB* gb) -> void { LD_mnn_r(gb, gb->cpu.A); }
/* Illegal */
/* Illegal */
/* Illegal */
static auto cpu_op_0xEE(GB* gb) -> void { XOR_n(gb); }
static auto cpu_op_0xEF(GB* gb) -> void { RST(gb, 5); }



static auto cpu_op_0xF0(GB* gb) -> void { LD_r_mn(gb, &(gb->cpu.A)); }
static auto cpu_op_0xF1(GB* gb) -> void { POP_rr(gb, &(gb->cpu.A), &(gb->cpu.F)); }
static auto cpu_op_0xF2(GB* gb) -> void { LD_r_mr(gb, &(gb->cpu.A), gb->cpu.C); }
static auto cpu_op_0xF3(GB* gb) -> void { DI(gb); }
/* Illegal */
static auto cpu_op_0xF5(GB* gb) -> void { PUSH_rr(gb, gb->cpu.get_af()); }
static auto cpu_op_0xF6(GB* gb) -> void { OR_n(gb); }
static auto cpu_op_0xF7(GB* gb) -> void { RST(gb, 6); }
static auto cpu_op_0xF8(GB* gb) -> void { LD_rr_sps8(gb, &(gb->cpu.H), &(gb->cpu.L)); }
static auto cpu_op_0xF9(GB* gb) -> void { LD_sp_rr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xFA(GB* gb) -> void { LD_r_mnn(gb, &(gb->cpu.A)); }
static auto cpu_op_0xFB(GB* gb) -> void { EI(gb); }
/* Illegal */
/* Illegal */
static auto cpu_op_0xFE(GB* gb) -> void { CP_r_n(gb, gb->cpu.A); }
static auto cpu_op_0xFF(GB* gb) -> void { RST(gb, 7); }



static auto cpu_op_illegal(GB* gb) -> void {
  gb->cpu.halt = true;
  gb->cpu.instruction_state = 0;
}



/*
 * 0xCB opcodes and helper functions start here
 */
static auto RLC_r(GB* gb, u8* reg) -> void
{
  gb->cpu.set_c(((*reg) & 0x80) >> 7);
  (*reg) = (*reg) << 1;
  (*reg) = (*reg) | gb->cpu.get_c();
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.instruction_state = 0;

  return;
}

static auto RLC_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.set_c((temp_data & 0x80) >> 7);
  temp_data = temp_data << 1;
  temp_data = temp_data | gb->cpu.get_c();
  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto RRC_r(GB* gb, u8* reg) -> void
{
  gb->cpu.set_c((*reg) & 0x1);
  (*reg) = (*reg) >> 1;
  (*reg) = (*reg) | (gb->cpu.get_c() << 7);
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.instruction_state = 0;

  return;
}

static auto RRC_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.set_c(temp_data & 0x1);
  temp_data = temp_data >> 1;
  temp_data = temp_data | (gb->cpu.get_c() << 7);
  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto RL_r(GB* gb, u8* reg) -> void
{
  u8 temp_storage = static_cast<bool>(gb->cpu.get_c());
  u8 temp_bit7 = ((*reg) >> 7);
  (*reg) = (*reg) << 1;
  (*reg) = (*reg) | temp_storage;
  gb->cpu.set_c(temp_bit7);

  gb->cpu.set_z(((*reg) == 0) ? 1 : 0);
  gb->cpu.set_h(0);
  gb->cpu.set_n(0);
  gb->cpu.instruction_state = 0;

  return;
}

static auto RL_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_storage = static_cast<bool>(gb->cpu.get_c());
  u8 temp_bit7 = (temp_data >> 7);
  temp_data = temp_data << 1;
  temp_data = temp_data | temp_storage;
  gb->cpu.set_c(temp_bit7);

  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_h(0);
  gb->cpu.set_n(0);
  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto RR_r(GB* gb, u8* reg) -> void
{
  u8 temp_bit0 = (*reg) & 0x1;
  (*reg) = (*reg) >> 1;
  (*reg) = ((*reg) | (gb->cpu.get_c() << 7));
  gb->cpu.set_c(temp_bit0);
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);
  gb->cpu.instruction_state = 0;

  return;
}

static auto RR_mrr(GB* gb, u16 pair) -> void
{

  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_bit0 = temp_data & 0x1;
  temp_data = temp_data >> 1;
  temp_data = (temp_data | (gb->cpu.get_c() << 7));

  gb->cpu.set_c(temp_bit0);
  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  bus_write(gb, pair, temp_data);

  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SLA_r(GB* gb, u8* reg) -> void
{
  u8 temp_bit7 = (((*reg) & 0x80) >> 7);
  (*reg) = (*reg) << 1;
  (*reg) = (*reg) & 0xFE;

  gb->cpu.set_c(temp_bit7);
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  gb->cpu.instruction_state = 0;

  return;
}

static auto SLA_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 temp_bit7 = ((temp_data & 0x80) >> 7);
  temp_data = temp_data << 1;
  temp_data = temp_data & 0xFE;

  gb->cpu.set_c(temp_bit7);
  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  bus_write(gb, pair, temp_data);

  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SRA_r(GB* gb, u8* reg) -> void
{
  gb->cpu.set_c((*reg) & 0x1);
  (*reg) = (*reg) >> 1;

  (*reg) = ((((*reg) & 0x40) << 1) | (*reg));
  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  gb->cpu.instruction_state = 0;

  return;
}

static auto SRA_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.set_c(temp_data & 0x1);
  temp_data = temp_data >> 1;
  temp_data = (((temp_data & 0x40) << 1) | temp_data);

  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SWAP_r(GB* gb, u8* reg) -> void
{
  u8 new_var = (((*reg) & 0xF0) >> 4);
  new_var = ((((*reg) & 0x0F) << 4) | new_var);
  if (new_var == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_c(0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  *reg = new_var;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SWAP_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  u8 new_var = ((temp_data & 0xF0) >> 4);
  new_var = (((temp_data & 0x0F) << 4) | new_var);
  if (new_var == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_c(0);
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  bus_write(gb, pair, new_var);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SRL_r(GB* gb, u8* reg) -> void
{
  gb->cpu.set_c((*reg) & 0x1);
  (*reg) = (*reg) >> 1;

  if ((*reg) == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  gb->cpu.instruction_state = 0;

  return;
}

static auto SRL_mrr(GB* gb, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.set_c(temp_data & 0x1);
  temp_data = temp_data >> 1;

  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(0);

  bus_write(gb, pair, temp_data);

  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto BIT_b_r(GB* gb, u8 bit, u8* reg) -> void
{
  u8 bit_value = (*reg) & (1 << bit);

  if (bit_value == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(1);

  gb->cpu.instruction_state = 0;

  return;
}

static auto BIT_b_mrr(GB* gb, u8 bit, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_data = temp_data & (0b1 << bit);
  if (temp_data == 0) { gb->cpu.set_z(1); } else { gb->cpu.set_z(0); }
  gb->cpu.set_n(0);
  gb->cpu.set_h(1);

  gb->cpu.instruction_state = 0;

  return;
}

static auto RES_b_r(GB* gb, u8 bit, u8* reg) -> void
{
  (*reg) = (*reg) & ~(1 << bit);
  gb->cpu.instruction_state = 0;

  return;
}

static auto RES_b_mrr(GB* gb, u8 bit, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_data = temp_data & ~(1 << bit);
  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto SET_b_r(GB* gb, u8 bit, u8* reg) -> void
{
  (*reg) = (*reg) | (1 << bit);
  gb->cpu.instruction_state = 0;

  return;
}

static auto SET_b_mrr(GB* gb, u8 bit, u16 pair) -> void
{
  u8 temp_data = bus_read(gb, pair);
  gb->cpu.instruction_state = 3;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  temp_data = temp_data | (1 << bit);
  bus_write(gb, pair, temp_data);
  gb->cpu.instruction_state = 4;
  gb->gb_step();
  gb->cpu.cycles_passed++;

  gb->cpu.instruction_state = 0;

  return;
}

static auto cpu_op_0xCB00(GB* gb) -> void { RLC_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB01(GB* gb) -> void { RLC_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB02(GB* gb) -> void { RLC_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB03(GB* gb) -> void { RLC_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB04(GB* gb) -> void { RLC_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB05(GB* gb) -> void { RLC_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB06(GB* gb) -> void { RLC_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB07(GB* gb) -> void { RLC_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0xCB08(GB* gb) -> void { RRC_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB09(GB* gb) -> void { RRC_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB0A(GB* gb) -> void { RRC_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB0B(GB* gb) -> void { RRC_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB0C(GB* gb) -> void { RRC_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB0D(GB* gb) -> void { RRC_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB0E(GB* gb) -> void { RRC_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB0F(GB* gb) -> void { RRC_r(gb, &(gb->cpu.A)); }

static auto cpu_op_0xCB10(GB* gb) -> void { RL_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB11(GB* gb) -> void { RL_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB12(GB* gb) -> void { RL_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB13(GB* gb) -> void { RL_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB14(GB* gb) -> void { RL_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB15(GB* gb) -> void { RL_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB16(GB* gb) -> void { RL_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB17(GB* gb) -> void { RL_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0xCB18(GB* gb) -> void { RR_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB19(GB* gb) -> void { RR_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB1A(GB* gb) -> void { RR_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB1B(GB* gb) -> void { RR_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB1C(GB* gb) -> void { RR_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB1D(GB* gb) -> void { RR_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB1E(GB* gb) -> void { RR_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB1F(GB* gb) -> void { RR_r(gb, &(gb->cpu.A)); }

static auto cpu_op_0xCB20(GB* gb) -> void { SLA_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB21(GB* gb) -> void { SLA_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB22(GB* gb) -> void { SLA_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB23(GB* gb) -> void { SLA_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB24(GB* gb) -> void { SLA_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB25(GB* gb) -> void { SLA_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB26(GB* gb) -> void { SLA_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB27(GB* gb) -> void { SLA_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0xCB28(GB* gb) -> void { SRA_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB29(GB* gb) -> void { SRA_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB2A(GB* gb) -> void { SRA_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB2B(GB* gb) -> void { SRA_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB2C(GB* gb) -> void { SRA_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB2D(GB* gb) -> void { SRA_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB2E(GB* gb) -> void { SRA_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB2F(GB* gb) -> void { SRA_r(gb, &(gb->cpu.A)); }

static auto cpu_op_0xCB30(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB31(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB32(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB33(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB34(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB35(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB36(GB* gb) -> void { SWAP_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB37(GB* gb) -> void { SWAP_r(gb, &(gb->cpu.A)); }
static auto cpu_op_0xCB38(GB* gb) -> void { SRL_r(gb, &(gb->cpu.B)); }
static auto cpu_op_0xCB39(GB* gb) -> void { SRL_r(gb, &(gb->cpu.C)); }
static auto cpu_op_0xCB3A(GB* gb) -> void { SRL_r(gb, &(gb->cpu.D)); }
static auto cpu_op_0xCB3B(GB* gb) -> void { SRL_r(gb, &(gb->cpu.E)); }
static auto cpu_op_0xCB3C(GB* gb) -> void { SRL_r(gb, &(gb->cpu.H)); }
static auto cpu_op_0xCB3D(GB* gb) -> void { SRL_r(gb, &(gb->cpu.L)); }
static auto cpu_op_0xCB3E(GB* gb) -> void { SRL_mrr(gb, gb->cpu.get_hl()); }
static auto cpu_op_0xCB3F(GB* gb) -> void { SRL_r(gb, &(gb->cpu.A)); }

static auto cpu_op_0xCB40(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.B)); }
static auto cpu_op_0xCB41(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.C)); }
static auto cpu_op_0xCB42(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.D)); }
static auto cpu_op_0xCB43(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.E)); }
static auto cpu_op_0xCB44(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.H)); }
static auto cpu_op_0xCB45(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.L)); }
static auto cpu_op_0xCB46(GB* gb) -> void { BIT_b_mrr(gb, 0, gb->cpu.get_hl()); }
static auto cpu_op_0xCB47(GB* gb) -> void { BIT_b_r(gb, 0, &(gb->cpu.A)); }
static auto cpu_op_0xCB48(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.B)); }
static auto cpu_op_0xCB49(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.C)); }
static auto cpu_op_0xCB4A(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.D)); }
static auto cpu_op_0xCB4B(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.E)); }
static auto cpu_op_0xCB4C(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.H)); }
static auto cpu_op_0xCB4D(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.L)); }
static auto cpu_op_0xCB4E(GB* gb) -> void { BIT_b_mrr(gb, 1, gb->cpu.get_hl()); }
static auto cpu_op_0xCB4F(GB* gb) -> void { BIT_b_r(gb, 1, &(gb->cpu.A)); }

static auto cpu_op_0xCB50(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.B)); }
static auto cpu_op_0xCB51(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.C)); }
static auto cpu_op_0xCB52(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.D)); }
static auto cpu_op_0xCB53(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.E)); }
static auto cpu_op_0xCB54(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.H)); }
static auto cpu_op_0xCB55(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.L)); }
static auto cpu_op_0xCB56(GB* gb) -> void { BIT_b_mrr(gb, 2, gb->cpu.get_hl()); }
static auto cpu_op_0xCB57(GB* gb) -> void { BIT_b_r(gb, 2, &(gb->cpu.A)); }
static auto cpu_op_0xCB58(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.B)); }
static auto cpu_op_0xCB59(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.C)); }
static auto cpu_op_0xCB5A(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.D)); }
static auto cpu_op_0xCB5B(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.E)); }
static auto cpu_op_0xCB5C(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.H)); }
static auto cpu_op_0xCB5D(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.L)); }
static auto cpu_op_0xCB5E(GB* gb) -> void { BIT_b_mrr(gb, 3, gb->cpu.get_hl()); }
static auto cpu_op_0xCB5F(GB* gb) -> void { BIT_b_r(gb, 3, &(gb->cpu.A)); }

static auto cpu_op_0xCB60(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.B)); }
static auto cpu_op_0xCB61(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.C)); }
static auto cpu_op_0xCB62(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.D)); }
static auto cpu_op_0xCB63(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.E)); }
static auto cpu_op_0xCB64(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.H)); }
static auto cpu_op_0xCB65(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.L)); }
static auto cpu_op_0xCB66(GB* gb) -> void { BIT_b_mrr(gb, 4, gb->cpu.get_hl()); }
static auto cpu_op_0xCB67(GB* gb) -> void { BIT_b_r(gb, 4, &(gb->cpu.A)); }
static auto cpu_op_0xCB68(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.B)); }
static auto cpu_op_0xCB69(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.C)); }
static auto cpu_op_0xCB6A(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.D)); }
static auto cpu_op_0xCB6B(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.E)); }
static auto cpu_op_0xCB6C(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.H)); }
static auto cpu_op_0xCB6D(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.L)); }
static auto cpu_op_0xCB6E(GB* gb) -> void { BIT_b_mrr(gb, 5, gb->cpu.get_hl()); }
static auto cpu_op_0xCB6F(GB* gb) -> void { BIT_b_r(gb, 5, &(gb->cpu.A)); }

static auto cpu_op_0xCB70(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.B)); }
static auto cpu_op_0xCB71(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.C)); }
static auto cpu_op_0xCB72(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.D)); }
static auto cpu_op_0xCB73(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.E)); }
static auto cpu_op_0xCB74(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.H)); }
static auto cpu_op_0xCB75(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.L)); }
static auto cpu_op_0xCB76(GB* gb) -> void { BIT_b_mrr(gb, 6, gb->cpu.get_hl()); }
static auto cpu_op_0xCB77(GB* gb) -> void { BIT_b_r(gb, 6, &(gb->cpu.A)); }
static auto cpu_op_0xCB78(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.B)); }
static auto cpu_op_0xCB79(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.C)); }
static auto cpu_op_0xCB7A(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.D)); }
static auto cpu_op_0xCB7B(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.E)); }
static auto cpu_op_0xCB7C(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.H)); }
static auto cpu_op_0xCB7D(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.L)); }
static auto cpu_op_0xCB7E(GB* gb) -> void { BIT_b_mrr(gb, 7, gb->cpu.get_hl()); }
static auto cpu_op_0xCB7F(GB* gb) -> void { BIT_b_r(gb, 7, &(gb->cpu.A)); }

static auto cpu_op_0xCB80(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.B)); }
static auto cpu_op_0xCB81(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.C)); }
static auto cpu_op_0xCB82(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.D)); }
static auto cpu_op_0xCB83(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.E)); }
static auto cpu_op_0xCB84(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.H)); }
static auto cpu_op_0xCB85(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.L)); }
static auto cpu_op_0xCB86(GB* gb) -> void { RES_b_mrr(gb, 0, gb->cpu.get_hl()); }
static auto cpu_op_0xCB87(GB* gb) -> void { RES_b_r(gb, 0, &(gb->cpu.A)); }
static auto cpu_op_0xCB88(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.B)); }
static auto cpu_op_0xCB89(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.C)); }
static auto cpu_op_0xCB8A(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.D)); }
static auto cpu_op_0xCB8B(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.E)); }
static auto cpu_op_0xCB8C(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.H)); }
static auto cpu_op_0xCB8D(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.L)); }
static auto cpu_op_0xCB8E(GB* gb) -> void { RES_b_mrr(gb, 1, gb->cpu.get_hl()); }
static auto cpu_op_0xCB8F(GB* gb) -> void { RES_b_r(gb, 1, &(gb->cpu.A)); }

static auto cpu_op_0xCB90(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.B)); }
static auto cpu_op_0xCB91(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.C)); }
static auto cpu_op_0xCB92(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.D)); }
static auto cpu_op_0xCB93(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.E)); }
static auto cpu_op_0xCB94(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.H)); }
static auto cpu_op_0xCB95(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.L)); }
static auto cpu_op_0xCB96(GB* gb) -> void { RES_b_mrr(gb, 2, gb->cpu.get_hl()); }
static auto cpu_op_0xCB97(GB* gb) -> void { RES_b_r(gb, 2, &(gb->cpu.A)); }
static auto cpu_op_0xCB98(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.B)); }
static auto cpu_op_0xCB99(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.C)); }
static auto cpu_op_0xCB9A(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.D)); }
static auto cpu_op_0xCB9B(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.E)); }
static auto cpu_op_0xCB9C(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.H)); }
static auto cpu_op_0xCB9D(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.L)); }
static auto cpu_op_0xCB9E(GB* gb) -> void { RES_b_mrr(gb, 3, gb->cpu.get_hl()); }
static auto cpu_op_0xCB9F(GB* gb) -> void { RES_b_r(gb, 3, &(gb->cpu.A)); }

static auto cpu_op_0xCBA0(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.B)); }
static auto cpu_op_0xCBA1(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.C)); }
static auto cpu_op_0xCBA2(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.D)); }
static auto cpu_op_0xCBA3(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.E)); }
static auto cpu_op_0xCBA4(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.H)); }
static auto cpu_op_0xCBA5(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.L)); }
static auto cpu_op_0xCBA6(GB* gb) -> void { RES_b_mrr(gb, 4, gb->cpu.get_hl()); }
static auto cpu_op_0xCBA7(GB* gb) -> void { RES_b_r(gb, 4, &(gb->cpu.A)); }
static auto cpu_op_0xCBA8(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.B)); }
static auto cpu_op_0xCBA9(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.C)); }
static auto cpu_op_0xCBAA(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.D)); }
static auto cpu_op_0xCBAB(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.E)); }
static auto cpu_op_0xCBAC(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.H)); }
static auto cpu_op_0xCBAD(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.L)); }
static auto cpu_op_0xCBAE(GB* gb) -> void { RES_b_mrr(gb, 5, gb->cpu.get_hl()); }
static auto cpu_op_0xCBAF(GB* gb) -> void { RES_b_r(gb, 5, &(gb->cpu.A)); }

static auto cpu_op_0xCBB0(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.B)); }
static auto cpu_op_0xCBB1(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.C)); }
static auto cpu_op_0xCBB2(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.D)); }
static auto cpu_op_0xCBB3(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.E)); }
static auto cpu_op_0xCBB4(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.H)); }
static auto cpu_op_0xCBB5(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.L)); }
static auto cpu_op_0xCBB6(GB* gb) -> void { RES_b_mrr(gb, 6, gb->cpu.get_hl()); }
static auto cpu_op_0xCBB7(GB* gb) -> void { RES_b_r(gb, 6, &(gb->cpu.A)); }
static auto cpu_op_0xCBB8(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.B)); }
static auto cpu_op_0xCBB9(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.C)); }
static auto cpu_op_0xCBBA(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.D)); }
static auto cpu_op_0xCBBB(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.E)); }
static auto cpu_op_0xCBBC(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.H)); }
static auto cpu_op_0xCBBD(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.L)); }
static auto cpu_op_0xCBBE(GB* gb) -> void { RES_b_mrr(gb, 7, gb->cpu.get_hl()); }
static auto cpu_op_0xCBBF(GB* gb) -> void { RES_b_r(gb, 7, &(gb->cpu.A)); }

static auto cpu_op_0xCBC0(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.B)); }
static auto cpu_op_0xCBC1(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.C)); }
static auto cpu_op_0xCBC2(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.D)); }
static auto cpu_op_0xCBC3(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.E)); }
static auto cpu_op_0xCBC4(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.H)); }
static auto cpu_op_0xCBC5(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.L)); }
static auto cpu_op_0xCBC6(GB* gb) -> void { SET_b_mrr(gb, 0, gb->cpu.get_hl()); }
static auto cpu_op_0xCBC7(GB* gb) -> void { SET_b_r(gb, 0, &(gb->cpu.A)); }
static auto cpu_op_0xCBC8(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.B)); }
static auto cpu_op_0xCBC9(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.C)); }
static auto cpu_op_0xCBCA(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.D)); }
static auto cpu_op_0xCBCB(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.E)); }
static auto cpu_op_0xCBCC(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.H)); }
static auto cpu_op_0xCBCD(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.L)); }
static auto cpu_op_0xCBCE(GB* gb) -> void { SET_b_mrr(gb, 1, gb->cpu.get_hl()); }
static auto cpu_op_0xCBCF(GB* gb) -> void { SET_b_r(gb, 1, &(gb->cpu.A)); }

static auto cpu_op_0xCBD0(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.B)); }
static auto cpu_op_0xCBD1(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.C)); }
static auto cpu_op_0xCBD2(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.D)); }
static auto cpu_op_0xCBD3(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.E)); }
static auto cpu_op_0xCBD4(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.H)); }
static auto cpu_op_0xCBD5(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.L)); }
static auto cpu_op_0xCBD6(GB* gb) -> void { SET_b_mrr(gb, 2, gb->cpu.get_hl()); }
static auto cpu_op_0xCBD7(GB* gb) -> void { SET_b_r(gb, 2, &(gb->cpu.A)); }
static auto cpu_op_0xCBD8(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.B)); }
static auto cpu_op_0xCBD9(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.C)); }
static auto cpu_op_0xCBDA(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.D)); }
static auto cpu_op_0xCBDB(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.E)); }
static auto cpu_op_0xCBDC(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.H)); }
static auto cpu_op_0xCBDD(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.L)); }
static auto cpu_op_0xCBDE(GB* gb) -> void { SET_b_mrr(gb, 3, gb->cpu.get_hl()); }
static auto cpu_op_0xCBDF(GB* gb) -> void { SET_b_r(gb, 3, &(gb->cpu.A)); }

static auto cpu_op_0xCBE0(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.B)); }
static auto cpu_op_0xCBE1(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.C)); }
static auto cpu_op_0xCBE2(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.D)); }
static auto cpu_op_0xCBE3(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.E)); }
static auto cpu_op_0xCBE4(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.H)); }
static auto cpu_op_0xCBE5(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.L)); }
static auto cpu_op_0xCBE6(GB* gb) -> void { SET_b_mrr(gb, 4, gb->cpu.get_hl()); }
static auto cpu_op_0xCBE7(GB* gb) -> void { SET_b_r(gb, 4, &(gb->cpu.A)); }
static auto cpu_op_0xCBE8(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.B)); }
static auto cpu_op_0xCBE9(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.C)); }
static auto cpu_op_0xCBEA(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.D)); }
static auto cpu_op_0xCBEB(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.E)); }
static auto cpu_op_0xCBEC(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.H)); }
static auto cpu_op_0xCBED(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.L)); }
static auto cpu_op_0xCBEE(GB* gb) -> void { SET_b_mrr(gb, 5, gb->cpu.get_hl()); }
static auto cpu_op_0xCBEF(GB* gb) -> void { SET_b_r(gb, 5, &(gb->cpu.A)); }

static auto cpu_op_0xCBF0(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.B)); }
static auto cpu_op_0xCBF1(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.C)); }
static auto cpu_op_0xCBF2(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.D)); }
static auto cpu_op_0xCBF3(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.E)); }
static auto cpu_op_0xCBF4(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.H)); }
static auto cpu_op_0xCBF5(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.L)); }
static auto cpu_op_0xCBF6(GB* gb) -> void { SET_b_mrr(gb, 6, gb->cpu.get_hl()); }
static auto cpu_op_0xCBF7(GB* gb) -> void { SET_b_r(gb, 6, &(gb->cpu.A)); }
static auto cpu_op_0xCBF8(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.B)); }
static auto cpu_op_0xCBF9(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.C)); }
static auto cpu_op_0xCBFA(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.D)); }
static auto cpu_op_0xCBFB(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.E)); }
static auto cpu_op_0xCBFC(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.H)); }
static auto cpu_op_0xCBFD(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.L)); }
static auto cpu_op_0xCBFE(GB* gb) -> void { SET_b_mrr(gb, 7, gb->cpu.get_hl()); }
static auto cpu_op_0xCBFF(GB* gb) -> void { SET_b_r(gb, 7, &(gb->cpu.A)); }

void (*cpu_optable[256])(GB* gb) =
{
  cpu_op_0x00,    cpu_op_0x01,    cpu_op_0x02,    cpu_op_0x03,    cpu_op_0x04,
  cpu_op_0x05,    cpu_op_0x06,    cpu_op_0x07,    cpu_op_0x08,    cpu_op_0x09,
  cpu_op_0x0A,    cpu_op_0x0B,    cpu_op_0x0C,    cpu_op_0x0D,    cpu_op_0x0E,
  cpu_op_0x0F,    cpu_op_0x10,    cpu_op_0x11,    cpu_op_0x12,    cpu_op_0x13,
  cpu_op_0x14,    cpu_op_0x15,    cpu_op_0x16,    cpu_op_0x17,    cpu_op_0x18,
  cpu_op_0x19,    cpu_op_0x1A,    cpu_op_0x1B,    cpu_op_0x1C,    cpu_op_0x1D,
  cpu_op_0x1E,    cpu_op_0x1F,    cpu_op_0x20,    cpu_op_0x21,    cpu_op_0x22,
  cpu_op_0x23,    cpu_op_0x24,    cpu_op_0x25,    cpu_op_0x26,    cpu_op_0x27,
  cpu_op_0x28,    cpu_op_0x29,    cpu_op_0x2A,    cpu_op_0x2B,    cpu_op_0x2C,
  cpu_op_0x2D,    cpu_op_0x2E,    cpu_op_0x2F,    cpu_op_0x30,    cpu_op_0x31,
  cpu_op_0x32,    cpu_op_0x33,    cpu_op_0x34,    cpu_op_0x35,    cpu_op_0x36,
  cpu_op_0x37,    cpu_op_0x38,    cpu_op_0x39,    cpu_op_0x3A,    cpu_op_0x3B,
  cpu_op_0x3C,    cpu_op_0x3D,    cpu_op_0x3E,    cpu_op_0x3F,    cpu_op_0x40,
  cpu_op_0x41,    cpu_op_0x42,    cpu_op_0x43,    cpu_op_0x44,    cpu_op_0x45,
  cpu_op_0x46,    cpu_op_0x47,    cpu_op_0x48,    cpu_op_0x49,    cpu_op_0x4A,
  cpu_op_0x4B,    cpu_op_0x4C,    cpu_op_0x4D,    cpu_op_0x4E,    cpu_op_0x4F,
  cpu_op_0x50,    cpu_op_0x51,    cpu_op_0x52,    cpu_op_0x53,    cpu_op_0x54,
  cpu_op_0x55,    cpu_op_0x56,    cpu_op_0x57,    cpu_op_0x58,    cpu_op_0x59,
  cpu_op_0x5A,    cpu_op_0x5B,    cpu_op_0x5C,    cpu_op_0x5D,    cpu_op_0x5E,
  cpu_op_0x5F,    cpu_op_0x60,    cpu_op_0x61,    cpu_op_0x62,    cpu_op_0x63,
  cpu_op_0x64,    cpu_op_0x65,    cpu_op_0x66,    cpu_op_0x67,    cpu_op_0x68,
  cpu_op_0x69,    cpu_op_0x6A,    cpu_op_0x6B,    cpu_op_0x6C,    cpu_op_0x6D,
  cpu_op_0x6E,    cpu_op_0x6F,    cpu_op_0x70,    cpu_op_0x71,    cpu_op_0x72,
  cpu_op_0x73,    cpu_op_0x74,    cpu_op_0x75,    cpu_op_0x76,    cpu_op_0x77,
  cpu_op_0x78,    cpu_op_0x79,    cpu_op_0x7A,    cpu_op_0x7B,    cpu_op_0x7C,
  cpu_op_0x7D,    cpu_op_0x7E,    cpu_op_0x7F,    cpu_op_0x80,    cpu_op_0x81,
  cpu_op_0x82,    cpu_op_0x83,    cpu_op_0x84,    cpu_op_0x85,    cpu_op_0x86,
  cpu_op_0x87,    cpu_op_0x88,    cpu_op_0x89,    cpu_op_0x8A,    cpu_op_0x8B,
  cpu_op_0x8C,    cpu_op_0x8D,    cpu_op_0x8E,    cpu_op_0x8F,    cpu_op_0x90,
  cpu_op_0x91,    cpu_op_0x92,    cpu_op_0x93,    cpu_op_0x94,    cpu_op_0x95,
  cpu_op_0x96,    cpu_op_0x97,    cpu_op_0x98,    cpu_op_0x99,    cpu_op_0x9A,
  cpu_op_0x9B,    cpu_op_0x9C,    cpu_op_0x9D,    cpu_op_0x9E,    cpu_op_0x9F,
  cpu_op_0xA0,    cpu_op_0xA1,    cpu_op_0xA2,    cpu_op_0xA3,    cpu_op_0xA4,
  cpu_op_0xA5,    cpu_op_0xA6,    cpu_op_0xA7,    cpu_op_0xA8,    cpu_op_0xA9,
  cpu_op_0xAA,    cpu_op_0xAB,    cpu_op_0xAC,    cpu_op_0xAD,    cpu_op_0xAE,
  cpu_op_0xAF,    cpu_op_0xB0,    cpu_op_0xB1,    cpu_op_0xB2,    cpu_op_0xB3,
  cpu_op_0xB4,    cpu_op_0xB5,    cpu_op_0xB6,    cpu_op_0xB7,    cpu_op_0xB8,
  cpu_op_0xB9,    cpu_op_0xBA,    cpu_op_0xBB,    cpu_op_0xBC,    cpu_op_0xBD,
  cpu_op_0xBE,    cpu_op_0xBF,    cpu_op_0xC0,    cpu_op_0xC1,    cpu_op_0xC2,
  cpu_op_0xC3,    cpu_op_0xC4,    cpu_op_0xC5,    cpu_op_0xC6,    cpu_op_0xC7,
  cpu_op_0xC8,    cpu_op_0xC9,    cpu_op_0xCA,    cpu_op_illegal, cpu_op_0xCC,
  cpu_op_0xCD,    cpu_op_0xCE,    cpu_op_0xCF,    cpu_op_0xD0,    cpu_op_0xD1,
  cpu_op_0xD2,    cpu_op_illegal, cpu_op_0xD4,    cpu_op_0xD5,    cpu_op_0xD6,
  cpu_op_0xD7,    cpu_op_0xD8,    cpu_op_0xD9,    cpu_op_0xDA,    cpu_op_illegal,
  cpu_op_0xDC,    cpu_op_illegal, cpu_op_0xDE,    cpu_op_0xDF,    cpu_op_0xE0,
  cpu_op_0xE1,    cpu_op_0xE2,    cpu_op_illegal, cpu_op_illegal, cpu_op_0xE5,
  cpu_op_0xE6,    cpu_op_0xE7,    cpu_op_0xE8,    cpu_op_0xE9,    cpu_op_0xEA,
  cpu_op_illegal, cpu_op_illegal, cpu_op_illegal, cpu_op_0xEE,    cpu_op_0xEF,
  cpu_op_0xF0,    cpu_op_0xF1,    cpu_op_0xF2,    cpu_op_0xF3,    cpu_op_illegal,
  cpu_op_0xF5,    cpu_op_0xF6,    cpu_op_0xF7,    cpu_op_0xF8,    cpu_op_0xF9,
  cpu_op_0xFA,    cpu_op_0xFB,    cpu_op_illegal, cpu_op_illegal, cpu_op_0xFE,
  cpu_op_0xFF,
};

void (*cpu_0xCB_optable[256])(GB* gb) =
{
  cpu_op_0xCB00, cpu_op_0xCB01, cpu_op_0xCB02, cpu_op_0xCB03, cpu_op_0xCB04,
  cpu_op_0xCB05, cpu_op_0xCB06, cpu_op_0xCB07, cpu_op_0xCB08, cpu_op_0xCB09,
  cpu_op_0xCB0A, cpu_op_0xCB0B, cpu_op_0xCB0C, cpu_op_0xCB0D, cpu_op_0xCB0E,
  cpu_op_0xCB0F, cpu_op_0xCB10, cpu_op_0xCB11, cpu_op_0xCB12, cpu_op_0xCB13,
  cpu_op_0xCB14, cpu_op_0xCB15, cpu_op_0xCB16, cpu_op_0xCB17, cpu_op_0xCB18,
  cpu_op_0xCB19, cpu_op_0xCB1A, cpu_op_0xCB1B, cpu_op_0xCB1C, cpu_op_0xCB1D,
  cpu_op_0xCB1E, cpu_op_0xCB1F, cpu_op_0xCB20, cpu_op_0xCB21, cpu_op_0xCB22,
  cpu_op_0xCB23, cpu_op_0xCB24, cpu_op_0xCB25, cpu_op_0xCB26, cpu_op_0xCB27,
  cpu_op_0xCB28, cpu_op_0xCB29, cpu_op_0xCB2A, cpu_op_0xCB2B, cpu_op_0xCB2C,
  cpu_op_0xCB2D, cpu_op_0xCB2E, cpu_op_0xCB2F, cpu_op_0xCB30, cpu_op_0xCB31,
  cpu_op_0xCB32, cpu_op_0xCB33, cpu_op_0xCB34, cpu_op_0xCB35, cpu_op_0xCB36,
  cpu_op_0xCB37, cpu_op_0xCB38, cpu_op_0xCB39, cpu_op_0xCB3A, cpu_op_0xCB3B,
  cpu_op_0xCB3C, cpu_op_0xCB3D, cpu_op_0xCB3E, cpu_op_0xCB3F, cpu_op_0xCB40,
  cpu_op_0xCB41, cpu_op_0xCB42, cpu_op_0xCB43, cpu_op_0xCB44, cpu_op_0xCB45,
  cpu_op_0xCB46, cpu_op_0xCB47, cpu_op_0xCB48, cpu_op_0xCB49, cpu_op_0xCB4A,
  cpu_op_0xCB4B, cpu_op_0xCB4C, cpu_op_0xCB4D, cpu_op_0xCB4E, cpu_op_0xCB4F,
  cpu_op_0xCB50, cpu_op_0xCB51, cpu_op_0xCB52, cpu_op_0xCB53, cpu_op_0xCB54,
  cpu_op_0xCB55, cpu_op_0xCB56, cpu_op_0xCB57, cpu_op_0xCB58, cpu_op_0xCB59,
  cpu_op_0xCB5A, cpu_op_0xCB5B, cpu_op_0xCB5C, cpu_op_0xCB5D, cpu_op_0xCB5E,
  cpu_op_0xCB5F, cpu_op_0xCB60, cpu_op_0xCB61, cpu_op_0xCB62, cpu_op_0xCB63,
  cpu_op_0xCB64, cpu_op_0xCB65, cpu_op_0xCB66, cpu_op_0xCB67, cpu_op_0xCB68,
  cpu_op_0xCB69, cpu_op_0xCB6A, cpu_op_0xCB6B, cpu_op_0xCB6C, cpu_op_0xCB6D,
  cpu_op_0xCB6E, cpu_op_0xCB6F, cpu_op_0xCB70, cpu_op_0xCB71, cpu_op_0xCB72,
  cpu_op_0xCB73, cpu_op_0xCB74, cpu_op_0xCB75, cpu_op_0xCB76, cpu_op_0xCB77,
  cpu_op_0xCB78, cpu_op_0xCB79, cpu_op_0xCB7A, cpu_op_0xCB7B, cpu_op_0xCB7C,
  cpu_op_0xCB7D, cpu_op_0xCB7E, cpu_op_0xCB7F, cpu_op_0xCB80, cpu_op_0xCB81,
  cpu_op_0xCB82, cpu_op_0xCB83, cpu_op_0xCB84, cpu_op_0xCB85, cpu_op_0xCB86,
  cpu_op_0xCB87, cpu_op_0xCB88, cpu_op_0xCB89, cpu_op_0xCB8A, cpu_op_0xCB8B,
  cpu_op_0xCB8C, cpu_op_0xCB8D, cpu_op_0xCB8E, cpu_op_0xCB8F, cpu_op_0xCB90,
  cpu_op_0xCB91, cpu_op_0xCB92, cpu_op_0xCB93, cpu_op_0xCB94, cpu_op_0xCB95,
  cpu_op_0xCB96, cpu_op_0xCB97, cpu_op_0xCB98, cpu_op_0xCB99, cpu_op_0xCB9A,
  cpu_op_0xCB9B, cpu_op_0xCB9C, cpu_op_0xCB9D, cpu_op_0xCB9E, cpu_op_0xCB9F,
  cpu_op_0xCBA0, cpu_op_0xCBA1, cpu_op_0xCBA2, cpu_op_0xCBA3, cpu_op_0xCBA4,
  cpu_op_0xCBA5, cpu_op_0xCBA6, cpu_op_0xCBA7, cpu_op_0xCBA8, cpu_op_0xCBA9,
  cpu_op_0xCBAA, cpu_op_0xCBAB, cpu_op_0xCBAC, cpu_op_0xCBAD, cpu_op_0xCBAE,
  cpu_op_0xCBAF, cpu_op_0xCBB0, cpu_op_0xCBB1, cpu_op_0xCBB2, cpu_op_0xCBB3,
  cpu_op_0xCBB4, cpu_op_0xCBB5, cpu_op_0xCBB6, cpu_op_0xCBB7, cpu_op_0xCBB8,
  cpu_op_0xCBB9, cpu_op_0xCBBA, cpu_op_0xCBBB, cpu_op_0xCBBC, cpu_op_0xCBBD,
  cpu_op_0xCBBE, cpu_op_0xCBBF, cpu_op_0xCBC0, cpu_op_0xCBC1, cpu_op_0xCBC2,
  cpu_op_0xCBC3, cpu_op_0xCBC4, cpu_op_0xCBC5, cpu_op_0xCBC6, cpu_op_0xCBC7,
  cpu_op_0xCBC8, cpu_op_0xCBC9, cpu_op_0xCBCA, cpu_op_0xCBCB, cpu_op_0xCBCC,
  cpu_op_0xCBCD, cpu_op_0xCBCE, cpu_op_0xCBCF, cpu_op_0xCBD0, cpu_op_0xCBD1,
  cpu_op_0xCBD2, cpu_op_0xCBD3, cpu_op_0xCBD4, cpu_op_0xCBD5, cpu_op_0xCBD6,
  cpu_op_0xCBD7, cpu_op_0xCBD8, cpu_op_0xCBD9, cpu_op_0xCBDA, cpu_op_0xCBDB,
  cpu_op_0xCBDC, cpu_op_0xCBDD, cpu_op_0xCBDE, cpu_op_0xCBDF, cpu_op_0xCBE0,
  cpu_op_0xCBE1, cpu_op_0xCBE2, cpu_op_0xCBE3, cpu_op_0xCBE4, cpu_op_0xCBE5,
  cpu_op_0xCBE6, cpu_op_0xCBE7, cpu_op_0xCBE8, cpu_op_0xCBE9, cpu_op_0xCBEA,
  cpu_op_0xCBEB, cpu_op_0xCBEC, cpu_op_0xCBED, cpu_op_0xCBEE, cpu_op_0xCBEF,
  cpu_op_0xCBF0, cpu_op_0xCBF1, cpu_op_0xCBF2, cpu_op_0xCBF3, cpu_op_0xCBF4,
  cpu_op_0xCBF5, cpu_op_0xCBF6, cpu_op_0xCBF7, cpu_op_0xCBF8, cpu_op_0xCBF9,
  cpu_op_0xCBFA, cpu_op_0xCBFB, cpu_op_0xCBFC, cpu_op_0xCBFD, cpu_op_0xCBFE,
  cpu_op_0xCBFF,
};

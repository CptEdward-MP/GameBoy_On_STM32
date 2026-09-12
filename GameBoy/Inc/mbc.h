#pragma once

typedef struct GB GB;
typedef struct rom_type rom_type;

// inline char serial_data[2];

extern u8 (*mbc_readbyte)(GB* gb, u16 address);
extern void (*mbc_writebyte)(GB* gb, u16 address, u8 byte);

void mbc_initialize_mbc_functions(rom_type i_rom_type);

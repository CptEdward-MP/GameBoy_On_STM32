#include "gb.h"

static f_inline auto mbc_nombc_readbyte(GB* gb, u16 address) -> u8
{
    return gb->cart.rom_data_ptr[address];
}

static auto mbc1_generic_readbyte(GB* gb, u16 address) -> u8
{
    auto& cart = gb->cart;

    if (address <= 0x3FFF)
    {
        /*
         * Banking_mode = 0
         */
        if (!cart.mbc.banking_mode)
            return cart.rom_data_ptr[address];

        /*
         * Banking_mode = 1
         */
        else
        {
            if (cart.mbc.is_rom_1mb)
            {
                u16 bank =
                    (cart.mbc.two_bits << 5) &
                    cart.mbc.max_rom_banks;

                return cart.rom_data_ptr[
                    (bank * 0x4000) + address
                ];
            }
            else
            {
                return cart.rom_data_ptr[address];
            }
        }
    }

    else if (address <= 0x7FFF)
    {
        u16 rom_bank =
            ((cart.mbc.is_rom_1mb) &&
             (!cart.mbc.contains_ram_banks))
                ?
                ((cart.mbc.two_bits << 5) |
                 cart.mbc.five_bits) &
                 cart.mbc.max_rom_banks
                :
                cart.mbc.five_bits;

        u32 offset =
            (static_cast<u32>(rom_bank) * 0x4000) +
            (address - 0x4000);

        return cart.rom_data_ptr[offset];
    }

    else
    {
        __builtin_unreachable();
    }
}

static auto mbc_mbc1_readbyte(GB* gb, u16 address) -> u8
{
    auto& cart = gb->cart;

    if (address <= 0x7FFF)
    {
        return mbc1_generic_readbyte(gb, address);
    }

    /* SRAM */
    else if (address <= 0xBFFF)
    {
        if ((!cart.rom_t.ram_t) ||
            (!cart.mbc.ram_enable))
        {
            return 0xFF;
        }

        u16 sram_bank = 0;

        if ((cart.mbc.contains_ram_banks) &&
            (cart.mbc.banking_mode == 1) &&
            (!cart.mbc.is_rom_1mb))
        {
            sram_bank = cart.mbc.two_bits & 0x03;
        }

        u32 offset =
            (static_cast<u32>(sram_bank) * 0x2000) +
            (address - 0xA000);

        return cart.sram_data[offset];
    }

    else
    {
        return 0xFF;
    }
}

static f_inline auto mbc_nombc_writebyte(
    GB*,
    u16,
    u8
) -> void
{
    return;
}

static auto mbc_mbc1_writebyte(
    GB* gb,
    u16 address,
    u8 byte
) -> void
{
    auto& cart = gb->cart;

    if (address <= 0x1FFF)
    {
        cart.mbc.ram_enable =
            ((byte & 0x0F) == 0x0A);
    }

    else if (address <= 0x3FFF)
    {
        u16 five_bits = byte & 0x1F;

        if (five_bits == 0)
            five_bits = 1;

        cart.mbc.five_bits =
            five_bits & cart.mbc.max_rom_banks;
    }

    else if (address <= 0x5FFF)
    {
        cart.mbc.two_bits = byte & 0x03;
    }

    else if (address <= 0x7FFF)
    {
        cart.mbc.banking_mode =
            static_cast<bool>(byte & 1);
    }

    else if (address <= 0xBFFF)
    {
        if (!cart.rom_t.ram_t)
            return;

        if (!cart.mbc.ram_enable)
            return;

        u16 sram_bank = 0;

        if ((cart.mbc.banking_mode == 1) &&
            (cart.mbc.contains_ram_banks) &&
            (!cart.mbc.is_rom_1mb))
        {
            sram_bank =
                cart.mbc.two_bits & 0x03;
        }

        u32 offset =
            (static_cast<u32>(sram_bank) * 0x2000) +
            (address - 0xA000);

        cart.sram_data[offset] = byte;
    }

    else
    {
        return;
    }
}


/*
 * MBC function pointers
 */
u8 (*mbc_readbyte)(GB* gb, u16 address);

void (*mbc_writebyte)(
    GB* gb,
    u16 address,
    u8 byte
);


/*
 * Select the appropriate MBC implementation.
 */
void mbc_initialize_mbc_functions(
    rom_type i_rom_type
)
{
    switch (i_rom_type.mbc_t)
    {
        case 0:
            mbc_readbyte =
                mbc_nombc_readbyte;

            mbc_writebyte =
                mbc_nombc_writebyte;
            break;

        case 1:
            mbc_readbyte =
                mbc_mbc1_readbyte;

            mbc_writebyte =
                mbc_mbc1_writebyte;
            break;

        /*
         * Future MBC implementations:
         *
         * case 2:
         * case 3:
         * case 4:
         */
    }
}

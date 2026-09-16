# `gb.h` — Game Boy Core State Reference

## Purpose
`gb.h` defines the main state of the Game Boy emulator.

Mental model:

```text
GB
├── CPU
├── MMU
├── CART
├── PPU
├── TIMER
└── JOYPAD
```

The `GB` object represents the state of the emulated Game Boy.

## 1. CPU State

Stores:
- CPU registers `A, B, C, D, E, H, L, F`
- `SP` and `PC`
- register-pair state such as `AF`, `BC`, `DE`, `HL`
- halt / halt-bug state
- interrupt master enable (`ime`)
- delayed interrupt enable
- interrupt flags
- instruction state
- cycles passed
- instruction register (`IR`)
- CB-prefix state
- joypad/interrupt-related state

The CPU structure is therefore more than just registers: it also tracks CPU execution state.

## 2. MMU

Represents Game Boy memory-related state:

```text
VRAM
WRAM
OAM
IO
HRAM
```

It also contains the framebuffer:

```cpp
u16 gb_framebuffer[23040];
```

Since:

```text
160 × 144 = 23040 pixels
23040 × 2 = 46080 bytes
```

the framebuffer is about 46 KB.

## 3. Cartridge (`CART`)

Contains cartridge-related state such as:
- ROM data
- SRAM
- boot ROM
- MBC information
- ROM header
- cartridge type
- ROM/file size

For the STM32 version, ROM can be stored in Flash and accessed through a pointer rather than copied into RAM.

## 4. PPU

The PPU state includes:
- LCD/PPU modes
- scanline timing
- OAM
- background/window FIFOs
- sprite FIFO
- fetcher state
- framebuffer drawing
- scrolling
- sprite data

Mental model:

```text
PPU
├── timing/state
├── OAM data
├── background FIFO
├── sprite FIFO
├── fetcher state
└── drawing state
```

## 5. Timer

Contains:
- `SYSCLK`
- double-speed state
- reload state
- timer enable/signal state
- selected timer clock bit

## 6. Joypad

Represents the Game Boy input state, including:
- D-pad
- buttons

## 7. The `GB` Object

The top-level structure combines the subsystems:

```text
                  GB
                   │
      ┌────────────┼────────────┐
      │            │            │
     CPU          MMU          CART
      │            │
      │            └── framebuffer
      │
      ├────────────┐
      │            │
     PPU         TIMER
      │
      └── JOYPAD
```

### Important takeaway

Do **not** think of `GB` as the thing that "runs the Game Boy."

Think of it primarily as:

> **the complete state of the emulated Game Boy hardware.**

Functions operate on this state to produce the behavior.

## STM32 Optimization Notes — Curiosity Only

Possible future areas to investigate:
1. Memory layout of the large `GB` state.
2. Avoiding framebuffer copies.
3. Frequently accessed CPU, timer, PPU and MMU state.
4. Repeated access to nested PPU structures.

These are hypotheses, **not proven bottlenecks**. Profiling should determine what is worth changing.

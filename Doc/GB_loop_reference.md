# `gb_step.cc` — Game Boy Timing / Hardware Step Reference

## Purpose

The key discovery is:

> **`GB::gb_step()` is not the function that executes a Game Boy CPU instruction.**

It advances emulated hardware time.

```text
GB::gb_step()
│
├── ticks += 4
├── gb_ppu_step()
│   ├── update PPU timing/mode
│   └── handle OAM DMA
└── gb_timer_step()
    ├── advance system clock
    ├── handle TIMA reload
    └── update TIMA edge behavior
```

## 1. `GB::gb_step()`

Conceptually:

```cpp
auto GB::gb_step() -> void
{
    ticks += 4;
    gb_ppu_step(this);
    gb_timer_step(this);
}
```

One call represents a small amount of Game Boy hardware time:

```text
one gb_step()
    ↓
4 timing units
```

The CPU instruction execution logic calls into this mechanism while emulating instructions.

## 2. PPU Timing

`gb_ppu_step()` performs:

```text
gb_ppu_step()
├── ppu_update_modes()
└── ppu_oam_dma_check()
```

## 3. PPU Mode Selection

Conceptually:

```text
LCD OFF?
 └── reset relevant PPU timing/state

LCD ON?
 ├── LY >= 144 → VBLANK
 │
 └── LY < 144
      ├── dots < 80 → OAM
      ├── LX < 160 → DRAWING
      └── otherwise → HBLANK
```

Visible resolution:

```text
160 × 144 pixels
```

A scanline is one horizontal row of 160 visible pixels.

## 4. Dots vs Pixels

**Dots are timing units, not pixels.**

A normal scanline is approximately:

```text
456 dots
```

while the visible width is:

```text
160 pixels
```

So:

```text
dots ≠ pixels
```

`LX` tracks visible drawing position; `dots` tracks PPU timing.

## 5. Scanline Structure

Conceptually:

```text
OAM        DRAWING                 HBLANK
|----------|------------------------|
0          ~80                      456 dots
```

VBlank uses:

```text
LY = 144 ... 153
```

then the frame wraps to:

```text
LY = 0
```

## 6. OAM Scan

During OAM processing, the implementation checks sprite entries for the current scanline.

Game Boy constraint:

```text
Maximum 10 sprites per scanline
```

The implementation tracks consumed OAM entries and sprite-fetch state.

This became important during STM32 profiling because the sprite-check loop was expensive.

## 7. Drawing Mode

Drawing performs pixel-generation work using:
- background FIFO
- sprite FIFO
- fetcher
- window
- scrolling
- framebuffer

Pixels eventually reach:

```cpp
gb->mmu.gb_framebuffer
```

## 8. HBlank

At the end of a visible scanline, HBlank updates/resets state needed for the next scanline, including window, OAM and sprite state.

## 9. VBlank

When:

```text
LY >= 144
```

the PPU is in VBlank.

The implementation requests the VBlank interrupt at VBlank entry.

After the VBlank lines, LY wraps to zero.

## 10. PPU Timing Flow

```text
dots increase
      ↓
check LY
      ↓
determine PPU mode
      ↓
perform mode-specific work
      ↓
increment dots
      ↓
scanline limit reached?
      │
      ├── no → continue
      └── yes
            ↓
         dots = 0
            ↓
          LY++
```

The implementation uses:
- `452` dots for the first scanline
- `456` dots for subsequent scanlines

## 11. OAM DMA

`ppu_oam_dma_check()` handles OAM DMA behavior and models transfer into the PPU OAM region.

## 12. Timer

`gb_timer_step()` roughly performs:

```text
SYSCLK += 4
      ↓
derive DIV
      ↓
check TAC
      ↓
handle TIMA reload
      ↓
update TIMA
```

## 13. TIMA Edge Detection

`gb_update_tima()` models the timer using the selected system-clock bit.

The timer signal is based on:

```text
Timer Enable AND selected SYSCLK bit
```

The implementation detects a falling edge.

On the appropriate falling edge:

```text
TIMA++
```

If TIMA was `0xFF`, overflow/reload behavior is handled using the timer reload state and TMA, with the timer interrupt requested as appropriate.

## 14. Why This File Matters

It is tempting to think:

```text
gb_step() = execute one CPU instruction
```

That is incorrect.

Better model:

```text
CPU execution
      │
      │ advances time
      ▼
   gb_step()
      │
      ├── PPU advances
      └── Timer advances
```

So `gb_step()` is a **hardware clock/timing advancement primitive**.

## 15. STM32 Profiling Notes

Clean profiling baseline from the current investigation:

```text
Total              ~77.34 M cycles
                   ~1289 ms/frame @ 60 MHz

BG FIFO             ~5.30 M cycles
                   ~88 ms

Sprite Scan        ~20.04 M cycles
                  ~334 ms

Sprite FIFO          0

Framebuffer         ~11.23 M cycles
                  ~187 ms

Drawing calls       24,198
```

Removing the entire sprite-check loop reduced the measured frame from roughly:

```text
~1289 ms → ~940 ms
```

This confirms that the sprite-check loop is an expensive region in the current STM32 build.

However, removing it changes emulator behavior, so this was a **diagnostic experiment**, not a final optimization.

## 16. Optimization Lesson

Replacing one suspected expensive operation (`get_sp_byte()`) with a constant did not produce a meaningful improvement.

Removing the entire loop did.

Therefore:

```text
measure
  ↓
change one thing
  ↓
measure again
  ↓
identify the actual bottleneck
```

Do not optimize merely because a line looks expensive.

## 17. Future Optimization Ideas — Not Yet Implemented

Possible investigation areas:
- PPU mode/timing checks
- repeated function calls in hot paths
- repeated nested state accesses
- timer overhead

All require profiling and correctness testing before changing the implementation.

## Final Mental Model

```text
                 GAME BOY EMULATOR
                        │
                  CPU executes
                        │
                 advances time
                        │
                    gb_step()
                        │
              ┌─────────┴─────────┐
              │                   │
             PPU                TIMER
              │                   │
       pixels / scanlines      TIMA / DIV
              │
              ▼
        framebuffer
```

Remember:

```text
gb_step()
≠
CPU instruction

gb_step()
=
advance emulated hardware time
```

The next useful step is to trace **who calls `gb_step()` and how it relates to actual CPU instruction execution**.

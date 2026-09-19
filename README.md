# RoToBoy

A GameBoy (DMG) hardware emulator written in C using SDL2.

RoToBoy emulates the CPU, graphics processor, memory bus, audio system, and input controllers required to run classic GameBoy games smoothly.

## Gallery & Compatibility

RoToBoy runs homebrew titles smoothly using SDL2 for hardware-accelerated graphics and audio output.

| *DanganGB* | *Spiritfall* | *Tobu Tobu Girl* |
| :---: | :---: | :---: |
| <img src="./assets/dangan.png" width="240" alt="DanganGB running on RoToBoy"/> | <img src="./assets/spiritfall.png" width="240" alt="Spiritfall running on RoToBoy"/> | <img src="./assets/tobutobu.png" width="240" alt="Tobu Tobu Girl running on RoToBoy"/> |
| **MBC None** • Bullet Hell | **MBC1** • Tower Defense | **MBC1** • Arcade Platformer |

## Features

* **CPU Core:** Complete 8-bit instruction set decoder, timing evaluation, and interrupt system.
* **Graphics & Windowing:** PPU implementation rendered via SDL2, supporting background tile maps, window overlays, and sprite layers.
* **Audio Processing:** APU sound generation implemented via SDL2 audio streams.
* **Memory Management:** Centralized MMU handling memory-mapped I/O, cartridge banking, and direct memory transfers.
* **Input & Timing:** Hardware timer implementation and responsive joypad input polling.

## Project Structure

The project is modularly structured into clear C subsystem components:

```text
RoToBoy/
├── src/
│   ├── main.c        # Main loop & SDL initialization
│   ├── cpu.c         # Sharp LR35902 CPU implementation
│   ├── mmu.c         # Memory Management Unit & I/O bus
│   ├── ppu.c         # Pixel Processing Unit (graphics)
│   ├── apu.c         # Audio Processing Unit
│   ├── joypad.c      # Input handling & button mapping
│   ├── timer.c       # Hardware timers & divider registers
│   ├── cartridge.c   # ROM parsing & memory bank control
│   └── window.c      # SDL window rendering interface
└── includes/         # Header definitions for subsystems
```

## Controls & Key Mapping

| GameBoy Button | Key |
| :--- | :--- |
| D-Pad (Up, Down, Left, Right) | Arrow Keys |
| A | Z |
| B | X |
| Start | Enter |
| Select | Backspace |

## Building & Running

### Prerequisites

Ensure you have a C compiler, CMake, and SDL2 development libraries installed on your system.

On Ubuntu/Debian:
```bash
sudo apt install build-essential cmake libsdl2-dev
```

### Build Commands

```bash
# Clone the repository
git clone https://github.com/your-username/RoToBoy.git
cd RoToBoy

# Build the project
mkdir build && cd build
cmake ../
cmake --build ./

# Run a ROM
./RoToBoy path/to/rom.gb
```

## Disclaimer

*RoToBoy is an open-source educational project created for emulation research. Game Boy is a registered trademark of Nintendo Co., Ltd. RoToBoy is not affiliated with or endorsed by Nintendo. All screenshots shown in this repository feature open-source, publicly distributed homebrew software.*

# RoToBoy

A GameBoy (DMG) hardware emulator written from scratch.

RoToBoy emulates the CPU, graphics processor, memory bus, and input systems required to run classic GameBoy games and diagnostic test suites.

## Gallery & Compatibility

RoToBoy runs homebrew titles smoothly while passing standard hardware test suites.

| *DanganGB* | *Spiritfall* | *Tobu Tobu Girl* | 
| ----- | ----- | ----- | 
| <img src="./assets/dangan.png" width="240" alt="DanganGB running on RoToBoy"/> | <img src="./assets/spiritfall.png" width="240" alt="Spiritfall running on RoToBoy"/> | <img src="./assets/tobutobu.png" width="240" alt="Tobu Tobu Girl running on RoToBoy"/> | 
| **MBC None** • Bullet Hell | **MBC1** • Tower Defense | **MBC1** • Arcade Platformer | 

## Features

* **CPU & Memory:** Complete implementation of the 8-bit CPU instruction set, timing mechanics, and memory mapping.
* **Graphics Engine:** Rendering engine handling background, window, and sprite layers.
* **Audio & Timing:** System timers and hardware interrupt handling for game loops.
* **Compatibility:** Passes key diagnostic test suites to ensure proper hardware emulation.

## Controls & Key Mapping

| GameBoy Button | Key |
| --- | --- |
| D-Pad (Up, Down, Left, Right) | Arrow Keys |
| A | Z |
| B | X |
| Start | Enter |
| Select | Backspace |

## Hardware Test Suite Compatibility

RoToBoy passes standardized GameBoy accuracy test ROMs:

* [x] **Blargg's CPU Instruction Tests** (`cpu_instrs.gb`)
* [x] **Blargg's Instruction Timing** (`instr_timing.gb`)
* [x] **Mooneye GB Test Suite**
* [x] **`dmg-acid2`** (PPU rendering accuracy)

## Building & Running

### Prerequisites

Ensure you have C++ build tools and CMake installed along with any required graphical libraries.

```bash
# Clone the repository
git clone https://github.com/your-username/RoToBoy.git
cd RoToBoy

# Build the project
mkdir build && cd build
cmake ../
cmake --build ./

# Run a ROM
./rotoboy path/to/rom.gb
```

## Disclaimer

*RoToBoy is an open-source educational project created for emulation research. Game Boy is a registered trademark of Nintendo Co., Ltd. RoToBoy is not affiliated with or endorsed by Nintendo. All screenshots shown in this repository feature open-source, publicly distributed homebrew software.*
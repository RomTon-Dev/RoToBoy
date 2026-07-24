import ctypes
import glob
import json
import os
import pytest


# ==========================================
# 1. CTYPES STRUCTURE DEFINITIONS
# ==========================================
class Cartridge(ctypes.Structure):
    _fields_ = [
        ("rom_data", ctypes.POINTER(ctypes.c_uint8)),
        ("rom_size", ctypes.c_uint32),
        ("eram_data", ctypes.POINTER(ctypes.c_uint8)),
        ("eram_size", ctypes.c_uint32),
        ("mbc_type", ctypes.c_uint8),
        ("current_rom_bank", ctypes.c_uint8),
        ("current_ram_bank", ctypes.c_uint8),
        ("ram_enabled", ctypes.c_bool),
    ]


class MMU(ctypes.Structure):
    _fields_ = [
        ("raw_memory", ctypes.POINTER(ctypes.c_uint8)),
        ("test_mode", ctypes.c_bool),
        ("cart", Cartridge),
        ("boot_rom", ctypes.c_uint8 * 256),
        ("wram", ctypes.c_uint8 * 0x2000),
        ("hram", ctypes.c_uint8 * 0x7F),
        ("boot_rom_mapped", ctypes.c_bool),
        ("dma_source_high", ctypes.c_uint8),
        ("if_register", ctypes.c_uint8),
        ("ie_register", ctypes.c_uint8),
        ("dma_active", ctypes.c_bool),
        ("dma_byte", ctypes.c_uint8),
        ("dma_delay", ctypes.c_uint8),
        ("dma_source_address", ctypes.c_uint8),
    ]


class _AF_Struct(ctypes.Structure):
    _fields_ = [("f", ctypes.c_uint8), ("a", ctypes.c_uint8)]


class _BC_Struct(ctypes.Structure):
    _fields_ = [("c", ctypes.c_uint8), ("b", ctypes.c_uint8)]


class _DE_Struct(ctypes.Structure):
    _fields_ = [("e", ctypes.c_uint8), ("d", ctypes.c_uint8)]


class _HL_Struct(ctypes.Structure):
    _fields_ = [("l", ctypes.c_uint8), ("h", ctypes.c_uint8)]


class _AF_Union(ctypes.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _AF_Struct), ("af", ctypes.c_uint16)]


class _BC_Union(ctypes.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _BC_Struct), ("bc", ctypes.c_uint16)]


class _DE_Union(ctypes.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _DE_Struct), ("de", ctypes.c_uint16)]


class _HL_Union(ctypes.Union):
    _anonymous_ = ("_s",)
    _fields_ = [("_s", _HL_Struct), ("hl", ctypes.c_uint16)]


class CPU(ctypes.Structure):
    _anonymous_ = ("_u_af", "_u_bc", "_u_de", "_u_hl")
    _fields_ = [
        ("_u_af", _AF_Union),
        ("_u_bc", _BC_Union),
        ("_u_de", _DE_Union),
        ("_u_hl", _HL_Union),
        ("sp", ctypes.c_uint16),
        ("pc", ctypes.c_uint16),
        ("ir", ctypes.c_uint8),
        ("master_interrupt_enable", ctypes.c_bool),
        ("ime_delay", ctypes.c_uint8),
        ("halted", ctypes.c_bool),
        ("stopped", ctypes.c_bool),
        ("mmu", ctypes.POINTER(MMU)),
    ]


# ==========================================
# 2. LOAD C LIBRARY & SIGNATURES
# ==========================================
lib = ctypes.CDLL(os.path.abspath("./test_data/libMyProjectLib.so"))

lib.cpu_init.argtypes = [ctypes.POINTER(CPU), ctypes.POINTER(MMU)]
lib.cpu_step.argtypes = [ctypes.POINTER(CPU)]
lib.cpu_step.restype = ctypes.c_uint8

# Matches C signature: bus_read(MMU*, uint16_t, bool)
lib.bus_read.argtypes = [
    ctypes.POINTER(MMU),
    ctypes.c_uint16,
    ctypes.c_bool,
]
lib.bus_read.restype = ctypes.c_uint8

lib.bus_write.argtypes = [
    ctypes.POINTER(MMU),
    ctypes.c_uint16,
    ctypes.c_uint8,
    ctypes.c_bool,
]


# ==========================================
# 3. TEST HELPERS
# ==========================================
def load_json_tests():
    # """
    test_files = sorted(glob.glob("test_data/cpu_test_data/*.json"))
    all_tests = []
    for file in test_files:
        with open(file, "r") as f:
            all_tests.extend(json.load(f))
    return all_tests
    # """

    # Target 00.json (NOP) or 01.json (LD BC, u16)

    """
    base_dir = os.path.dirname(os.path.abspath(__file__))
    target_file = os.path.join(base_dir, "test_data", "cpu_test_data", "93.json")

    if not os.path.exists(target_file):
        print(f"\n[WARNING] File not found: {target_file}")
        return []

    with open(target_file, "r") as f:
        return json.load(f)
    """


def inject_test_ram(mmu, address, value):
    """Directly populates memory buffers bypassing hardware write logic."""
    if address < 0x8000:
        if mmu.cart.rom_data:
            mmu.cart.rom_data[address] = value
    elif 0x8000 <= address <= 0x9FFF:
        mmu.vram[address - 0x8000] = value
    elif 0xA000 <= address <= 0xBFFF:
        if mmu.cart.eram_data:
            mmu.cart.eram_data[address - 0xA000] = value
    elif 0xC000 <= address <= 0xDFFF:
        mmu.wram[address - 0xC000] = value
    elif 0xE000 <= address <= 0xFDFF:
        mmu.wram[address - 0xE000] = value
    elif 0xFE00 <= address <= 0xFE9F:
        mmu.oam[address - 0xFE00] = value
    elif 0xFF80 <= address <= 0xFFFE:
        mmu.hram[address - 0xFF80] = value
    elif address == 0xFFFF:
        mmu.ie_register = value


# ==========================================
# 4. PYTEST HARNESS
# ==========================================
ALL_TESTS = load_json_tests()


# 1. Reuse the memory block and CPU/MMU instances across tests
@pytest.fixture(scope="module")
def emulator():
    mmu = MMU()
    cpu = CPU()
    # Allocate the 64 KiB block ONCE in memory
    memory_block = (ctypes.c_uint8 * 0x10000)()
    mmu.raw_memory = ctypes.cast(memory_block, ctypes.POINTER(ctypes.c_uint8))

    return cpu, mmu, memory_block


# 2. Pass the fixture into your test
@pytest.mark.parametrize("test_case", ALL_TESTS, ids=lambda tc: tc["name"])
def test_cpu_instruction(test_case, emulator):
    cpu, mmu, memory_block = emulator

    # Fast zero-out of existing RAM without re-allocating
    ctypes.memset(memory_block, 0, ctypes.sizeof(memory_block))
    ctypes.memset(ctypes.byref(cpu), 0, ctypes.sizeof(CPU))

    lib.cpu_init(ctypes.byref(cpu), ctypes.byref(mmu))
    mmu.test_mode = True

    initial = test_case["initial"]

    # 1. Inject RAM State FIRST
    for addr, val in initial["ram"]:
        memory_block[addr] = val

    # 2. Setup Registers
    cpu.a = initial["a"]
    cpu.f = initial["f"] & 0xF0
    cpu.b = initial["b"]
    cpu.c = initial["c"]
    cpu.d = initial["d"]
    cpu.e = initial["e"]
    cpu.h = initial["h"]
    cpu.l = initial["l"]
    cpu.sp = initial["sp"]

    # 3. Setup PC and prime the pipeline IR
    cpu.pc = initial["pc"]

    # --- EXECUTE INSTRUCTION ---
    cpu.ir = lib.bus_read(ctypes.byref(mmu), cpu.pc, True)  # fetch instruction
    cpu.pc += 1
    lib.cpu_step(ctypes.byref(cpu))

    # --- ASSERT FINAL STATE ---
    final = test_case["final"]
    name = test_case["name"]

    assert cpu.a == final["a"], (
        f"Reg A mismatch in {name}. Expected 0x{final['a']:02X}, got 0x{cpu.a:02X}"
    )
    assert (cpu.f & 0xF0) == (final["f"] & 0xF0), (
        f"Reg F mismatch in {name}. Expected 0x{final['f']:02X}, got 0x{cpu.f:02X}"
    )
    assert cpu.b == final["b"], (
        f"Reg B mismatch in {name}. Expected 0x{final['b']:02X}, got 0x{cpu.b:02X}"
    )
    assert cpu.c == final["c"], (
        f"Reg C mismatch in {name}. Expected 0x{final['c']:02X}, got 0x{cpu.c:02X}"
    )
    assert cpu.d == final["d"], (
        f"Reg D mismatch in {name}. Expected 0x{final['d']:02X}, got 0x{cpu.d:02X}"
    )
    assert cpu.e == final["e"], (
        f"Reg E mismatch in {name}. Expected 0x{final['e']:02X}, got 0x{cpu.e:02X}"
    )
    assert cpu.h == final["h"], (
        f"Reg H mismatch in {name}. Expected 0x{final['h']:02X}, got 0x{cpu.h:02X}"
    )
    assert cpu.l == final["l"], (
        f"Reg L mismatch in {name}. Expected 0x{final['l']:02X}, got 0x{cpu.l:02X}"
    )
    assert cpu.sp == final["sp"], (
        f"SP mismatch in {name}. Expected 0x{final['sp']:04X}, got 0x{cpu.sp:04X}"
    )
    assert ((cpu.pc - 1) & 0xFFFF) == final["pc"], (
        f"PC mismatch in {name}. Expected 0x{final['pc']:04X}, got 0x{cpu.pc:04X}"
    )

    # Verify RAM mutations
    for addr, val in final["ram"]:
        actual_val = lib.bus_read(ctypes.byref(mmu), addr, False)
        assert actual_val == val, (
            f"RAM mismatch at 0x{addr:04X}. Expected 0x{val:02X}, got 0x{actual_val:02X} in {name}"
        )

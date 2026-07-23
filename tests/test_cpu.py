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
        ("cart", Cartridge),
        ("vram", ctypes.c_uint8 * 0x2000),
        ("wram", ctypes.c_uint8 * 0x2000),
        ("oam", ctypes.c_uint8 * 0xA0),
        ("hram", ctypes.c_uint8 * 0x7F),
        ("ie_register", ctypes.c_uint8),
        ("boot_rom_mapped", ctypes.c_bool),
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
        ("master_interrupt_enable", ctypes.c_bool),
        ("halted", ctypes.c_bool),
        ("stopped", ctypes.c_bool),
        ("mmu", ctypes.POINTER(MMU)),
        # Add 'ir' or 'halt_bug' here if present in C struct!
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
    # test_files = glob.glob("test_data/cpu_test_data/*.json")
    # all_tests = []
    # for file in test_files:
    #     with open(file, "r") as f:
    #         all_tests.extend(json.load(f))
    # return all_tests

    base_dir = os.path.dirname(os.path.abspath(__file__))

    # CHANGE THIS: Only load the opcode file you're working on
    # Target 00.json (NOP) or 01.json (LD BC, u16)
    target_file = os.path.join(base_dir, "test_data", "cpu_test_data", "00.json")

    if not os.path.exists(target_file):
        print(f"\n[WARNING] File not found: {target_file}")
        return []

    with open(target_file, "r") as f:
        return json.load(f)


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


@pytest.mark.parametrize("test_case", ALL_TESTS, ids=lambda tc: tc["name"])
def test_cpu_instruction(test_case):
    mmu = MMU()
    cpu = CPU()

    # Allocate both ROM and ERAM buffers
    mock_rom = (ctypes.c_uint8 * 0x8000)()
    mock_eram = (ctypes.c_uint8 * 0x8000)()
    mmu.cart.rom_data = ctypes.cast(mock_rom, ctypes.POINTER(ctypes.c_uint8))
    mmu.cart.eram_data = ctypes.cast(mock_eram, ctypes.POINTER(ctypes.c_uint8))
    mmu.cart.rom_size = 0x8000
    mmu.cart.eram_size = 0x8000
    mmu.cart.mbc_type = 0

    lib.cpu_init(ctypes.byref(cpu), ctypes.byref(mmu))

    initial = test_case["initial"]

    # 1. Inject RAM State FIRST
    for addr, val in initial["ram"]:
        inject_test_ram(mmu, addr, val)

    # 2. Setup Registers
    cpu.a = initial["a"]
    cpu.f = initial["f"] & 0xF0
    cpu.b = initial["b"]
    cpu.c = initial["c"]
    cpu.d = initial["d"]
    cpu.e = initial["e"]
    cpu.h = initial["h"]
    cpu.l = initial["l"]
    cpu.pc = initial["pc"]
    cpu.sp = initial["sp"]

    # --- EXECUTE INSTRUCTION ---
    cycles_taken = lib.cpu_step(ctypes.byref(cpu))

    # --- ASSERT FINAL STATE ---
    final = test_case["final"]
    name = test_case["name"]

    assert cpu.a == final["a"], f"Reg A mismatch in {name}"
    assert (cpu.f & 0xF0) == (final["f"] & 0xF0), f"Reg F mismatch in {name}"
    assert cpu.b == final["b"], f"Reg B mismatch in {name}"
    assert cpu.c == final["c"], f"Reg C mismatch in {name}"
    assert cpu.d == final["d"], f"Reg D mismatch in {name}"
    assert cpu.e == final["e"], f"Reg E mismatch in {name}"
    assert cpu.h == final["h"], f"Reg H mismatch in {name}"
    assert cpu.l == final["l"], f"Reg L mismatch in {name}"
    assert cpu.pc == final["pc"], f"PC mismatch in {name}"
    assert cpu.sp == final["sp"], f"SP mismatch in {name}"

    # Verify RAM mutations
    for addr, val in final["ram"]:
        actual_val = lib.bus_read(ctypes.byref(mmu), addr, False)
        assert actual_val == val, (
            f"RAM mismatch at 0x{addr:04X}. Expected 0x{val:02X}, got 0x{actual_val:02X} in {name}"
        )

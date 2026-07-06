import ctypes
import json
import pytest
import glob
import os


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
    ]


# ==========================================
# 2. LOAD C LIBRARY & SIGNATURES
# ==========================================
lib = ctypes.CDLL(os.path.abspath("./test_data/libMyProjectLib.so"))

lib.cpu_init.argtypes = [ctypes.POINTER(CPU), ctypes.POINTER(MMU)]
lib.cpu_step.argtypes = [ctypes.POINTER(CPU)]
lib.cpu_step.restype = ctypes.c_uint8  # M-cycles taken

lib.bus_read.argtypes = [ctypes.POINTER(MMU), ctypes.c_uint16]
lib.bus_read.restype = ctypes.c_uint8
lib.bus_write.argtypes = [ctypes.POINTER(MMU), ctypes.c_uint16, ctypes.c_uint8]


# ==========================================
# 3. TEST HELPERS
# ==========================================
def load_json_tests():
    # TIP: For a repo with 500k tests, only put the opcodes you are
    # currently working on (e.g., "00.json") in this folder to speed up Pytest!
    test_files = glob.glob("test_data/cpu_test_data/*.json")
    all_tests = []
    for file in test_files:
        with open(file, "r") as f:
            all_tests.extend(json.load(f))
    return all_tests


def inject_test_ram(mmu, address, value):
    """Bypasses normal bus logic to force test data into memory."""
    if address < 0x8000:
        if mmu.cart.rom_data:
            mmu.cart.rom_data[address] = value
    elif 0x8000 <= address <= 0x9FFF:
        mmu.vram[address - 0x8000] = value
    elif 0xC000 <= address <= 0xDFFF:
        mmu.wram[address - 0xC000] = value
    elif 0xFE00 <= address <= 0xFE9F:
        mmu.oam[address - 0xFE00] = value
    elif 0xFF80 <= address <= 0xFFFE:
        mmu.hram[address - 0xFF80] = value
    else:
        lib.bus_write(ctypes.byref(mmu), address, value)


# ==========================================
# 4. PYTEST HARNESS
# ==========================================
ALL_TESTS = load_json_tests()


@pytest.mark.parametrize("test_case", ALL_TESTS, ids=lambda tc: tc["name"])
def test_cpu_instruction(test_case):
    mmu = MMU()
    cpu = CPU()

    # Mock a 32KB ROM Array
    mock_rom = (ctypes.c_uint8 * 0x8000)()
    mmu.cart.rom_data = ctypes.cast(mock_rom, ctypes.POINTER(ctypes.c_uint8))
    mmu.cart.rom_size = 0x8000
    mmu.cart.mbc_type = 0

    lib.cpu_init(ctypes.byref(cpu), ctypes.byref(mmu))

    # --- SETUP INITIAL STATE ---
    initial = test_case["initial"]
    cpu.a = initial["a"]
    cpu.f = initial["f"]
    cpu.b = initial["b"]
    cpu.c = initial["c"]
    cpu.d = initial["d"]
    cpu.e = initial["e"]
    cpu.h = initial["h"]
    cpu.l = initial["l"]
    cpu.pc = initial["pc"]
    cpu.sp = initial["sp"]

    # SingleStepTests/sm83 Specific Additions
    # if "ime" in initial:
    #    cpu.master_interrupt_enable = bool(initial["ime"])
    # if "ie" in initial:
    #    mmu.ie_register = initial["ie"]

    for addr, val in initial["ram"]:
        inject_test_ram(mmu, addr, val)

    # --- EXECUTE INSTRUCTION ---
    cycles_taken = lib.cpu_step(ctypes.byref(cpu))

    # --- ASSERT FINAL STATE ---
    final = test_case["final"]
    name = test_case["name"]

    # 1. Assert Cycle Timing (SingleStepTests tracks every M-cycle in the array)
    # expected_m_cycles = len(test_case["cycles"])
    # assert cycles_taken == expected_m_cycles, (
    #    f"Cycle mismatch in {name}. Expected {expected_m_cycles}, got {cycles_taken}"
    # )

    # 2. Assert Registers
    assert cpu.a == final["a"], f"Reg A mismatch in {name}"
    assert cpu.f == final["f"], f"Reg F mismatch in {name}"
    assert cpu.b == final["b"], f"Reg B mismatch in {name}"
    assert cpu.c == final["c"], f"Reg C mismatch in {name}"
    assert cpu.d == final["d"], f"Reg D mismatch in {name}"
    assert cpu.e == final["e"], f"Reg E mismatch in {name}"
    assert cpu.h == final["h"], f"Reg H mismatch in {name}"
    assert cpu.l == final["l"], f"Reg L mismatch in {name}"
    assert cpu.pc == final["pc"], f"PC mismatch in {name}"
    assert cpu.sp == final["sp"], f"SP mismatch in {name}"

    # 3. Assert Interrupt State (if present)
    # if "ime" in final:
    #    assert cpu.master_interrupt_enable == bool(final["ime"]), (
    #        f"IME mismatch in {name}"
    #    )
    # if "ie" in final:
    #    assert mmu.ie_register == final["ie"], f"IE Register mismatch in {name}"

    # 4. Assert Memory mutations via the actual bus
    for addr, val in final["ram"]:
        actual_val = lib.bus_read(ctypes.byref(mmu), addr)
        assert actual_val == val, (
            f"RAM mismatch at 0x{addr:04X}. Expected 0x{val:02X}, got 0x{actual_val:02X} in {name}"
        )

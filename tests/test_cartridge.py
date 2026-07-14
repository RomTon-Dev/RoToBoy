import ctypes
import pytest
import os

LIB_PATH = os.path.abspath("./test_data/libMyProjectLib.so")
try:
    cart_lib = ctypes.CDLL(LIB_PATH)
except OSError:
    pytest.fail(f"Could not load shared library at {LIB_PATH}. Did you compile it?")


# WARNING: If you change cartridge.h, you MUST change this to match!
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


cart_lib.cartridge_load.argtypes = [ctypes.POINTER(Cartridge), ctypes.c_char_p]
cart_lib.cartridge_load.restype = ctypes.c_bool

cart_lib.cartridge_free.argtypes = [ctypes.POINTER(Cartridge)]
cart_lib.cartridge_free.restype = None

cart_lib.cartridge_read.argtypes = [ctypes.POINTER(Cartridge), ctypes.c_uint16]
cart_lib.cartridge_read.restype = ctypes.c_uint8

cart_lib.cartridge_write.argtypes = [
    ctypes.POINTER(Cartridge),
    ctypes.c_uint16,
    ctypes.c_uint8,
]
cart_lib.cartridge_write.restype = None


@pytest.fixture
def empty_cartridge():
    """Provides a fresh Cartridge struct for testing."""
    return Cartridge()


@pytest.fixture
def dummy_rom_file(tmp_path):
    """
    Creates a temporary dummy ROM file for testing loading.
    Standard Gameboy ROMs are usually at least 32KB.
    """
    rom_path = tmp_path / "test_game.gb"
    # Create 32KB of dummy data (filled with 0x00)
    dummy_data = bytearray([0x00] * 32768)

    # Let's put a specific byte at address 0x0100 (standard entry point)
    # so we can test reading it later.
    dummy_data[0x0100] = 0xAA

    rom_path.write_bytes(dummy_data)

    # ctypes needs byte strings for char*, so we encode the path
    return str(rom_path).encode("utf-8")


def test_cartridge_load_success(empty_cartridge, dummy_rom_file):
    """Tests that a valid ROM file loads correctly."""
    cart = empty_cartridge

    success = cart_lib.cartridge_load(ctypes.byref(cart), dummy_rom_file)

    assert success is True
    assert cart.rom_size == 32768

    # Clean up to avoid memory leaks during testing
    cart_lib.cartridge_free(ctypes.byref(cart))


def test_cartridge_load_failure(empty_cartridge):
    """Tests that loading a non-existent ROM fails gracefully."""
    cart = empty_cartridge
    bad_path = b"/path/to/nowhere/fake_game.gb"

    success = cart_lib.cartridge_load(ctypes.byref(cart), bad_path)

    assert success is False


def test_cartridge_read(empty_cartridge, dummy_rom_file):
    """Tests reading from the ROM via the MMU interface."""
    cart = empty_cartridge
    cart_lib.cartridge_load(ctypes.byref(cart), dummy_rom_file)

    # We injected 0xAA at address 0x0100 in our dummy fixture
    val = cart_lib.cartridge_read(ctypes.byref(cart), 0x0100)
    assert val == 0xAA

    cart_lib.cartridge_free(ctypes.byref(cart))


def test_cartridge_write(empty_cartridge, dummy_rom_file):
    """
    Tests writing to the cartridge.
    Note: Writing to ROM space usually intercepts MBC commands rather than
    actually writing data, but you can test state changes here.
    """
    cart = empty_cartridge
    cart_lib.cartridge_load(ctypes.byref(cart), dummy_rom_file)

    # Example: writing to 0x2000-0x3FFF changes the ROM bank in MBC1.
    # This assumes you have implemented that logic in cartridge_write!
    cart_lib.cartridge_write(ctypes.byref(cart), 0x2000, 0x02)

    cart_lib.cartridge_free(ctypes.byref(cart))

import os
import sys

"""
Simple File System Simulator 
Disk Layout:
  - 100 blocks total
  - Blocks  0–9  : Free Map  (10 blocks reserved)
  - Blocks 10–99 : File storage (90 blocks)

Individual Block Layout (544 bytes each):
  - Bytes   0–31  : File name  (32 bytes, null-padded)
  - Bytes  32–543 : File data  (512 bytes, null-padded)

Free Map:
  - Stored as the first 100 bytes of the Free Map region on disk
  - 0 = free block, 1 = allocated block
  - Blocks 0–9 are always marked allocated (reserved for Free Map)

DONE  : Disk layout, block I/O, free map (load/save/scan), format, REPL shell
Not Done: Rebuild file_table in startup(), then implement create, delete, ls
Not Done: Implement read, write, and full error handling
"""

# Constants
DISK_FILE        = "disk.bin"
TOTAL_BLOCKS     = 100
FREE_MAP_BLOCKS  = 10          # Blocks 0–9 Reserved
FILE_START_BLOCK = 10          # First Usable File Block
FILENAME_SIZE    = 32          # Bytes Per Filename Field
DATA_SIZE        = 512         # Bytes Per Data Field
BLOCK_SIZE       = FILENAME_SIZE + DATA_SIZE   # 544 Bytes Per Block
DISK_SIZE        = TOTAL_BLOCKS * BLOCK_SIZE   # 54,400 Bytes Total


# Low-level Disk I/O
def read_block(block_num: int) -> bytes:
    """Read and return the raw bytes of a single block from disk."""
    with open(DISK_FILE, "rb") as f:
        f.seek(block_num * BLOCK_SIZE)
        return f.read(BLOCK_SIZE)


def write_block(block_num: int, data: bytes) -> None:
    """Write exactly BLOCK_SIZE bytes to a block. Zero-pads if data is short."""
    padded = data[:BLOCK_SIZE].ljust(BLOCK_SIZE, b"\x00")
    with open(DISK_FILE, "r+b") as f:
        f.seek(block_num * BLOCK_SIZE)
        f.write(padded)


# Free Map
def load_free_map() -> list:
    """
    Read the free map from disk.
    Returns a list of 100 ints — one per block (0 = free, 1 = allocated).
    The map is stored as the first TOTAL_BLOCKS bytes of the disk.
    """
    with open(DISK_FILE, "rb") as f:
        f.seek(0)
        raw = f.read(TOTAL_BLOCKS)
    return list(raw)


def save_free_map(free_map: list) -> None:
    """Persist the free map to the reserved region at the start of disk."""
    data = bytes(free_map).ljust(FREE_MAP_BLOCKS * BLOCK_SIZE, b"\x00")
    with open(DISK_FILE, "r+b") as f:
        f.seek(0)
        f.write(data[:FREE_MAP_BLOCKS * BLOCK_SIZE])


def first_free_block(free_map: list) -> int | None:
    """
    Scan the free map and return the lowest-numbered available file block.
    Returns None if the disk is full.
    """
    for i in range(FILE_START_BLOCK, TOTAL_BLOCKS):
        if free_map[i] == 0:
            return i
    return None


# Format Command
def cmd_format(free_map: list, file_table: dict) -> tuple:
    """
    Wipe the entire disk and re-initialize the free map.
    Blocks 0–9 are marked allocated (Free Map region); all others are free.
    """
    print("Formatting disk...")

    with open(DISK_FILE, "wb") as f:
        f.write(b"\x00" * DISK_SIZE)

    new_map = [1] * FREE_MAP_BLOCKS + [0] * (TOTAL_BLOCKS - FREE_MAP_BLOCKS)
    save_free_map(new_map)

    print("Disk formatted successfully.")
    print(f"FreeMap blocks 0-{FREE_MAP_BLOCKS - 1} are now allocated.")

    free_map[:] = new_map
    file_table.clear()

    return free_map, file_table


# Startup
def startup() -> tuple:
    """
    On first run: create a blank disk image.
    On subsequent runs: load the existing disk and rebuild state.
    Returns (free_map, file_table).
    """
    if not os.path.isfile(DISK_FILE) or os.path.getsize(DISK_FILE) != DISK_SIZE:
        with open(DISK_FILE, "wb") as f:
            f.write(b"\x00" * DISK_SIZE)
        free_map  = [0] * TOTAL_BLOCKS
        file_table = {}
        print("No disk image found. A blank disk has been created.")
        print("Tip: run 'format' to initialize the file system before use.")
    else:
        free_map = load_free_map()
        print("Loading file table from disk...")
        file_table = {}  # TODO: Rebuild file table from allocated blocks
        print("File table loaded successfully.")

    return free_map, file_table


# Main REPL
HELP_TEXT = """\
Available commands:
  --> format
  --> create <filename>    
  --> read <filename>       
  --> write <filename>      
  --> delete <filename>      
  --> ls                     
  --> exit"""


def main():
    free_map, file_table = startup()
    print("\nWelcome to the simple file system simulator.")
    print(HELP_TEXT)

    while True:
        try:
            raw = input("\n> ").strip()
        except (EOFError, KeyboardInterrupt):
            print("\nExiting file system simulator.")
            sys.exit(0)

        if not raw:
            continue

        parts   = raw.split(maxsplit=1)
        command = parts[0].lower()
        argument = parts[1].strip() if len(parts) > 1 else ""

        if command == "format":
            free_map, file_table = cmd_format(free_map, file_table)

        elif command in ("create", "read", "write", "delete", "ls"):
            # TODO: Implement these commands
            print(f"'{command}' is not yet implemented.")

        elif command == "exit":
            print("Exiting file system simulator.")
            sys.exit(0)

        else:
            print(f"Unknown command: '{command}'.")


if __name__ == "__main__":
    main()
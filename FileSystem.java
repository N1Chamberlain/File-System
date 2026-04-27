import java.io.*;
import java.nio.file.*;
import java.util.*;

/*
 * Simple File System Simulator
 * Disk Layout:
 *   - 100 blocks total
 *   - Blocks  0–9  : Free Map  (10 blocks reserved)
 *   - Blocks 10–99 : File storage (90 blocks)
 *
 * Individual Block Layout (544 bytes each):
 *   - Bytes   0–31  : File name  (32 bytes, null-padded)
 *   - Bytes  32–543 : File data  (512 bytes, null-padded)
 *
 * Free Map:
 *   - Stored as the first 100 bytes of the Free Map region on disk
 *   - 0 = free block, 1 = allocated block
 *   - Blocks 0–9 are always marked allocated (reserved for Free Map)
 *
 * DONE  : Disk layout, block I/O, free map (load/save/scan), format, REPL shell
 * Not Done: Rebuild file_table in startup(), then implement create, delete, ls
 * Not Done: Implement read, write, and full error handling
 */
public class FileSystem {

    static final String DISK_FILE        = "disk.bin";
    static final int    TOTAL_BLOCKS     = 100;
    static final int    FREE_MAP_BLOCKS  = 10;
    static final int    FILE_START_BLOCK = 10;
    static final int    FILENAME_SIZE    = 32;
    static final int    DATA_SIZE        = 512;
    static final int    BLOCK_SIZE       = FILENAME_SIZE + DATA_SIZE;
    static final int    DISK_SIZE        = TOTAL_BLOCKS * BLOCK_SIZE;

    // Low-level Disk I/O
    static byte[] readBlock(int blockNum) throws IOException {
        try (RandomAccessFile raf = new RandomAccessFile(DISK_FILE, "r")) {
            raf.seek((long) blockNum * BLOCK_SIZE);
            byte[] buffer = new byte[BLOCK_SIZE];
            raf.readFully(buffer);
            return buffer;
        }
    }

    static void writeBlock(int blockNum, byte[] data) throws IOException {
        byte[] padded = new byte[BLOCK_SIZE];
        System.arraycopy(data, 0, padded, 0, Math.min(data.length, BLOCK_SIZE));
        try (RandomAccessFile raf = new RandomAccessFile(DISK_FILE, "rw")) {
            raf.seek((long) blockNum * BLOCK_SIZE);
            raf.write(padded);
        }
    }

    // Free Map
    static int[] loadFreeMap() throws IOException {
        try (RandomAccessFile raf = new RandomAccessFile(DISK_FILE, "r")) {
            raf.seek(0);
            byte[] raw = new byte[TOTAL_BLOCKS];
            raf.readFully(raw);
            int[] freeMap = new int[TOTAL_BLOCKS];
            for (int i = 0; i < TOTAL_BLOCKS; i++) freeMap[i] = raw[i] & 0xFF;
            return freeMap;
        }
    }

    static void saveFreeMap(int[] freeMap) throws IOException {
        byte[] data = new byte[FREE_MAP_BLOCKS * BLOCK_SIZE];
        for (int i = 0; i < TOTAL_BLOCKS; i++) data[i] = (byte) freeMap[i];
        try (RandomAccessFile raf = new RandomAccessFile(DISK_FILE, "rw")) {
            raf.seek(0);
            raf.write(data);
        }
    }

    static int firstFreeBlock(int[] freeMap) {
        for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS; i++)
            if (freeMap[i] == 0) return i;
        return -1;
    }

    // Format Command
    static void cmdFormat(int[] freeMap, Map<String, Integer> fileTable) throws IOException {
        System.out.println("Formatting disk...");
        try (FileOutputStream fos = new FileOutputStream(DISK_FILE)) {
            fos.write(new byte[DISK_SIZE]);
        }
        int[] newMap = new int[TOTAL_BLOCKS];
        for (int i = 0; i < FREE_MAP_BLOCKS; i++) newMap[i] = 1;
        saveFreeMap(newMap);
        System.out.println("Disk formatted successfully.");
        System.out.printf("FreeMap blocks 0-%d are now allocated.%n", FREE_MAP_BLOCKS - 1);
        System.arraycopy(newMap, 0, freeMap, 0, TOTAL_BLOCKS);
        fileTable.clear();
    }

    // Startup
    static class StartupResult {
        int[] freeMap;
        Map<String, Integer> fileTable;
        StartupResult(int[] freeMap, Map<String, Integer> fileTable) {
            this.freeMap = freeMap;
            this.fileTable = fileTable;
        }
    }

    static StartupResult startup() throws IOException {
        File diskFile = new File(DISK_FILE);
        if (!diskFile.exists() || diskFile.length() != DISK_SIZE) {
            try (FileOutputStream fos = new FileOutputStream(DISK_FILE)) {
                fos.write(new byte[DISK_SIZE]);
            }
            System.out.println("No disk image found. A blank disk has been created.");
            System.out.println("Tip: run 'format' to initialize the file system before use.");
            return new StartupResult(new int[TOTAL_BLOCKS], new HashMap<>());
        } else {
            int[] freeMap = loadFreeMap();
            System.out.println("Loading file table from disk...");
            Map<String, Integer> fileTable = new HashMap<>(); // TODO: Rebuild file table
            System.out.println("File table loaded successfully.");
            return new StartupResult(freeMap, fileTable);
        }
    }

    // Main REPL
    static final String HELP_TEXT =
            "Available commands:\n" +
                    "  format\n" +
                    "  create <filename>\n" +
                    "  read <filename>\n" +
                    "  write <filename>\n" +
                    "  delete <filename>\n" +
                    "  ls\n" +
                    "  exit";

    public static void main(String[] args) throws IOException {
        StartupResult state = startup();
        int[] freeMap = state.freeMap;
        Map<String, Integer> fileTable = state.fileTable;

        System.out.println("\nWelcome to the simple file system simulator.");
        System.out.println(HELP_TEXT);

        Scanner scanner = new Scanner(System.in);
        while (true) {
            System.out.print("\n> ");
            if (!scanner.hasNextLine()) {
                System.out.println("\nExiting file system simulator.");
                break;
            }
            String raw = scanner.nextLine().strip();
            if (raw.isEmpty()) continue;

            String[] parts = raw.split("\\s+", 2);
            String command = parts[0].toLowerCase();
            String argument = parts.length > 1 ? parts[1].strip() : "";

            try {
                switch (command) {
                    case "format":
                        cmdFormat(freeMap, fileTable);
                        break;
                    case "create": case "read": case "write": case "delete": case "ls":
                        System.out.printf("'%s' is not yet implemented.%n", command);
                        break;
                    case "exit":
                        System.out.println("Exiting file system simulator.");
                        scanner.close();
                        return;
                    default:
                        System.out.printf("Unknown command: '%s'.%n", command);
                }
            } catch (IOException e) {
                System.out.println("Disk I/O error: " + e.getMessage());
            }
        }
        scanner.close();
    }
}
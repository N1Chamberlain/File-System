/*
 * Simple File System Simulator 
 * Disk Layout:
 *   - 100 blocks total
 *   - Blocks  0-9  : Free Map (10 blocks reserved)
 *   - Blocks 10-99 : File storage (90 blocks)
 *
 * Individual Block Layout (544 bytes each):
 *   - Bytes   0-31  : File name  (32 bytes, null-padded)
 *   - Bytes  32-543 : File data  (512 bytes, null-padded)
 *
 * Free Map:
 *   - Stored as the first 100 bytes of the disk
 *   - 0 = free block, 1 = allocated block
 *   - Blocks 0-9 are always marked allocated (reserved for Free Map)
 *
 * Originally DONE: Disk layout, block I/O, free map (load/save/scan), format, REPL shell
 * Currently Done : Rebuilt file table in startup(), implemented create, delete, ls
 *
 * Need to get done : Implement read, write, and full error handling
 *
 * Compile: gcc -o filesystem filesystem_part1.c
 * Run:     ./filesystem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constants */
#define DISK_FILE        "disk.bin"
#define TOTAL_BLOCKS     100
#define FREE_MAP_BLOCKS  10
#define FILE_START_BLOCK 10
#define FILENAME_SIZE    32
#define DATA_SIZE        512
#define BLOCK_SIZE       (FILENAME_SIZE + DATA_SIZE)   /* 544 bytes */
#define DISK_SIZE        (TOTAL_BLOCKS * BLOCK_SIZE)   /* 54,400 bytes */

/* In-memory state */
static int freeMap[TOTAL_BLOCKS];
static char fileTable[TOTAL_BLOCKS][FILENAME_SIZE];


/* Low-level Disk I/O */
void read_block(int blockNum, char *buf) {
    FILE *f = fopen(DISK_FILE, "rb");
    if (!f) { perror("read_block: fopen"); exit(1); }
    fseek(f, blockNum * BLOCK_SIZE, SEEK_SET);
    fread(buf, 1, BLOCK_SIZE, f);
    fclose(f);
}

void write_block(int blockNum, const char *buf) {
    FILE *f = fopen(DISK_FILE, "r+b");
    if (!f) { perror("write_block: fopen"); exit(1); }
    char padded[BLOCK_SIZE];
    memset(padded, 0, BLOCK_SIZE);
    memcpy(padded, buf, BLOCK_SIZE);
    fseek(f, blockNum * BLOCK_SIZE, SEEK_SET);
    fwrite(padded, 1, BLOCK_SIZE, f);
    fclose(f);
}

/* Free Map */
void load_free_map(void) {
    FILE *f = fopen(DISK_FILE, "rb");
    if (!f) { perror("load_free_map: fopen"); exit(1); }
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        freeMap[i] = fgetc(f);
    }
    fclose(f);
}

void save_free_map(void) {
    FILE *f = fopen(DISK_FILE, "r+b");
    if (!f) { perror("save_free_map: fopen"); exit(1); }
    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        fputc(freeMap[i], f);
    }
    fclose(f);
}

int first_free_block(void) {
    for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS; i++) {
        if (freeMap[i] == 0) return i;
    }
    return -1;  /* disk full */
}

/* Format Command */
void cmd_format(void) {
    printf("Formatting disk...\n");

    FILE *f = fopen(DISK_FILE, "wb");
    if (!f) { perror("cmd_format: fopen"); exit(1); }
    char zero[DISK_SIZE];
    memset(zero, 0, DISK_SIZE);
    fwrite(zero, 1, DISK_SIZE, f);
    fclose(f);

    for (int i = 0; i < FREE_MAP_BLOCKS; i++)       freeMap[i] = 1;
    for (int i = FREE_MAP_BLOCKS; i < TOTAL_BLOCKS; i++) freeMap[i] = 0;
    save_free_map();

    memset(fileTable, 0, sizeof(fileTable));

    printf("Disk formatted successfully.\n");
    printf("FreeMap blocks 0-%d are now allocated.\n", FREE_MAP_BLOCKS - 1);
}


/* LS */
void cmd_ls(void){
    printf("Files on disk:\n");
    int count = 0;
    for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS; i++){
        if(freeMap[i]==1){
            printf(" %s\n", fileTable[i]);
            count++;
        }
    }
    if(count==0){
        printf(" (no files found sorry)\n");
    }
}

/* Create */
void cmd_create(char *filename) {
    // Check if filename is empty
    if (strlen(filename) == 0) {
        printf("Error: Please provide a filename.\n");
        return;
    }
    // Check if filename is too long
    if (strlen(filename) >= FILENAME_SIZE) {
        printf("Error: Filename too long (max %d characters).\n", FILENAME_SIZE - 1);
        return;
    }
    // Check if file already exists
    for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS; i++) {
        if (freeMap[i] == 1 && strcmp(fileTable[i], filename) == 0) {
            printf("Error: File '%s' already exists.\n", filename);
            return;
        }
    }

    // Find first free block
    int block = first_free_block();
    if (block == -1) {
        printf("Error: Disk is full. No free blocks available.\n");
        return;
    }

    // Build a blank block with just the filename in the name field
    char blockData[BLOCK_SIZE];
    memset(blockData, 0, BLOCK_SIZE);
    strncpy(blockData, filename, FILENAME_SIZE - 1);

    write_block(block, blockData);
    freeMap[block] = 1;
    save_free_map();

    // Update in-memory file table
    strncpy(fileTable[block], filename, FILENAME_SIZE - 1);

    printf("File '%s' created successfully at block %d.\n", filename, block);
}

/*delete*/
void cmd_delete(char *filename){
    for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS;i++){
        if(freeMap[i] == 1 && strcmp(fileTable[i], filename) == 0){
            /*clear the block*/
            char empty[BLOCK_SIZE];
            memset(empty, 0, BLOCK_SIZE);
            write_block(i, empty);

            /*clear file table */
            memset(fileTable[i], 0, FILENAME_SIZE);

            /*mark block as free*/
            freeMap[i] = 0;
            save_free_map();
            printf("file '%s' deleted successfully.\n", filename);
            return;
        }
    }
    printf("error: file '%s' not found\n", filename);
}





/* Startup */

void startup(void) {
    FILE *f = fopen(DISK_FILE, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        if (size == DISK_SIZE) {
            load_free_map();
            printf("Loading file table from disk...\n");
            for (int i = FILE_START_BLOCK; i < TOTAL_BLOCKS;i++){
                if (freeMap[i] == 1){
                    char buf[BLOCK_SIZE];
                    read_block(i, buf);
                    memcpy(fileTable[i], buf, FILENAME_SIZE);
                }
            }
                printf("File table loaded successfully.\n");
            return;
        }
    }

    /* First run or corrupt disk — create blank image */
    f = fopen(DISK_FILE, "wb");
    if (!f) { perror("startup: fopen"); exit(1); }
    char zero[DISK_SIZE];
    memset(zero, 0, DISK_SIZE);
    fwrite(zero, 1, DISK_SIZE, f);
    fclose(f);

    memset(freeMap, 0, sizeof(freeMap));
    printf("No disk image found. A blank disk has been created.\n");
    printf("Tip: run 'format' to initialize the file system before use.\n");
}

/* Main REPL */

void print_help(void) {
    printf("Available commands:\n");
    printf("  --> format\n");
    printf("  --> create <filename>\n");
    printf("  --> read <filename>      [NOT YET IMPLEMENTED]\n");
    printf("  --> write <filename>     [NOT YET IMPLEMENTED]\n");
    printf("  --> delete <filename>\n");
    printf("  --> ls\n");
    printf("  --> exit\n");
}

int main(void) {
    startup();
    printf("\nWelcome to the simple file system simulator.\n");
    print_help();

    char line[600];
    while (1) {
        printf("\n> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nExiting file system simulator.\n");
            break;
        }

        /* Strip trailing newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strlen(line) == 0) continue;

        char command[64] = {0};
        char argument[512] = {0};
        sscanf(line, "%63s %511[^\n]", command, argument);

        if (strcmp(command, "format") == 0) {
            cmd_format();
        }
        else if (strcmp(command, "ls") == 0) {
            cmd_ls();
        }
        else if (strcmp(command, "create") == 0) {
            if (strlen(argument) == 0) {
                printf("Error: Please provide a filename.\n");
            } else {
                cmd_create(argument);
            }
        }
        else if (strcmp(command, "delete") == 0) {
            if (strlen(argument) == 0 ){
                printf("please enter name of the file you want to delete");
            } else{
                cmd_delete(argument);
            }
        }
        else if (strcmp(command, "read")   == 0 ||
            strcmp(command, "write")  == 0) {
            /* TODO (Part 2 & 3): implement these commands */
            printf("'%s' is not yet implemented.\n", command);
        } else if (strcmp(command, "exit") == 0) {
            printf("Exiting file system simulator.\n");
            break;
        } else {
            printf("Unknown command: '%s'.\n", command);
        }
    }

    return 0;
}

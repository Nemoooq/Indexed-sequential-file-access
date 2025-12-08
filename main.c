#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define PAGES_INDEXES_FILENAME "IndexFile.txt"
#define PRIMARY_AREA_FILENAME "PrimaryFile.txt"
#define OVERFLOW_AREA_FILENAME "OverflowArea.txt"
#define TEST_DATA_FILENAME "testData.txt"
#define MAX_RECORD_LENGTH 30 //30 characters + \n
#define DIGITS_IN_MAX_INT 10
#define BLOCKING_FACTOR_PAGE 4
#define BLOCKING_FACTOR_INDEX 1000
#define ALFA_FACTOR 0.5
#define MAX_RECORDS _CRT_INT_MAX
#define REORGANIZATION_OVERFLOW_SIZE 4
#define MAX_COMMAND_LENGTH 47
#define MAX_MNEMONIC_LENGTH 7
#define CHARACTER_SET "abcdefghijklmnopqrstuvwxyz"
#define MAX_INT_LEN 10
#define INDEX_FILE_POSITION_LENGHT MAX_INT_LEN+MAX_INT_LEN+1+1 //maxIntLen + maxIntLen + ";" + "\n"
#define OVERFLOW_AREA_RATIO 0.2 
#define PRIMARY_RECORD_LENGTH 42
#define PRIMARY_PAGE_LENGTH (PRIMARY_RECORD_LENGTH * BLOCKING_FACTOR_PAGE)

int numberOfPages =  0; //it is rather index than number (number = index+1)

typedef struct Record {
    char data[MAX_RECORD_LENGTH];
} Record;

typedef struct Cell {
    unsigned int key;
    Record record;
} Cell;

typedef struct Page {
    Cell cell[BLOCKING_FACTOR_PAGE];
    unsigned int overflowPageId;
} Page;

typedef struct IndexEntry {
    unsigned int key;
    unsigned int pageNumber;
} IndexEntry;

int generateKey() {
    //because RAND_MAX = 0x7FFF
    unsigned int firstPart = (unsigned int)rand();
    unsigned int secondPart = (unsigned int)rand() << 15;
    unsigned int lastTwo = ((unsigned int)rand() & 0x03) << 30; 
    return firstPart | secondPart | lastTwo;
}

char* generateValue(char* record) {
    for (int i = 0; i < MAX_RECORD_LENGTH; i++) {
        int idx = rand() % (sizeof(CHARACTER_SET) - 1);
        record[i] = CHARACTER_SET[idx];
    }
    for (int i = MAX_RECORD_LENGTH; i < MAX_RECORD_LENGTH; i++) {
        record[i] = ' ';
    }
    record[MAX_RECORD_LENGTH] = '\0';
    return record;
}

int fileProcess() {
    return 1;
}

void printHelp() {
    printf("Possible commands:\n");
    printf("- ADD [RECORD_VALUE] - adds specified record to the file\n");
    printf("- ADDG - adds auto generated record to the file\n");
    printf("- READR [RECORD_INDEX] - reads record from specified index\n");
    printf("- DISP - display whole file\n");
    printf("- DISPI [PAGE_INDEX]- display specific page index\n");
    printf("- DEL [INDEX] - deletes specified index\n");
    printf("- MOD [INDEX] [NEW_VALUE] - changes value of record on specified index\n");
    return;
}

int chackIsFileEmpty() {
    long long fileSize = 0;
    int isFileEmpty = 1;
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "r");
    fseek(indexFile, 0, SEEK_END);
    fileSize = ftell(indexFile);
    if(fileSize) {
        printf("File exists and is not empty\n");
        isFileEmpty = 0;
        
    } else {
        printf("File is empty\n");
        isFileEmpty = 1;
    }
    fclose(indexFile);
    return isFileEmpty;
}

//Index file view PAGE_ID;KEY
    //so number of characters each is PAGE_ID LEN + KEY_LEN + 1 (";")

int addPageToIndexFile(Page page) {
    char buffor[MAX_RECORD_LENGTH + 2];
    IndexEntry newIndexEntry;
    newIndexEntry.key = page.cell[0].key; 
    newIndexEntry.pageNumber = numberOfPages;
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "a+");
    long offset = (long)(newIndexEntry.pageNumber) * INDEX_FILE_POSITION_LENGHT;
    fseek(indexFile, offset, SEEK_SET);
    fprintf(indexFile, "%010u;%010u\n", newIndexEntry.pageNumber, newIndexEntry.key);
    fclose(indexFile);
    return 0; 
}

int addPageToPrimaryFile(Page page) {
    unsigned int pageIndex = numberOfPages; 
    FILE* primaryFile = fopen(PRIMARY_AREA_FILENAME, "r+b");
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    long offset = (long)pageIndex * pageSize;
    fseek(primaryFile, offset, SEEK_SET);
    for (int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        unsigned int currentKey;
        const char* currentData;
        if (page.cell[i].key != 0) {
            currentKey = page.cell[i].key;
            currentData = page.cell[i].record.data;
        } else {
            currentKey = 0; 
            currentData = ""; 
        }

        fprintf(primaryFile, "%010u;%-30.30s\n", currentKey, currentData); 
    }
    
    fclose(primaryFile);
    return 0;
}

int createNewPage(Cell cell) {
    Page newPage;
    memset(&newPage, 0, sizeof(Page));
    newPage.cell[0] = cell;
    if (addPageToIndexFile(newPage) != 0) return 1;
    if (addPageToPrimaryFile(newPage) != 0) return 1;
    numberOfPages++; 
    return 0;
}

int insertCellToFile(Cell cell) {
    return 1;
}

int insertCell(Cell cell) {
    int isFileEmpty = chackIsFileEmpty();
    if (isFileEmpty) {
        createNewPage(cell);
    } else {
        insertCellToFile(cell);
    }
    return 1;
}

int addRecord(Record record) {
    Cell newCell;
    newCell.key = generateKey();
    newCell.record = record;
    if (!insertCell(newCell)) {
        printf("Cell added succesfully\n");
        return 0;
    } else {
        printf("Error - cell not added\n");
        return 1;
    }
    return 1;
}

void CommandADDGProcess() {
    //int initialCreation = isFileEmpty();
    char dataBuffer[MAX_RECORD_LENGTH] = {0};
    generateValue(dataBuffer);
    Record newRecord;
    strncpy(newRecord.data, dataBuffer, MAX_RECORD_LENGTH);
    if (!addRecord(newRecord)) {
        printf("Record added succesfully\n");
    } else {
        printf("Error - record not added\n");
    }
}

int processCommand(char* inputBuffor) {
    char debugRecord[MAX_RECORD_LENGTH] = {0};
    char mnemonic[MAX_MNEMONIC_LENGTH] = {0};
    char firstArgument[MAX_RECORD_LENGTH] = {0};
    char secondArgument[MAX_RECORD_LENGTH] = {0};
    sscanf(inputBuffor,"%s %s %s",mnemonic,firstArgument,secondArgument);
    if(strcmp(mnemonic, "DISP") == 0) {
        printf("chuja\n");
    } else if (strcmp(mnemonic, "ADDG") == 0) {
        CommandADDGProcess();
    } else if (strcmp(mnemonic, "ADD") == 0) {
        printf("%s\n", firstArgument);
    } else if (strcmp(mnemonic, "READR") == 0) {
        printf("%s\n", firstArgument);
    } else if (strcmp(mnemonic, "DISPI") == 0) {
        printf("%s\n", firstArgument);
    } else if (strcmp(mnemonic, "DEL") == 0) {
        printf("%s\n", firstArgument);
    } else if (strcmp(mnemonic, "MOD") == 0) {
        printf("%s, %s\n", firstArgument, secondArgument);
    } else {
        printf("Unknown mnemonic\n");
        return 1;
    }
    
    return 0;
}

void clearInputBufor(char* inputBufor) {
    for(int i = 0; i < MAX_COMMAND_LENGTH; i++) {
        inputBufor[i] = 0;
    }
    return;
}

int commandLineLoop() {
    int exit = 0;
    int showAfterOperation = 0;
    char inputBufor[MAX_COMMAND_LENGTH] = {0}; 
    printf("Write help for list of commands\n");
    printf("Write exit to exit the program\n");
    while(!exit) {
        printf("SBD_DISK\\: ");
        fgets(inputBufor, MAX_COMMAND_LENGTH, stdin);
        size_t len = strlen(inputBufor);
        if (len > 0 && inputBufor[len-1] == '\n') {
             inputBufor[len-1] = '\0';
        }
        if(!strcmp(inputBufor, "help")) {
            printHelp();
        } else if (!strcmp(inputBufor, "exit")) {
            exit = 1;
        } else {
            int command = processCommand(inputBufor);
        }
        clearInputBufor(inputBufor);
    }
    printf("Program ended by the user\n");
}

int main() {
    int decision = 0;
    while (decision!=1 && decision!=2) {
        printf("How do you want operate on file:\n");
        printf("1 - by commands in file %s\n", TEST_DATA_FILENAME);
        printf("2 - by commands in command line\n");
        scanf("%d", &decision);
    }
    printf("You picked %d\n", decision);

    if(decision == 1) {
        printf("This option in not Implemented\n");
        return fileProcess();
    } else if (decision == 2) {
        return commandLineLoop();
    } else {
        printf("Error - wrong decision ID\n");
        return 1;
    }

    return 0;
}
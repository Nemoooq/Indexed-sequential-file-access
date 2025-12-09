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

unsigned int numberOfPages =  0; //it is rather index than number (number = index+1)
unsigned int currentOverflowIndex = 0;

typedef struct Record {
    char data[MAX_RECORD_LENGTH];
} Record;

typedef struct Cell {
    unsigned int key;
    Record record;
    unsigned int overflowPointer;
} Cell;

typedef struct Page {
    Cell cell[BLOCKING_FACTOR_PAGE];
} Page;

typedef struct IndexEntry {
    unsigned int key;
    unsigned int pageNumber;
} IndexEntry;

typedef struct IndexPage {
    IndexEntry indexEntry[BLOCKING_FACTOR_PAGE];
    unsigned int nextIndexPageId;
} IndexPage;

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

int addPageToIndexFile(IndexPage page) {
    unsigned int pageIndex = numberOfPages; 
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "r+b");
    long pageSize = (long)BLOCKING_FACTOR_PAGE * INDEX_FILE_POSITION_LENGHT;
    long offset = (long)pageIndex * pageSize;
    fseek(indexFile, offset, SEEK_SET);
    for (int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        unsigned int currentKey = page.indexEntry[i].key;
        unsigned int currentPageNo = page.indexEntry[i].pageNumber;
        fprintf(indexFile, "%010u;%010u\n", currentPageNo, currentKey); 
    }
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
        unsigned int currentOverflowPointer;
        if (page.cell[i].key != 0) {
            currentKey = page.cell[i].key;
            currentData = page.cell[i].record.data;
            currentOverflowPointer = page.cell[i].overflowPointer;
        } else {
            currentKey = 0; 
            currentData = ""; 
        }

        fprintf(primaryFile, "%010u;%-30.30s;%010u\n", currentKey, currentData, currentOverflowPointer);
    }
    numberOfPages++;
    
    fclose(primaryFile);
    return 0;
}

int createNewPage(Cell cell) {
    Page newPage;
    IndexPage newIndexPage;
    memset(&newIndexPage, 0, sizeof(IndexPage));
    memset(&newPage, 0, sizeof(Page));
    newPage.cell[0] = cell;
    newIndexPage.indexEntry[0].key = cell.key;
    newIndexPage.indexEntry[0].pageNumber = numberOfPages;
    if (addPageToIndexFile(newIndexPage) != 0) {
        return 1;
    }
    if (addPageToPrimaryFile(newPage) != 0) { 
        return 1;
    }
    numberOfPages++; 
    return 0;
}

int getIndexPage(IndexPage* page, unsigned int index) {
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "r");
    long pageSize = (long)BLOCKING_FACTOR_PAGE * INDEX_FILE_POSITION_LENGHT;
    long offset = (long)index * pageSize;
    fseek(indexFile, offset, SEEK_SET);
    memset(page, 0, sizeof(IndexPage));
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        unsigned int pageNo = 0;
        unsigned int key = 0;
        int result = fscanf(indexFile, "%%%10u;%%%10u\n", &pageNo, &key); 
        if (result == 2) {
            page->indexEntry[i].pageNumber = pageNo;
            page->indexEntry[i].key = key;
        } else if (result == EOF) {
            printf("EOF\n");
            break; 
        }
    }

    fclose(indexFile);
}

unsigned int getIndexOfPageToInsert(unsigned int key) {
    unsigned int currentPageIndex = 0;
    unsigned int indexOfInsertion = 0;
    IndexPage readPage;
    IndexPage tmpReadPage;
    getIndexPage(&readPage, currentPageIndex);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        if(key > readPage.indexEntry[i].key) {
            if(key < readPage.indexEntry[i+1].key && i+1<=(BLOCKING_FACTOR_PAGE-1)) {
                return readPage.indexEntry[i].pageNumber;
            } else if (i+1 > (BLOCKING_FACTOR_PAGE-1)) {
                getIndexPage(&tmpReadPage, currentPageIndex+1);
                if (key < tmpReadPage.indexEntry[0].key) {
                    return readPage.indexEntry[i].pageNumber;
                }
            }
        }
    }
    return indexOfInsertion;
}

int getPrimaryPage(Page* page, unsigned int index) {
    FILE* primaryFile = fopen(PRIMARY_AREA_FILENAME, "rb"); 
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    long offset = (long)index * pageSize;
    fseek(primaryFile, offset, SEEK_SET);
    memset(page, 0, sizeof(Page)); 
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        char valueBuffer[MAX_RECORD_LENGTH + 1] = {0}; 
        unsigned int key = 0;
        int result = fscanf(primaryFile, "%10u;%30s\n", &key, valueBuffer); 
        if (result == 2) {
            page->cell[i].key = key;
            strncpy(page->cell[i].record.data, valueBuffer, MAX_RECORD_LENGTH);
            page->cell[i].record.data[MAX_RECORD_LENGTH - 1] = '\0';
            
        } else if (result == EOF) {
            printf("EOF\n");
            break; 
        }
    }
    fclose(primaryFile);
    return 0;
}

int writePageToPrimary(Page page, unsigned int index) {
    FILE* primaryFile = fopen(PRIMARY_AREA_FILENAME, "r+b");
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    long offset = (long)index * pageSize;
    fseek(primaryFile, offset, SEEK_SET);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        unsigned int currentKey = page.cell[i].key;
        const char* currentData = page.cell[i].record.data;
        fprintf(primaryFile, "%010u;%-30.30s\n", currentKey, currentData); 
    }
    fclose(primaryFile);
    
    return 0; 
}

int writeCellToOverflow(Cell cell) {
    //to be implemented
    return 0;
}

int insertCellToFile(Cell cell) {
    //getIndexOfPage() - done
    //getPage(index) - done
    //tryToInsert() - done
    //if insert is successfull place Page back - done
    //if you cannot insert go to overflow
    Page readPage;
    int isFitted = 0;
    int isInserted = 0;
    unsigned int overflowPageIndex = 0;
    unsigned int indexOfPage = getIndexOfPageToInsert(cell.key);
    getPrimaryPage(&readPage, indexOfPage);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        if(readPage.cell[i].key == 0) {
            readPage.cell[i].key = cell.key;
            strncpy(readPage.cell[i].record.data, cell.record.data, MAX_RECORD_LENGTH);
            isInserted = 1;
            break;
        } else if (readPage.cell[i].key < cell.key) {
            isFitted = 1;
            for(int j = i+1; j < BLOCKING_FACTOR_PAGE; j++){
                if (readPage.cell[j].key < cell.key) {
                    isFitted = 0;
                    break;
                }
            }
            if (isFitted) {
                readPage.cell[i].overflowPointer = currentOverflowIndex;
            }
        }
    }

    if (isInserted) {
        writePageToPrimary(readPage, indexOfPage);
    } else {
        writeCellToOverflow(cell);
    }
    
    return 1;
}

int insertCell(Cell cell) {
    int isFileEmpty = chackIsFileEmpty();
    if (isFileEmpty) {
        createNewPage(cell);
    } else {
        insertCellToFile(cell);
    }
    return 0;
}

int addRecord(Record record) {
    Cell newCell;
    newCell.key = generateKey();
    newCell.record = record;
    newCell.overflowPointer = 0;
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
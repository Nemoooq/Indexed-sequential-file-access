#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define PAGES_INDEXES_FILENAME "IndexFile.txt"
#define PRIMARY_AREA_FILENAME "PrimaryFile.txt"
#define OVERFLOW_AREA_FILENAME "OverflowArea.txt"
#define OVERFLOW_AREA_NEW_FILE "OverflowAreaNewFile.txt"
#define PRIMARY_AREA_NEW_FILE "PrimaryFileNewFile.txt"
#define INDEX_AREA_NEW_FILE "IndexFileNewFile.txt"
#define TEST_DATA_FILENAME "testData.txt"
#define MAX_RECORD_LENGTH 30 //30 characters + \n
#define DIGITS_IN_MAX_INT 10
#define BLOCKING_FACTOR_PAGE 4
#define ALFA_FACTOR 0.5
#define MAX_RECORDS _CRT_INT_MAX
#define REORGANIZATION_OVERFLOW_SIZE 3
#define MAX_COMMAND_LENGTH 47
#define MAX_MNEMONIC_LENGTH 7
#define CHARACTER_SET "abcdefghijklmnopqrstuvwxyz"
#define MAX_INT_LEN 10
#define INDEX_FILE_POSITION_LENGHT MAX_INT_LEN+MAX_INT_LEN+1+1 //maxIntLen + maxIntLen + ";" + "\n"
#define OVERFLOW_AREA_RATIO 0.2 
#define PRIMARY_RECORD_LENGTH 53 //10+10+30+3
#define MAX_OVERFLOW_SIZE_WITHOUT_REORGANIZATION 3
#define PRIMARY_PAGE_LENGTH (PRIMARY_RECORD_LENGTH * BLOCKING_FACTOR_PAGE)
#define max(a,b) (((a) > (b)) ? (a) : (b))

unsigned int numberOfPages =  0; //it is rather index than number (number = index+1)
unsigned int mainOverflowCounter = 1;

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

int compare(const void* a, const void* b) {
    Cell* cellA = (Cell*)a;
    Cell* cellB = (Cell*)b;
    if (cellA->key < cellB->key) {
        return 0;
    } else {
        return 1;
    }
}

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

int allocateOverflowArea(unsigned int numberOfPages) {
    FILE* overflowFile = fopen(OVERFLOW_AREA_FILENAME, "r+b");
    unsigned int overflowAreaSize = (unsigned int)fmax(ceil((numberOfPages * BLOCKING_FACTOR_PAGE * OVERFLOW_AREA_RATIO)),BLOCKING_FACTOR_PAGE);
    for(unsigned int i = 0; i < overflowAreaSize; i++) {
        fprintf(overflowFile, "%010u;%-30.30s;%010u\n", 0, "", 0);
    }
    fclose(overflowFile);
    return 0;
}

int allocateNewOverflowArea(unsigned int numberOfPages) {
    FILE* overflowFile = fopen(OVERFLOW_AREA_NEW_FILE, "r+b");
    unsigned int overflowAreaSize = (unsigned int)fmax(ceil((numberOfPages * BLOCKING_FACTOR_PAGE * OVERFLOW_AREA_RATIO)),BLOCKING_FACTOR_PAGE);
    for(unsigned int i = 0; i < overflowAreaSize; i++) {
        fprintf(overflowFile, "%010u;%-30.30s;%010u\n", 0, "", 0);
    }
    fclose(overflowFile);
    return 0;
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
    long fileSize = 0;
    int isFileEmpty = 1;
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "r");
    fseek(indexFile, 0, SEEK_END);
    fileSize = ftell(indexFile);
    fclose(indexFile);
    if (fileSize > 0) {
        isFileEmpty = 0;
    } else {
        isFileEmpty = 1;
    }
    
    return isFileEmpty;
    return 0;
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
            currentOverflowPointer = 0;
        }

        fprintf(primaryFile, "%010u;%-30.30s;%010u\n", currentKey, currentData, currentOverflowPointer);
    }
    
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
    allocateOverflowArea(numberOfPages+1);
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
        int result = fscanf(indexFile, "%10u;%10u", &pageNo, &key); 
        if (result == 2) {
            page->indexEntry[i].pageNumber = pageNo;
            page->indexEntry[i].key = key;
        } else if (result == EOF) {
            //printf("EOF\n");
            fclose(indexFile);
            return 1;
        }
    }

    fclose(indexFile);
}

int getNewIndexPage(IndexPage* page, unsigned int index) {
    FILE* indexFile = fopen(INDEX_AREA_NEW_FILE, "r");
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
            //printf("EOF\n");
            fclose(indexFile);
            return 1;
        }
    }

    fclose(indexFile);
}

unsigned int getIndexOfPageToInsert(unsigned int key) {
    IndexPage page;
    for (int i = 0; i < numberOfPages; i++) {
        getIndexPage(&page, i);

        for (int j = 0; j < BLOCKING_FACTOR_PAGE; j++) {
            if (page.indexEntry[j].key == 0)
                return page.indexEntry[j - 1].pageNumber;
            if (key <= page.indexEntry[j].key) {
                if (j + 1 < BLOCKING_FACTOR_PAGE &&
                    page.indexEntry[j + 1].key != 0 &&
                    key <= page.indexEntry[j + 1].key) {
                    continue;
                }
                return page.indexEntry[j].pageNumber;
            }
        }
    }
    return page.indexEntry[BLOCKING_FACTOR_PAGE - 1].pageNumber;
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
        unsigned int currentOverflowPointer = 0;
        int result = fscanf(primaryFile, "%010u;%30c;%010u\n", &key, valueBuffer, &currentOverflowPointer); 
        if (result == 3) {
            page->cell[i].key = key;
            strncpy(page->cell[i].record.data, valueBuffer, MAX_RECORD_LENGTH);
            page->cell[i].overflowPointer = currentOverflowPointer;
        } else if (result == EOF) {
            //printf("EOF\n");
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
        unsigned int currentOverflowPointer = page.cell[i].overflowPointer;
        fprintf(primaryFile, "%010u;%-30.30s;%010u\n", currentKey, currentData, currentOverflowPointer); 
    }
    fclose(primaryFile);
    
    return 0; 
}

int writePageToNewPrimary(Page page, unsigned int index) {
    FILE* newPrimaryFile = fopen(PRIMARY_AREA_NEW_FILE, "r+b");
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    long offset = (long)index * pageSize;
    fseek(newPrimaryFile, offset, SEEK_SET);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        unsigned int currentKey = page.cell[i].key;
        const char* currentData = page.cell[i].record.data;
        unsigned int currentOverflowPointer = page.cell[i].overflowPointer;
        fprintf(newPrimaryFile, "%010u;%-30.30s;%010u\n", currentKey, currentData, currentOverflowPointer); 
    }
    fclose(newPrimaryFile);
    
    return 0; 
}

int writePageToOverflowArea(Page page, unsigned int currentOverflowIndex) {
    FILE* overflowFile = fopen(OVERFLOW_AREA_FILENAME, "r+b");
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    int minusOne = 1;
    if(currentOverflowIndex==0){
        minusOne = 0;
    }
    long offset = (long)(currentOverflowIndex-minusOne) * PRIMARY_RECORD_LENGTH;
    fseek(overflowFile, offset, SEEK_SET);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        unsigned int currentKey = page.cell[i].key;
        const char* currentData = page.cell[i].record.data;
        unsigned int currentOverflowPointer = page.cell[i].overflowPointer;
        fprintf(overflowFile, "%010u;%-30.30s;%010u\n", currentKey, currentData, currentOverflowPointer); 
    }
    fclose(overflowFile);
    return 0;
}

int readPageFromOverflowArea(Page* page, unsigned int index) {
    FILE* overflowFile = fopen(OVERFLOW_AREA_FILENAME, "rb"); 
    long pageSize = (long)PRIMARY_PAGE_LENGTH;
    long offset = (long)((index-1) * PRIMARY_RECORD_LENGTH);
    fseek(overflowFile, offset, SEEK_SET);
    memset(page, 0, sizeof(Page));  
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
        char valueBuffer[MAX_RECORD_LENGTH + 1] = {0}; 
        unsigned int key = 0;
        unsigned int currentOverflowPointer = 0;
        int result = fscanf(overflowFile, "%010u;%30c;%010u\n", &key, valueBuffer, &currentOverflowPointer); 
        if (result == 3) {
            page->cell[i].key = key;
            strncpy(page->cell[i].record.data, valueBuffer, MAX_RECORD_LENGTH);
            page->cell[i].overflowPointer = currentOverflowPointer;
        } else if (result == EOF) {
            //printf("EOF\n");
            break; 
        }
    }
    
    fclose(overflowFile);
    return 0;
}

int findRecordInOverflowArea(unsigned int overflowIndex, Cell searchCell, Cell* foundCell) {
    Page readPage;
    int isFound = 0;
    unsigned int index = overflowIndex;
    while (!isFound && readPageFromOverflowArea(&readPage, index)) {
        for (int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
            if (readPage.cell[i].key == searchCell.key) {
                isFound = 1;
                break;
            }
        }
        index++;
    }
    return isFound;
}

int writeCellToOverflow(Cell cell, unsigned int overflowIndexInstert, unsigned int* primaryPageRecordOverflowIndex) {
    FILE* overflowFile = fopen(OVERFLOW_AREA_FILENAME, "r+b");
    Page readPage;
    int isInserted = 0;
    unsigned int index = 0;
    unsigned int indexOfRecordInPage = 0;
    memset(&readPage, 0, sizeof(Page));
    while (!isInserted && !readPageFromOverflowArea(&readPage, index)) {
        for (int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
            indexOfRecordInPage = i;
            if (readPage.cell[i].key==0) {
                readPage.cell[i].key = cell.key;
                strncpy(readPage.cell[i].record.data, cell.record.data, MAX_RECORD_LENGTH);
                readPage.cell[i].overflowPointer = cell.overflowPointer;
                isInserted = 1;
                break;
            }
        }
        index++;
    }
    *primaryPageRecordOverflowIndex = index + indexOfRecordInPage;
    if(isInserted) {
        writePageToOverflowArea(readPage, overflowIndexInstert);
    }
    fclose(overflowFile);
    return 0;
}

int insertCellToFile(Cell cell) {
    Page readPage;
    unsigned int indexOfPage = getIndexOfPageToInsert(cell.key);
    unsigned int newOverflowIndex; 
    int insertedInPrimary = 0; 
    getPrimaryPage(&readPage, indexOfPage); 
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        if (readPage.cell[i].key == 0){
            readPage.cell[i].key = cell.key;
            strncpy(readPage.cell[i].record.data, cell.record.data, MAX_RECORD_LENGTH);
            readPage.cell[i].overflowPointer = 0;
            insertedInPrimary = 1;
            break;
        } else if (readPage.cell[i].key == cell.key) {
            printf("Record with that index exists, aborting\n");
            return 1; 
        } 

        if (readPage.cell[i].key < cell.key)  {
            unsigned int currentPrimaryOverflowPointer = readPage.cell[i].overflowPointer;
            Cell* prevCell = &readPage.cell[i];
            if (prevCell->overflowPointer != 0 && findRecordInOverflowArea(prevCell->overflowPointer, cell, NULL)) {
                printf("Record with that index exists in overflow area, aborting\n");
                return 1;
            }
            if (prevCell->overflowPointer == 0) {
                unsigned int overflowIndexForRecordInPrimaryFile = 0;
                if (writeCellToOverflow(cell, overflowIndexForRecordInPrimaryFile, &newOverflowIndex) == 0) {
                    readPage.cell[i].overflowPointer = newOverflowIndex; 
                    writePageToPrimary(readPage, indexOfPage);
                    mainOverflowCounter++;
                    return 0; 
                } else {
                    printf("Error: Overflow Area is full.\n");
                    return 1;
                }
            }  else {
                    unsigned int currentPtr = prevCell->overflowPointer;
                    unsigned int previousPtr = i; 
                    Page overflowPage;
                    Cell* currentOverflowCell = NULL;
                    
                    
                    while (currentPtr != 0) {
                        readPageFromOverflowArea(&overflowPage, currentPtr);
                        currentOverflowCell = &overflowPage.cell[0];
                    
                        if (currentOverflowCell->key > cell.key) { 
                            cell.overflowPointer = currentPrimaryOverflowPointer;
                            if (writeCellToOverflow(cell, currentPtr-1, &newOverflowIndex) == 0) {
                                
                                if (previousPtr == i) { 
                                    prevCell->overflowPointer = newOverflowIndex;
                                } else { 
                
                                }
                                writePageToPrimary(readPage, indexOfPage);
                                mainOverflowCounter++;
                                return 0; 
                            } else {
                                printf("Error: Overflow Area is full.\n");
                                return 1;
                            }
                        } 
                        
                        previousPtr = currentPtr;
                        currentPtr = currentOverflowCell->overflowPointer; 
                    }

                    if (writeCellToOverflow(cell, 0, &newOverflowIndex) == 0) {
                        currentOverflowCell->overflowPointer = newOverflowIndex;
                        writePageToOverflowArea(overflowPage, previousPtr); 
                        writePageToPrimary(readPage, indexOfPage);
                        return 0; 
                    } else {
                        printf("Error: Overflow Area is full.\n");
                        return 1;
                    }
                }
        }
    }

    if (insertedInPrimary) {
        writePageToPrimary(readPage, indexOfPage);
    }
    return 0; 
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

    printf("\n\nAddind record:\nkey: %u\nvalue: %s\n\n", newCell.key,newCell.record);

    if (!insertCell(newCell)) {
        //printf("Cell added succesfully\n");
        return 0;
    } else {
        //printf("Error - cell not added\n");
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
       // printf("Record added succesfully\n");
    } else {
        //printf("Error - record not added\n");
    }
}

unsigned int countNumberOfPages() {
    unsigned int countedNumberOfPages = 0;
    IndexPage readPage;
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "r");
    while(!feof(indexFile)){
        getIndexPage(&readPage, countedNumberOfPages);
        for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++){
            if(readPage.indexEntry[i].key != 0) {
                //printf("Key: %u\n", readPage.indexEntry[i].key);
                countedNumberOfPages++;
            } else {
                fclose(indexFile);
                return countedNumberOfPages;
            }
        }
    }
    fclose(indexFile);
    return countedNumberOfPages;
}

void clearFiles() {
    FILE* indexFile = fopen(PAGES_INDEXES_FILENAME, "w");
    FILE* primaryFile = fopen(PRIMARY_AREA_FILENAME, "w");
    FILE* overflowFile = fopen(OVERFLOW_AREA_FILENAME, "w");
    fclose(indexFile);
    fclose(primaryFile);
    fclose(overflowFile);
    printf("All files cleared\n");
}

int processCommand(char* inputBuffor) {
    numberOfPages = 0;
    numberOfPages = countNumberOfPages();
    if(!chackIsFileEmpty()) {
        numberOfPages = countNumberOfPages();
    } else {
        allocateOverflowArea(max(BLOCKING_FACTOR_PAGE,numberOfPages));
    }
    printf("Number of pages: %u\n", numberOfPages); 
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
    } else if (strcmp(mnemonic, "CLR") == 0) {
        clearFiles();
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

void createFiles() {
    FILE* newFileIndex = fopen(INDEX_AREA_NEW_FILE, "w");
    FILE* newFilePrimary = fopen(PRIMARY_AREA_NEW_FILE, "w");
    FILE* newFileOverflow = fopen(OVERFLOW_AREA_NEW_FILE, "w");
    fclose(newFileIndex);
    fclose(newFilePrimary);
    fclose(newFileOverflow);
    return;
}

void changeFilenames() {
    remove(PAGES_INDEXES_FILENAME);
    rename(INDEX_AREA_NEW_FILE, PAGES_INDEXES_FILENAME);
    remove(PRIMARY_AREA_FILENAME);
    rename(PRIMARY_AREA_NEW_FILE, PRIMARY_AREA_FILENAME);
    remove(OVERFLOW_AREA_FILENAME);
    rename(OVERFLOW_AREA_NEW_FILE, OVERFLOW_AREA_FILENAME);
    return;
}


void fillPageWithEmptyData(Page* page) {
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        page->cell[i].key = 0;
        page->cell[i].overflowPointer = 0;
        memset(page->cell[i].record.data, 0, MAX_RECORD_LENGTH);
    }
    return;
}

void writeIndexEntryToIndexFile(IndexEntry indexEntry) {
    IndexPage indexPage;
    int inserted = 0;
    unsigned int index;
    while(!inserted) {
        getIndexPage(&indexPage, index);
        for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
            if(indexPage.indexEntry->key == 0) {
                indexPage.indexEntry->key = indexEntry.key;
                indexPage.indexEntry->pageNumber = indexEntry.pageNumber;
            }
        }
        index++;
    }
}

void initIndexPageWithEmptyData(IndexPage* IndexEntry) {
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        IndexEntry->indexEntry->key = 0;
        IndexEntry->indexEntry->pageNumber = 0;
        IndexEntry->nextIndexPageId = 0;
    }
    return;
}

void writeIndexEntryToNewIndexFile(IndexEntry indexEntry) {
    IndexPage indexPage;
    int inserted = 0;
    unsigned int index;
    while(!inserted) {
        if(getNewIndexPage(&indexPage, index)){
            initIndexPageWithEmptyData(&indexPage);
        }
        for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
            if(indexPage.indexEntry->key == 0) {
                indexPage.indexEntry->key = indexEntry.key;
                indexPage.indexEntry->pageNumber = indexEntry.pageNumber;
                inserted = 1;
            }
        }
        index++;
    }
}

Cell takeBiggestRecordAndShrink(unsigned int *previousPointer) {
    unsigned int currentOverflowIndex = *previousPointer;
    if (currentOverflowIndex == 0) {
        Cell emptyCell = {0};
        return emptyCell;
    }
    Page readOverflowPage;
    readPageFromOverflowArea(&readOverflowPage, currentOverflowIndex);
    
    if (readOverflowPage.cell[0].overflowPointer != 0) {
        Cell cellToReturn = takeBiggestRecordAndShrink(&readOverflowPage.cell[0].overflowPointer);
        writePageToOverflowArea(readOverflowPage, currentOverflowIndex); 
        return cellToReturn;
    } else {
        Cell cellToReturn = readOverflowPage.cell[0];

        readOverflowPage.cell[0].key = 0;
        memset(readOverflowPage.cell[0].record.data, 0, MAX_RECORD_LENGTH);
        
        *previousPointer = 0; 

        writePageToOverflowArea(readOverflowPage, currentOverflowIndex);
        return cellToReturn;
    }
}

void writeIndexPageToIndexFile(IndexEntry IndexEntry, unsigned int indexPageIndex) {
    IndexPage indexPage;
    getNewIndexPage(&indexPage, indexPageIndex);
    for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        if(indexPage.indexEntry[i].key == 0) {
            indexPage.indexEntry[i].key = IndexEntry.key;
            indexPage.indexEntry[i].pageNumber = IndexEntry.pageNumber;
            break;
        }
    }
    FILE* indexFile = fopen(INDEX_AREA_NEW_FILE, "r+"); // Zmień na tryb tekstowy 'r+'
    
    // Obliczenie długości strony w trybie tekstowym (np. 100 rekordów * 23 bajty/rekord)
    long pageSize = (long)INDEX_FILE_POSITION_LENGHT; 
    long offset = (long)indexPageIndex * pageSize;
    
    fseek(indexFile, offset, SEEK_SET);
    
    for (int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
        unsigned int currentKey = indexPage.indexEntry[i].key;
        unsigned int currentPageNo = indexPage.indexEntry[i].pageNumber;
        fprintf(indexFile, "%010u;%010u\n", currentPageNo, currentKey); 
    }
    
    fclose(indexFile);
    return;
    return;
}

void reorganiseFile(){
    createFiles();
    unsigned int primaryPageIndex = 0;
    unsigned int indexPageIndex = 0;
    unsigned int numberOfPagesToReorganise = countNumberOfPages();
    int isTakenFromOVerflow = 0;
    int newPageSubIndex = 0;
    Page primaryPage;
    Page newPrimaryPage;
    fillPageWithEmptyData(&newPrimaryPage);
    while(primaryPageIndex < numberOfPagesToReorganise) {
        isTakenFromOVerflow = 0;
        getPrimaryPage(&primaryPage, primaryPageIndex);
        for(int i = 0; i < BLOCKING_FACTOR_PAGE; i++) {
            isTakenFromOVerflow = 0;
            if (primaryPage.cell[i].overflowPointer == 0 && primaryPage.cell[i].key != 0) { //sprawdzamy czy overflow jakis jest dla tego indeksu
                newPrimaryPage.cell[newPageSubIndex] = primaryPage.cell[i];
                primaryPage.cell[i].overflowPointer = 0;
                primaryPage.cell[i].key = 0;
                memset(primaryPage.cell[i].record.data, 0, MAX_RECORD_LENGTH);
                newPageSubIndex++;
            } else if (primaryPage.cell[i].overflowPointer != 0 && primaryPage.cell[i].key != 0) {
                newPrimaryPage.cell[newPageSubIndex] = takeBiggestRecordAndShrink(&primaryPage.cell[i].overflowPointer);
                isTakenFromOVerflow = 1;
                newPageSubIndex++;
            }
            if (newPageSubIndex >= (int)(BLOCKING_FACTOR_PAGE*ALFA_FACTOR)){
                IndexEntry newIndexEntry;
                newIndexEntry.key = newPrimaryPage.cell[0].key;
                newIndexEntry.pageNumber = indexPageIndex;
                writeIndexPageToIndexFile(newIndexEntry, indexPageIndex);
                writePageToNewPrimary(newPrimaryPage, indexPageIndex); //trzeba znieminc na taka z inna nazwa pliku
                indexPageIndex++;
                newPageSubIndex = 0;
                fillPageWithEmptyData(&newPrimaryPage);
            }
            if(isTakenFromOVerflow){
                i--;
            }
        }
        primaryPageIndex++;
    }
    allocateNewOverflowArea(primaryPageIndex);

    changeFilenames();
    mainOverflowCounter = 0;
    return;
}

int commandLineLoop() {
    int exit = 0;
    int showAfterOperation = 0;
    char inputBufor[MAX_COMMAND_LENGTH] = {0}; 
    printf("Write help for list of commands\n");
    printf("Write exit to exit the program\n");
    while(!exit) {
        if(mainOverflowCounter >= REORGANIZATION_OVERFLOW_SIZE) {
            reorganiseFile();
        }
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
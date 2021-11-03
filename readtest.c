#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"

//* index search
//* search from 317.blk to 348.blk (sorted S data)
//* build an index table

Buffer *buf;
unsigned char *wblk;
unsigned char *wblkbase;

//index table
struct table{
    int data;
    int indexBlock;
};

unsigned char *GetBlockdataAddress(int num, Buffer *buf) {
    //! get numth block's address in buffer
    if(num > 7 || num < 0) {
        perror("invalid number!\n");
        return NULL;
    }
    return buf->data + (buf->blkSize+1)*num + 1;
}

Buffer *InitBuffer(size_t bufSize, size_t blkSize, Buffer *buf) {
    //! package initBuffer
    if(!initBuffer(bufSize, blkSize, buf)) {
        perror("Buffer Initialization Failed!\n");
        exit(-1);
    }
    return buf;
}

unsigned char *ReadBlockFromDisk(unsigned int addr, Buffer *buf) {
    //! package readBlockFromDisk
    unsigned char *blk;
    if(!(blk = readBlockFromDisk(addr, buf))) {
        perror("Reading Block Failed!\n");
        exit(-1);
    }
    return blk;
}

void itostr(int num, char str[5]) {
    int i = 4;
    while(num) {
        int mod = num % 10;
        num = num / 10;
        str[--i] = '0' + mod;
    }
    while(i) str[--i] = '0';
    str[4] = '\0';
}

void ReadBlockData(unsigned char *blk, char str1[5], char str2[5]) {
    for(int k=0; k<4; k++) {
        str1[k] = *(blk + k);
        str2[k] = *(blk + k + 4);
    }
}

int main() {
    buf = (Buffer *)malloc(sizeof(Buffer));
    InitBuffer(520, 64, buf);
    
    //build an index table(100, 110, 120, 130)

    struct table t[4];
    t[0].data = 100;
    t[1].data = 110;
    t[2].data = 120;
    t[3].data = 130;
    int indexNum = 0;

    for(int n=0; n<4; n++) {
        for(int i=0; i<8; i++) {
            int addr = n*8 + i + 316 + 1;
            printf("addr=%d\n", addr);
            unsigned char *blkPtr = ReadBlockFromDisk(addr, buf);
            for(unsigned char *blk=blkPtr; blk<blkPtr+buf->blkSize-8; blk+=8) {
                // printf("-local=%d\n", blk-blkPtr);
                char str1[5];
                char str2[5];
                for(int k=0; k<4; k++) {
                    str1[k] = *(blk + k);
                    str2[k] = *(blk + k + 4);
                }
                printf("-str1=%s, str2=%s\n",str1, str2);
                int x = atoi(str1);
                if(x >= t[indexNum].data) {
                    // printf("--x=%d\n", x);
                    t[indexNum++].indexBlock = addr;
                    // ++indexNum;
                }
                if(indexNum == 4) {
                    freeBlockInBuffer(blkPtr, buf);
                    break;
                }
            }
            freeBlockInBuffer(blkPtr, buf);
            if(indexNum == 4) break;
        }
    }
    for(int i=0; i<4; i++) {
        // printf("t[%d].indexBlock=%d\n", i, t[i].indexBlock);
    }
    
    return 0;
}







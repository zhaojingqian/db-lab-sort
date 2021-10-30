#include <stdlib.h>
#include <stdio.h>
#include "extmem.h"

//* linea search
//* R:1.blk-16.blk
//* S:17.blk-48.blk
//* fetch from disk to memory and match
//* use 6 blocks to pick up, 2 blocks to store match data

//? store match data block
const int NUM = 4;
//? store blk
int numblk = 100;

Buffer *buf;
unsigned char *wblk;
unsigned char *wblkbase;

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

void WriteBlockData(int x, int y, Buffer *buf) {
    char str1[5];
    char str2[5];
    itostr(x, str1);
    itostr(y, str2);

    //whether write into disk
    if(wblk > buf->data+buf->bufSize) {
        //write into disk
        for(int i=NUM; i<8; i++) {
            unsigned char *blkPtr = GetBlockdataAddress(i, buf);
            writeBlockToDisk(blkPtr, numblk++, buf);
        }
        //restart
        wblk = GetBlockdataAddress(NUM, buf);
    }
    // whether jump or not
    // printf("local=%ld\n", (wblk-wblkbase)%65);

    if((wblk-wblkbase)%65 == 0) {
        printf("***\n");
        //valid bit, jump
        wblk++;
    }

    for(int k=0; k<4; k++) {
        // printf("k=%d, %s, %s\n", k, str1, str2);
        // printf("%s\n", wblk + k);
        *(wblk + k) = str1[k];
        *(wblk + k + 4) = str2[k];
    }
    wblk += 8;

    //jump nextdiskindex
    if((wblk-wblkbase)%65 == 57) {
        itostr(numblk+(wblk-wblkbase)/65+1, str1);
        printf("nextdisk=%ld\n", numblk+(wblk-wblkbase)/65+1);
        for(int k=0; k<4; k++) 
            *(wblk + k) = str1[k];
        wblk += 8;
    }
    // printf("--wblk=%p\n", wblk);
}

void ReadBlockData(unsigned char *blk, int *x, int *y) {
    char str[5];
    for(int i=0; i<7; i++) {
        for(int k=0; k<4; k++) 
            str[k] = *(blk + i*8 + k);
        *x = atoi(str);
        for(int k=0; k<4; k++) 
            str[k] = *(blk + i*8 + k + 4);
        *y = atoi(str);

        if(*x == 130) {
            printf("x=%d, y=%d\n", *x, *y);
            // write into block
            // printf("wblk=%p\n", wblk);
            WriteBlockData(*x, *y, buf);
        }
    }
}


int main() {
    buf = (Buffer *)malloc(sizeof(Buffer));
    InitBuffer(520, 64, buf);
    unsigned char *blk;
    wblk = GetBlockdataAddress(NUM, buf);
    wblkbase = wblk - 1;
    // printf("--wblk=%p\n", wblk);
    //each cycle
    for(int n=0; n<8; n++) {
        //batch process
        for(int i=0; i<4; i++) {
            unsigned int addr = n*4 + i + 1 + 16;
            // printf("begin read data/%d.blk\n", addr);
            ReadBlockFromDisk(addr, buf);
        }

        unsigned char *blkPtr = buf->data;
        while(blkPtr < buf->data + (buf->blkSize+1) * (buf->numAllBlk-4)) {
            //process
            int x, y;
            ReadBlockData(blkPtr+1, &x, &y);
            blkPtr += buf->blkSize + 1;
        }

        for(int j=0; j<4; j++) {
            blk = GetBlockdataAddress(j, buf);
            freeBlockInBuffer(blk, buf);
        }
    }

    for(int i=NUM; i<8; i++) {
        unsigned char *blkPtr = GetBlockdataAddress(i, buf);
        if(*(blkPtr+1)) 
            writeBlockToDisk(blkPtr, ++numblk, buf);
        else 
            break;
    }
    printf("search over, IO=%ld, store in disk%d\n", buf->numIO, numblk);
    return 0;

}
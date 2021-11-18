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
const int NUM = 7;
int numblk = 350;

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

void WriteBlockData(char str1[5], char str2[5], Buffer *buf) {

    //whether write into disk
    if(wblk > buf->data+buf->bufSize) {
        //write into disk
        for(int i=NUM; i<8; i++) {
            unsigned char *blkPtr = GetBlockdataAddress(i, buf);
            writeBlockToDisk(blkPtr, numblk++, buf);
            memset(blkPtr, '\0', sizeof(char)*buf->blkSize);
        }
        //restart
        wblk = GetBlockdataAddress(NUM, buf);
    }
    // whether jump or not
    // printf("local=%ld\n", (wblk-wblkbase)%65);

    if((wblk-wblkbase)%65 == 0) {
        // printf("***\n");
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
        char strnextdisk[5];
        itostr(numblk+(wblk-wblkbase)/65+1, strnextdisk);
        printf("nextdisk=%ld\n", numblk+(wblk-wblkbase)/65+1);
        for(int k=0; k<4; k++) 
            *(wblk + k) = strnextdisk[k];
        wblk += 8;
    }
    // printf("--wblk=%p\n", wblk);
}

int main() {
    buf = (Buffer *)malloc(sizeof(Buffer));
    InitBuffer(520, 64, buf);
    
    //build an index table(100, 110, 120, 130)
    wblk = GetBlockdataAddress(NUM, buf);
    wblkbase = wblk - 1;

    struct table t[4];
    t[0].data = 120;
    t[1].data = 130;
    t[2].data = 140;
    t[3].data = 150;
    int indexNum = 0;

    for(int n=0; n<4; n++) {
        for(int i=0; i<8; i++) {
            int addr = n*8 + i + 316 + 1;
            // printf("addr=%d\n", addr);
            unsigned char *blkPtr = ReadBlockFromDisk(addr, buf);
            for(unsigned char *blk=blkPtr; blk<blkPtr+buf->blkSize-8; blk+=8) {
                // printf("-local=%d\n", blk-blkPtr);
                char str1[5];
                char str2[5];
                for(int k=0; k<4; k++) {
                    str1[k] = *(blk + k);
                    str2[k] = *(blk + k + 4);
                }
                // printf("-str1=%s, str2=%s\n",str1, str2);
                int x = atoi(str1);
                if(x >= t[indexNum].data) {
                    // printf("--x=%d, indeNum=%d\n", x, indexNum);
                    t[indexNum].indexBlock = addr;
                    ++indexNum;
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
    unsigned char *indexblk = GetBlockdataAddress(0, buf);
    memset(indexblk, '\0', sizeof(char)*buf->blkSize);
    unsigned char *blk = indexblk;
    for(int i=0; i<4; i++) {
        char index[5], block[5];
        itostr(t[i].data, index);
        itostr(t[i].indexBlock, block);
        for(int k=0; k<4; k++) {
            *(blk + k) = index[k];
            *(blk + k + 4) = block[k];
        }
        blk += 8;
    }
    writeBlockToDisk(indexblk, 100, buf);


    int begin_io = buf->numIO;
    indexblk = readBlockFromDisk(100, buf);
    blk = indexblk;
    for(int i=0; i<4; i++) {
        char index[5], block[5];
        for(int k=0; k<4; k++) {
            index[k] = *(blk + k);
            block[k] = *(blk + k + 4);
        }
        t[i].data = atoi(index);
        t[i].indexBlock = atoi(block);
        blk += 8;
    }
    freeBlockInBuffer(indexblk, buf);

    // for(int i=0; i<4; i++) {
    //     printf("t[%d].indexBlock=%d\n", i, t[i].indexBlock);
    // }

    // find data 130's indexaddr
    int indexaddr = 0;
    for(int i=3; i>=0; i--) {
        if(t[i].data <= 130) {
            indexaddr = t[i].indexBlock;
            break;
        }
    }
    if(!indexaddr) {
        perror("cannot find data");
        return -1;
    } 

    int x = 0;
    unsigned char *blkPtr = readBlockFromDisk(indexaddr++, buf);
    for(unsigned char *blk=blkPtr; blk<blkPtr+buf->blkSize-8; blk+=8) {
        char str1[5];
        char str2[5];
        ReadBlockData(blk, str1, str2);
        x = atoi(str1);
        if(x == 130) {
            printf("x=%s, y=%s\n", str1, str2);
            WriteBlockData(str1, str2, buf);
        }
    }
    freeBlockInBuffer(blkPtr, buf);
    while(blkPtr = readBlockFromDisk(indexaddr++, buf)) {
        for(unsigned char *blk=blkPtr; blk<blkPtr+buf->blkSize-8; blk+=8) {
            char str1[5];
            char str2[5];
            ReadBlockData(blk, str1, str2);
            x = atoi(str1);
            if(x == 130) {
                printf("x=%s, y=%s\n", str1, str2);
                WriteBlockData(str1, str2, buf);
            } else {
                freeBlockInBuffer(blkPtr, buf);
                break;
            }
        }
        if(x != 130) break;
    }

    for(int i=NUM; i<8; i++) {
        unsigned char *blkPtr = GetBlockdataAddress(i, buf);
        if(*(blkPtr)) 
            writeBlockToDisk(blkPtr, ++numblk, buf);
        else 
            break;
    }

    printf("search over, IO=%ld, store in disk351-%d\n", buf->numIO-begin_io, numblk);
    return 0;
}

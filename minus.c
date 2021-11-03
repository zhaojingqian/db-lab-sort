#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"

Buffer *buf;
unsigned char *wblk;
unsigned char *wblkbase;
int fnumblk = 700;
int fnextdisknum = 701;
int count = 0;
int flag = 0;

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

unsigned char *FindSamePtr2(unsigned char *ptr1, unsigned char *ptr2) {
    if(!ptr1 && !ptr2) 
        return NULL;
    else if(!ptr1) 
        return ptr2;
    else if(!ptr2)
        return ptr1;
    else {
        char str11[5], str12[5], str21[5], str22[5];
        str12[4] = '\0';    str22[4] = '\0';
        for(int k=0; k<4; k++) {
            str11[k] = *(ptr1 + k);
            str12[k] = *(ptr1 + k + 4);
            str21[k] = *(ptr2 + k);
            str22[k] = *(ptr2 + k + 4);
        }
        
        if((strcmp(str11, str21)<0) || (strcmp(str11, str21)==0 && strcmp(str12, str22)<0))
            return ptr1;
        else if((strcmp(str11, str21)==0) && (strcmp(str12, str22)==0)) {
            // char str[5] = '0000';
            printf("(%s, %s), (%s, %s)\n", str11, str12, str21, str22);
            flag = 1;
            return ptr2;
        }
        else
            return ptr2;
    }
}

int main() {
    buf = (Buffer *)malloc(sizeof(Buffer));
    InitBuffer(520, 64, buf);

    wblk = GetBlockdataAddress(2, buf);
    wblkbase = wblk - 1;
    int mergeblock1 = 300;
    int mergeblock2 = 316;

    unsigned char *blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
    unsigned char *blkPtr2 = readBlockFromDisk(++mergeblock2, buf);

    while(blkPtr1 || blkPtr2) {
        if((blkPtr1-buf->data)%65==57) {
            printf("way-1 change!\n");
            freeBlockInBuffer(GetBlockdataAddress(0, buf), buf);
            if(mergeblock1 == 316) {
                printf("way-1 over!\n");               
                blkPtr1 = NULL;
                mergeblock1 = 317;
            } else {
                blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
            }
        }
        if((blkPtr2-buf->data)%65==57) {
            printf("way-2 change!\n");
            freeBlockInBuffer(GetBlockdataAddress(1, buf), buf);
            if(mergeblock2 == 348) {
                printf("way-2 over!\n");           
                blkPtr2 = NULL;
                mergeblock2 = 349;
            } else {
                blkPtr2 = readBlockFromDisk(++mergeblock2, buf);
            }
        }

        unsigned char *minPtr = FindSamePtr2(blkPtr1, blkPtr2);
        if(!minPtr) break;
        else {
            if(minPtr == blkPtr2) {
                if(flag == 1) {
                    flag = 0;  
                    blkPtr1 += 8;
                    blkPtr2 += 8;      
                } else {
                    ++count;
                    for(int k=0; k<4; k++) {
                        *(wblk + k) = *(minPtr + k);
                        *(wblk + k + 4) = *(minPtr + k + 4);
                    }
                    wblk += 8;
                    blkPtr2 += 8;
                }
            } else {
                blkPtr1 += 8;
            }
        }
        if((wblk-wblkbase)%65 == 57) {
            char strnextdisk[5];
            itostr(++fnextdisknum, strnextdisk);
            // printf("nextdisk=%ld\n", numblk+(wblk-wblkbase)/65+1);
            for(int k=0; k<4; k++) 
                *(wblk + k) = strnextdisk[k];
            wblk += 8;
            wblk++;
        }
        if(wblk > buf->data+buf->bufSize) {
            //! write into disk
            // printf("write into disk!\n");
            for(int i=2; i<8; i++) {
                unsigned char *cblk = GetBlockdataAddress(i, buf);
                writeBlockToDisk(cblk, ++fnumblk, buf);
                memset(cblk, '\0', sizeof(char)*buf->blkSize);
            }
            wblk = GetBlockdataAddress(2, buf);
        }   
    }
    for(int i=2; i<8; i++) {
        unsigned char *blkPtr = GetBlockdataAddress(i, buf);
        if(*(blkPtr)) {
            writeBlockToDisk(blkPtr, ++fnumblk, buf);
            memset(blkPtr, '\0', sizeof(char)*buf->blkSize);
        }
        else 
            break;
    }
    printf("the number of tuple after minus is %d, IO=%ld, store in disk701-%d\n", count, buf->numIO, fnumblk);
    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"

//* sort-merge-join


Buffer *buf;
unsigned char *wblk;
unsigned char *wblkbase;
int fnumblk = 400;
int fnextdisknum = 401;
int count = 0;

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

    //1 2 blocks to merge, 6 blocks to write
    wblk = GetBlockdataAddress(2, buf);
    wblkbase = wblk - 1;
    int mergeblock1 = 300;
    int mergeblock2 = 316;

    unsigned char *blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
    unsigned char *blkPtr2 = readBlockFromDisk(++mergeblock2, buf);
    // int countnum = 0;
    while(blkPtr1 && blkPtr2) {
        if((blkPtr1-buf->data)%65==57) {
            // printf("way-1 change!\n");
            freeBlockInBuffer(GetBlockdataAddress(0, buf), buf);
            if(mergeblock1 == 316) {
                // printf("way-1 over!\n");               
                blkPtr1 = NULL;
                mergeblock1 = 317;
            } else {
                blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
            }
        }
        if((blkPtr2-buf->data)%65==57) {
            // printf("way-2 change!\n");
            freeBlockInBuffer(GetBlockdataAddress(1, buf), buf);
            if(mergeblock2 == 348) {
                // printf("way-2 over!\n");           
                blkPtr2 = NULL;
                mergeblock2 = 349;
            } else {
                blkPtr2 = readBlockFromDisk(++mergeblock2, buf);
            }
        }
        if(!blkPtr1 || !blkPtr2) break;

        char strRA[5], strRB[5], strSC[5], strSD[5];
        ReadBlockData(blkPtr1, strRA, strRB);
        ReadBlockData(blkPtr2, strSC, strSD);

        // printf("RA=%s, SC=%s\n", strRA, strSC);
        
        if(strcmp(strRA, strSC) < 0) {
            //! RA < SC, move ptr1
            blkPtr1 += 8;
        } else if(strcmp(strRA, strSC) > 0) {
            //! RA > SC, move ptr2
            blkPtr2 += 8;
        } else if(strcmp(strRA, strSC) == 0) {
            // count++;
            //! RA = SC, write and move ptr1
            unsigned char *Rptr = blkPtr1;
            int Raddr = mergeblock1;
            while(strcmp(strRA, strSC) == 0) {
                count++;
                // printf("%d: (%s, %s), (%s, %s)\n", ++countnum, strSC, strSD, strRA, strRB);
                if((blkPtr1-buf->data)%65==57) {
                    // printf("way-1 change!\n");
                    freeBlockInBuffer(GetBlockdataAddress(0, buf), buf);
                    if(mergeblock1 == 316) {
                        // printf("way-1 over!\n");               
                        blkPtr1 = NULL;
                        mergeblock1 = 317;
                    } else {
                        blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
                    }
                }

                for(int k=0; k<4; k++) {
                    *(wblk + k) = *(blkPtr2 + k);
                    *(wblk + k + 4) = *(blkPtr2 + k + 4);
                }
                wblk += 8;
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

                for(int k=0; k<4; k++) {
                    *(wblk + k) = *(blkPtr1 + k);
                    *(wblk + k + 4) = *(blkPtr1 + k + 4);
                }  
                wblk += 8;
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
                blkPtr1 += 8; 
                ReadBlockData(blkPtr1, strRA, strRB);
                ReadBlockData(blkPtr2, strSC, strSD);
            }
            blkPtr1 = Rptr;
            mergeblock1 = Raddr;
            freeBlockInBuffer(GetBlockdataAddress(0, buf), buf);
            readBlockFromDisk(Raddr, buf);
            blkPtr2 += 8;
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
    printf("the number of tuple after join is %d, IO=%ld, store in disk401-%d\n", count, buf->numIO, fnumblk);
    return 0;
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "extmem.h"

//* R 2-merge 
//* S 4-merge
//* store in block 200+
int numblk = 200;
int nextdisknum = 201;
int fnumblk = 300;
int fnextdisknum = 301;
const int NUM = 4;
Buffer *buf;
unsigned char *wblk;
unsigned char *wblkbase;

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

unsigned char *GetBlockdataAddress(int num, Buffer *buf) {
    //! get numth block's address in buffer
    if(num > 7 || num < 0) {
        perror("invalid number!\n");
        return NULL;
    }
    return buf->data + (buf->blkSize+1)*num + 1;
}

void ReadBlockData(unsigned char *blk, char str1[5], char str2[5]) {
    int x, y;
    for(int i=0; i<7; i++) {
        for(int k=0; k<4; k++) {
            str1[k] = *(blk + i*8 + k);
            str2[k] = *(blk + i*8 + k + 4);
        }
    }
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

int main() {
    buf = (Buffer *)malloc(sizeof(Buffer));
    InitBuffer(520, 64, buf);
    unsigned char *blk;
    // wblk = GetBlockdataAddress(NUM, buf);
    // wblkbase = wblk - 1;
    
    /** R **/
    //* first stage
    for(int n=0; n<2; n++) {
        //* read 8 blocks each time
        for(int i=0; i<8; i++) {
            unsigned int addr = n*8 + i + 1;
            ReadBlockFromDisk(addr, buf);
        }
        //* inside sort (selected sot)
        for(unsigned char *blkPtr=buf->data+1; blkPtr<buf->data+buf->bufSize; blkPtr+=8) {
            //! jump nextdisk area
            if((blkPtr-buf->data)%65 == 57) {
                char nextdisk[5];
                itostr(++nextdisknum, nextdisk);
                for(int k=0; k<4; k++) {
                    *(blkPtr + k) = nextdisk[k];
                }
                blkPtr++;
                continue;
            }
            unsigned char *minPtr = blkPtr; 
            char minstr1[5];
            char minstr2[5];
            for(int k=0; k<4; k++) {
                minstr1[k] = *(minPtr + k);
                minstr2[k] = *(minPtr + k + 4);
            }
            // printf("n=%d, curmin=%s, %s\n", n, minstr1, minstr2);

            for(unsigned char *curPtr=blkPtr+8; curPtr<buf->data+buf->bufSize; curPtr+=8) {
                if((curPtr-buf->data)%65 == 57) {
                    curPtr++;
                    continue;
                }
                // find smallest tab
                char curstr1[5];
                char curstr2[5];
                for(int k=0; k<4; k++) {
                    curstr1[k] = *(curPtr + k);
                    curstr2[k] = *(curPtr + k + 4);
                }
                if((strcmp(curstr1, minstr1)<0) || (strcmp(curstr1, minstr1)==0 && strcmp(curstr2, minstr2)<0)) {
                    strcpy(minstr1, curstr1);
                    strcpy(minstr2, curstr2);
                    minPtr = curPtr;
                }
            }

            //! swap data
            if(minPtr != blkPtr) {
                for(int k=0; k<4; k++) {
                    char temp1;
                    char temp2;
                    temp1 = *(blkPtr + k);
                    *(blkPtr + k) = *(minPtr + k);
                    *(minPtr + k) = temp1;

                    temp2 = *(blkPtr + k + 4);
                    *(blkPtr + k + 4) = *(minPtr + k + 4);
                    *(minPtr + k + 4) = temp2;
                }
            }
            // printf("n=%d, curmin=%s, %s\n", n, minstr1, minstr2);
        }

        //* write back
        //* free 8 blocks for next read
        for(int i=0; i<8; i++) {
            unsigned char *cblk = GetBlockdataAddress(i, buf);
            writeBlockToDisk(cblk, ++numblk, buf);
            // freeBlockInBuffer(cblk, buf);
        }
    }
    //* second stage
    // R is 2-merge, use 2 blocks to sort, 6 blocks to write
    // unsigned char *blk1 = GetBlockdataAddress(0, buf);
    // unsigned char *blk2 = GetBlockdataAddress(1, buf);

    // for(int i=2; i<8; i++) {
    //     wblk = GetBlockdataAddress(i, buf);
    //     *(wblk-1) = BLOCK_UNAVAILABLE;
    // }
    wblk = GetBlockdataAddress(2, buf);
    wblkbase = wblk - 1;
    int mergeblock1 = 200;
    int mergeblock2 = 208;

    //put first 2 blocks into buffer
    unsigned char *blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
    unsigned char *blkPtr2 = readBlockFromDisk(++mergeblock2, buf);

    // printf("***%s, %s***\n", blkPtr1, blkPtr2);

    // unsigned char *blkPtr1 = blk1;
    // unsigned char *blkPtr2 = blk2;

    while(blkPtr1 || blkPtr2) {

        printf("--%d, %d--\n", mergeblock1, mergeblock2);

        if((blkPtr1-buf->data)%65==57) {
            printf("way-1 change!\n");
            if(mergeblock1 == 208) {
                printf("way-1 over!\n");
                blkPtr1 = NULL;
                mergeblock1 = 209;
            } else {
                freeBlockInBuffer(GetBlockdataAddress(0, buf), buf);
                blkPtr1 = readBlockFromDisk(++mergeblock1, buf);
            }
        }
        if((blkPtr2-buf->data)%65==57) {
            printf("way-2 change!\n");
            if(mergeblock2 == 216) {
                printf("way-2 over!\n");
                blkPtr2 = NULL;
                mergeblock2 = 217;
            } else {
                freeBlockInBuffer(GetBlockdataAddress(1, buf), buf);
                blkPtr2 = readBlockFromDisk(++mergeblock2, buf);
            }
        }
        
        if(!blkPtr1 && !blkPtr2) 
            break;
        else if(!blkPtr1) {
            for(int k=0; k<4; k++) {
                *(wblk + k) = *(blkPtr2 + k);
                *(wblk + k+ 4) = *(blkPtr2 + k + 4);
            }
            wblk += 8;
            blkPtr2 += 8;
        } else if(!blkPtr2) {
            for(int k=0; k<4; k++) {
                *(wblk + k) = *(blkPtr1 + k);
                *(wblk + k+ 4) = *(blkPtr1 + k + 4);
            }
            wblk += 8;
            blkPtr1 += 8;
        } else {
            char str11[5], str12[5], str21[5], str22[5];
            for(int k=0; k<4; k++) {
                str11[k] = *(blkPtr1 + k);
                str12[k] = *(blkPtr1 + k + 4);
                str21[k] = *(blkPtr2 + k);
                str22[k] = *(blkPtr2 + k + 4);
            }
            // printf("%s, %s\n", str11, str21);
            if((strcmp(str11, str21)<0) || (strcmp(str11, str21)==0 && strcmp(str12, str22)<0)) {
                // printf("---%s, %s---\n", str21, str22);
                for(int k=0; k<4; k++) {
                    *(wblk + k) = *(blkPtr1 + k);
                    *(wblk + k+ 4) = *(blkPtr1 + k + 4);
                }
                wblk += 8;
                blkPtr1 += 8;
            } else {
                // printf("---%s, %s---\n", str21, str22);
                for(int k=0; k<4; k++) {
                    *(wblk + k) = *(blkPtr2 + k);
                    *(wblk + k+ 4) = *(blkPtr2 + k + 4);
                }
                wblk += 8;
                blkPtr2 += 8;     
            }
        }

        // printf("local=%ld\n", (wblk-wblkbase)%65);
        if((wblk-wblkbase)%65 == 57) {
            char strnextdisk[5];
            itostr(++fnextdisknum, strnextdisk);
            // printf("nextdisk=%ld\n", numblk+(wblk-wblkbase)/65+1);
            for(int k=0; k<4; k++) 
                *(wblk + k) = strnextdisk[k];
            wblk += 8;
            wblk++;
        }

        // judge if write into disk or not
        if(wblk > buf->data+buf->bufSize) {
            //! write into disk
            printf("write into disk!\n");
            for(int i=2; i<8; i++) {
                unsigned char *cblk = GetBlockdataAddress(i, buf);
                writeBlockToDisk(cblk, ++fnumblk, buf);
                memset(cblk, '\0', sizeof(char)*buf->blkSize);
            }
            wblk = GetBlockdataAddress(2, buf);
        }
        // printf("--wblk=%p, blk1=%p, blk2=%p--\n", wblk, blkPtr1, blkPtr2);
    }

    for(int i=2; i<8; i++) {
        unsigned char *blkPtr = GetBlockdataAddress(i, buf);
        if(*(blkPtr)) 
            writeBlockToDisk(blkPtr, ++fnumblk, buf);
        else 
            break;
    }

    return 0;
}


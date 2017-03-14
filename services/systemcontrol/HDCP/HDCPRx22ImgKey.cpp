/*
 * \file        HDCPRx22ImgKey.cpp
 * \brief
 *
 * \version     1.0.0
 * \date        16/12/1
 * \author      Xindongxu <xindong.xu@amlgic.com>
 *
 * Copyright (c) 2016 Amlogic. All Rights Reserved.
 *
 */
#define LOG_TAG "SystemControl"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <utils/Log.h>
//#include "crc32.h"
#include "HDCPRx22ImgKey.h"
#include "HDCPRxKey.h"


#define HDCP_LOGD(...)  ALOGD(__VA_ARGS__)
#define HDCP_LOGE(x, ...)   ALOGE("[%s, %d] " x, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define MAX_PATH 256

#define errorP HDCP_LOGE

#define COMPILE_TYPE_CHK(expr, t)       typedef char t[(expr) ? 1 : -1]

COMPILE_TYPE_CHK(AML_RES_IMG_HEAD_SZ == sizeof(AmlResImgHead_t), a);//assert the image header size 64
COMPILE_TYPE_CHK(AML_RES_ITEM_HEAD_SZ == sizeof(AmlResItemHead_t), b);//assert the item head size 64

#define IMG_HEAD_SZ     sizeof(AmlResImgHead_t)
#define ITEM_HEAD_SZ    sizeof(AmlResItemHead_t)
//#define ITEM_READ_BUF_SZ    (1U<<20)//1M
#define ITEM_READ_BUF_SZ    (64U<<10)//64K to test

static unsigned add_sum(const void* pBuf, const unsigned size, unsigned int sum)
{
    const unsigned* data = (const unsigned*)pBuf;
    unsigned wordLen = size >> 2;
    unsigned rest = size & 3;

    for (; wordLen / 4; wordLen -= 4) {
        sum += *data++;
        sum += *data++;
        sum += *data++;
        sum += *data++;
    }
    while (wordLen--) {
        sum += *data++;
    }

    if (rest == 1) {
        sum += (*data) & 0xff;
    }
    else if (rest == 2) {
        sum += (*data) & 0xffff;
    }
    else if (rest == 3) {
        sum += (*data) & 0xffffff;
    }

    return sum;
}

//Generate crc32 value with file steam, which from 'offset' to end if checkSz==0
unsigned calc_img_crc(FILE* fp, off_t offset, unsigned checkSz)
{
    unsigned char* buf = NULL;
    unsigned MaxCheckLen = 0;
    unsigned totalLenToCheck = 0;
    const int oneReadSz = 12 * 1024;
    unsigned int crc = 0;

    if (fp == NULL) {
        fprintf(stderr,"bad param!!\n");
        return 0;
    }

    buf = (unsigned char*)malloc(oneReadSz);
    if (!buf) {
        errorP("Fail in malloc for sz %d\n", oneReadSz);
        return 0;
    }

    fseeko(fp, 0, SEEK_END);
    MaxCheckLen = ftell(fp);
    MaxCheckLen -= offset;
    if (!checkSz) {
        checkSz = MaxCheckLen;
    }
    else if (checkSz > MaxCheckLen) {
        fprintf(stderr, "checkSz %u > max %u\n", checkSz, MaxCheckLen);
        free(buf);
        return 0;
    }
    fseeko(fp, offset, SEEK_SET);

    while (totalLenToCheck < checkSz)
    {
        int nread;
        unsigned leftLen = checkSz - totalLenToCheck;
        int thisReadSz = leftLen > oneReadSz ? oneReadSz : leftLen;

        nread = fread(buf, 1, thisReadSz, fp);
        if (nread < 0) {
            fprintf(stderr, "%d:read %s.\n", __LINE__, strerror(errno));
            free(buf);
            return 0;
        }
        crc = add_sum(buf, thisReadSz, crc);

        totalLenToCheck += thisReadSz;
    }

    free(buf);
    return crc;
}

static char generalDataChange(const char input)
{
    int i;
    char result = 0;

    for (i = 0; i < 8; i++) {
        if ((input & (1 << i)) != 0)
            result |= (1 << (7 - i));
        else
            result &= ~(1 << (7 - i));
    }
    return result;
}

static void hdcp2DataEncryption(const unsigned len, const char *input, char *out)
{
    unsigned int i = 0;

    for (i = 0; i < len; i++)
        *out++ = generalDataChange(*input++);
}

static void hdcp2DataDecryption(const unsigned len, const char *input, char *out)
{
     unsigned int i = 0;

     for (i = 0; i < len; i++)
         *out++ = generalDataChange(*input++);
}

static size_t get_filesize(const char *fpath)
{
    struct stat buf;
    if (stat(fpath, &buf) < 0) {
        HDCP_LOGE("Can't stat %s : %s\n", fpath, strerror(errno));
        return 0;
    }
    return buf.st_size;
}

int readSys(const char *path, char *buf, int count) {
    int fd, len = -1;

    if ( NULL == buf ) {
        HDCP_LOGE("buf is NULL");
        return len;
    }

    if ((fd = open(path, O_RDONLY)) < 0) {
        HDCP_LOGE("readSysFs, open %s fail.", path);
        return len;
    }

    len = read(fd, buf, count);
    if (len < 0) {
        HDCP_LOGE("read error: %s, %s\n", path, strerror(errno));
    }

    close(fd);
    return len;
}

int writeSys(const char *path, const char *val) {
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        HDCP_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    write(fd, val, strlen(val));
    close(fd);
    return 0;
}

int writeSysBin(const char *path, const char *val, const int size) {
    int fd;

    if ((fd = open(path, O_WRONLY)) < 0) {
        HDCP_LOGE("writeSysFs, open %s fail.", path);
        return -1;
    }

    if (write(fd, val, size) != size) {
        HDCP_LOGE("write %s size:%d failed!\n", path, size);
        return -1;
    }

    close(fd);
    return 0;
}

//Generate crc32 value with file steam, which from 'offset' to end if checkSz==0
unsigned storage_calc_img_crc(char *pbuf, int bufLen, unsigned checkSz)
{
    unsigned int crc = 0;
    if (checkSz == 0) {
        checkSz = bufLen;
    }
    else if ((int)checkSz > bufLen) {
        HDCP_LOGE("checkSz %u > max %u\n", checkSz, bufLen);
        return 0;
    }

    crc = add_sum(pbuf, checkSz, crc);
    return crc;
}

static int write_hdcp_key(const char *data, const char *key_name, const int size)
{
    int keyLen;
    char existKey[10] = {0};

    if (writeSys(UNIFYKEY_ATTACH, "1")) {
        errorP("attach failed!\n");
        return -1;
    }

    if (writeSys(UNIFYKEY_NAME, key_name)) {
        errorP("name failed!\n");
        return -1;
    }

    if (writeSysBin(UNIFYKEY_WRITE, data, size) == -1) {
        errorP("write failed!\n");
        return -1;
    }

    readSys(UNIFYKEY_EXIST, (char*)existKey, 10);
    if (0 == strcmp(existKey, "0")) {
        errorP("get status: not burned!\n");
        return -1;
    }

    return 0;
}

void dump_keyitem_info(struct key_item_info_t *info)
{
    if (info == NULL)
        return;
    errorP("id: %d\n", info->id);
    errorP("name: %s\n", info->name);
    errorP("size: %d\n", info->size);
    errorP("permit: 0x%x\n", info->permit);
    errorP("flag: 0x%x\n", info->flag);
    return;
}

void dump_mem(unsigned char * buffer, int count)
{
    int i;
    if (NULL == buffer || count == 0) {
        errorP("%s() %d: %p, %d", __func__, __LINE__, buffer, count);
        return;
    }
    for (i = 0; i < count; i += 16) {
        if (i % 256 == 0)
            errorP("\n");
        errorP("%02x ", buffer[i]);
    }
    errorP("\n");
}

int setImgPath(const char *path)
{
    int ret = 0;
    unsigned int num = 0;
    int result = -1;
    FILE* fdImg = NULL;
    unsigned int crc32 = 0;
    AmlResImgHead_t *pImgHead = NULL;
    AmlResItemHead_t *pItemHead = NULL;

    if (path == NULL) {
        errorP("Fail path(%s) is null\n", path);
        return -1;
    }

    fdImg = fopen(path, "rb");
    if (!fdImg) {
        errorP("Fail to open res image at path %s\n", path);
        return -1;
    }

    char* itemReadBuf = (char*)malloc(ITEM_READ_BUF_SZ);
    if (!itemReadBuf) {
        errorP("Fail to malloc buffer at size 0x%x\n", ITEM_READ_BUF_SZ);
        fclose(fdImg);
        return -1;
    }

    int actualReadSz = 0;
    actualReadSz = fread(itemReadBuf, 1, IMG_HEAD_SZ, fdImg);
    if (actualReadSz != IMG_HEAD_SZ) {
        errorP("Want to read %lu, but only read %d\n", IMG_HEAD_SZ, actualReadSz);
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    pImgHead = (AmlResImgHead_t *)itemReadBuf;

    if (strncmp(AML_RES_IMG_V1_MAGIC, (char*)pImgHead->magic, AML_RES_IMG_V1_MAGIC_LEN)) {
        errorP("magic error.\n");
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    crc32 = calc_img_crc(fdImg, 4, pImgHead->imgSz - 4);
    if (pImgHead->crc != crc32) {
        errorP("Error when check crc\n");
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    fseek(fdImg, IMG_HEAD_SZ, SEEK_SET);
    int ItemHeadSz = (pImgHead->imgItemNum)*ITEM_HEAD_SZ;
    actualReadSz = fread(itemReadBuf+IMG_HEAD_SZ, 1, ItemHeadSz, fdImg);
    if (actualReadSz != ItemHeadSz) {
        errorP("Want to read 0x%x, but only read 0x%x\n", ItemHeadSz, actualReadSz);
        fclose(fdImg);
        free(itemReadBuf);
        return -1;
    }

    pItemHead = (AmlResItemHead_t *)(itemReadBuf + IMG_HEAD_SZ);
    for (num = 0; num < pImgHead->imgItemNum; num++, pItemHead++)
    {
        errorP("pItemHead->name:%s\n", pItemHead->name);
        errorP("pItemHead->size:%d\n", pItemHead->dataSz);
        errorP("pItemHead->dataOffset:%d\n", pItemHead->dataOffset);

        if (!strcmp(pItemHead->name, HDCP_RX_PRIVATE)) {
            char *tmpbuffer = (char *)malloc(pItemHead->dataSz + 4);
            if (!tmpbuffer) {
                errorP("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz + 4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }
            char *writebuffer = (char *)malloc(pItemHead->dataSz + 4);
            if (!writebuffer) {
                errorP("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz + 4);
                fclose(fdImg);
                free(itemReadBuf);
                free(tmpbuffer);
                return -1;
            }

            memset(tmpbuffer, 0, pItemHead->dataSz + 4);
            memset(writebuffer, 0, pItemHead->dataSz + 4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            unsigned int readlen = fread(tmpbuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(tmpbuffer);
                free(writebuffer);
                return -1;
            }

            for (unsigned int i = 0; i < pItemHead->dataSz; i++) {
                errorP("tmpbuffer[%d]:%x\n", i, ((unsigned char *)tmpbuffer)[i]);
            }

            hdcp2DataDecryption(pItemHead->dataSz, (char *)tmpbuffer, (char *)writebuffer);
            free(tmpbuffer);

            for (unsigned int i = 0; i < pItemHead->dataSz; i++) {
                errorP("writebuffer[%d]:%x\n", i, ((unsigned char *)writebuffer)[i]);
            }

            result = write_hdcp_key(writebuffer, HDCP_RX_PRIVATE, pItemHead->dataSz);
            free(writebuffer);
            if (result) {
                errorP("write hdcp key failed1!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            else
            {
                errorP("write hdcp key OK1!\n");
            }
        }else if (!strcmp(pItemHead->name, HDCP_RX)) {
            #if 1
            char *writebuffer = (char *)malloc(pItemHead->dataSz + 4);
            if (!writebuffer) {
                errorP("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz + 4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }

            memset(writebuffer, 0, pItemHead->dataSz + 4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            unsigned int readlen = fread(writebuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(writebuffer);
                return -1;
            }

            result = write_hdcp_key(writebuffer, HDCP_RX, pItemHead->dataSz);
            free(writebuffer);
            if (result) {
                errorP("write hdcp key failed2!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            #endif
        } else if (!strcmp(pItemHead->name, HDCP_RX_FW)) {
            #if 1
            char *writebuffer = (char *)malloc(pItemHead->dataSz + 4);
            if (!writebuffer) {
                errorP("Fail to malloc buffer  size 0x%x\n", pItemHead->dataSz + 4);
                fclose(fdImg);
                free(itemReadBuf);
                return -1;
            }

            memset(writebuffer, 0, pItemHead->dataSz + 4);
            fseek(fdImg, pItemHead->dataOffset, SEEK_SET);
            unsigned int readlen = fread(writebuffer, 1, pItemHead->dataSz, fdImg);
            if (readlen != pItemHead->dataSz) {
                fclose(fdImg);
                free(itemReadBuf);
                free(writebuffer);
                return -1;
            }

            result = write_hdcp_key(writebuffer, "hdcp22_rx_fw", pItemHead->dataSz);

            HDCPRxKey hdcpRx22(HDCP_RX_22_KEY);
            hdcpRx22.refresh();
            writeSysBin(HDCP_RX_DEBUG_PATH, "load22key", 10);

            free(writebuffer);
            if (result) {
                errorP("write hdcp key failed3!\n");
                free(itemReadBuf);
                fclose(fdImg);
                return -1;
            }
            #endif
        }
    }

    fclose(fdImg);
    free(itemReadBuf);
    return result;
}


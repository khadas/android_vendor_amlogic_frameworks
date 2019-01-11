#define LOG_TAG "MiracastService"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <utils/Log.h>

#include "SysTokenizer.h"
#include "MiracastService.h"
#define EVENT_NUM 12

const char *event_str[EVENT_NUM] =
{
    "IN_ACCESS",
    "IN_MODIFY",
    "IN_ATTRIB",
    "IN_CLOSE_WRITE",
    "IN_CLOSE_NOWRITE",
    "IN_OPEN",
    "IN_MOVED_FROM",
    "IN_MOVED_TO",
    "IN_CREATE",
    "IN_DELETE",
    "IN_DELETE_SELF",
    "IN_MOVE_SELF"
};

MiracastService::MiracastService():
    mNotifyListener(NULL),
    mfd(0),
    mwd(0) {
    init();
    pthread_t id;
    int ret = pthread_create(&id, NULL, FileObserverThread, this);
    if (ret != 0) {
        ALOGE("Create FileObserver thread error!\n");
    }
    ALOGD("create FileObserver success\n");
}

MiracastService::~MiracastService() {

}

void MiracastService::setNotifyCallback(const sp<ServiceNotify>& listener) {
    mNotifyListener = listener;
}

int MiracastService::init() {
    mfd = inotify_init();
    if (mfd < 0) {
        ALOGE("inotify_init failed\n");
        return -1;
    }
    return 0;
}

int MiracastService::startWatching(const std::string& path, int mask) {
    strcpy(pathName, path.c_str());
    mwd = inotify_add_watch(mfd, path.c_str(), mask);
    if (mwd < 0) {
        ALOGE("inotify_add_watch %s failed\n", path.c_str());
        return -1;
    }
    return 0;
}

void MiracastService::stopWatching() {
    inotify_rm_watch(mfd, (uint32_t)mwd);
}

std::vector<std::string> MiracastService::parseDnsmasqAddr(const char* name, const char* event) {
    const char* WHITESPACE = " \t\r";
    SysTokenizer* tokenizer = NULL;
    char mac[32] = {0};
    char ip[32]  = {0};
    std::vector<std::string> addrlist;
    char fullpath[512] = {0};
    if (name != NULL && !strcmp(name, "dnsmasq.leases")) {
        sprintf(fullpath, "%s/%s", pathName, name);
        ALOGD("full path = %s\n", fullpath);

        int status = SysTokenizer::open(fullpath, &tokenizer);
        if (status) {
            ALOGE("Error %d opening dnsmasq leases file %s.", status);
        } else {
            while (!tokenizer->isEof()) {
                ALOGI("Parsing %s: %s", tokenizer->getLocation(), tokenizer->peekRemainderOfLine());
                tokenizer->skipDelimiters(WHITESPACE);
                if (!tokenizer->isEol()) {
                    char *token = tokenizer->nextToken(WHITESPACE);
                    if (token != NULL) {
                        tokenizer->skipDelimiters(WHITESPACE);
                        strcpy(mac, tokenizer->nextToken(WHITESPACE));
                        tokenizer->skipDelimiters(WHITESPACE);
                        strcpy(ip, tokenizer->nextToken(WHITESPACE));
                        ALOGI("token = %s, MAC = %s, IP = %s", token, mac, ip);
                        addrlist.push_back(ip);
                        addrlist.push_back(mac);
                    }
                }
                tokenizer->nextLine();
            }
        }
        delete tokenizer;
    }
    //debug
    for (size_t i=0; i< addrlist.size(); i++) {
        ALOGI("AddrList[%d] = %s", i, addrlist[i].c_str());
    }
    return addrlist;
}

void* MiracastService::FileObserverThread(void* data) {
    MiracastService *pthiz = (MiracastService *)data;
    //int fd;
    //int wd;
    int len;
    int nread;
    char buf[512] = {0};
    struct inotify_event *event;
    int i;
    std::vector<std::string> AddrList;

    buf[sizeof(buf) - 1] = 0;
    while ((len = read(pthiz->mfd, buf, sizeof(buf) - 1)) > 0) {
        nread = 0;
        while (len > 0) {
            event = (struct inotify_event *)&buf[nread];
            for (i = 0; i<EVENT_NUM; i++) {
                if ((event->mask >> i) & 1) {
                    if (event->len > 0) {
                        ALOGD("%s --- %s\n", event->name, event_str[i]);
                        AddrList = pthiz->parseDnsmasqAddr(event->name, event_str[i]);
                        pthiz->mNotifyListener->onEvent(AddrList);
                    }
                    else
                        ALOGD("%s --- %s\n", " ", event_str[i]);
                }
            }
            nread = nread + sizeof(struct inotify_event) + event->len;
            len = len - sizeof(struct inotify_event) - event->len;
        }
   }
   return NULL;
}


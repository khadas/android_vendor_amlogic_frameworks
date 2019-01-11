#ifndef _MIRACAST_SERVICE_H
#define _MIRACAST_SERVICE_H
#include <vector>
#include <string>
#include <utils/RefBase.h>
#include <utils/Mutex.h>
#include <utils/Log.h>

using namespace android;
class ServiceNotify : virtual public RefBase {
public:
    ServiceNotify() {}
    virtual ~ServiceNotify(){}
    virtual void onEvent(const std::vector<std::string> &list) = 0;
};

class MiracastService {
public:
    MiracastService();
    virtual ~MiracastService();
    void setNotifyCallback(const sp<ServiceNotify>& listener);
    int init();
    int startWatching(const std::string& path, int mask);
    void stopWatching();
private:
    static void* FileObserverThread(void* data);
    std::vector<std::string> parseDnsmasqAddr(const char* name, const char* event);
    sp<ServiceNotify> mNotifyListener;
    int mfd;
    int mwd;
    char pathName[64] = {0};
};
#endif

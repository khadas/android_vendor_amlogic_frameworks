#ifndef DROIDLOGIC_ICALLBACK_H_
#define DROIDLOGIC_ICALLBACK_H_

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include "HdmiCecBase.h"

namespace android {


class IHdmiCecCallback : public IInterface {
public:
    DECLARE_META_INTERFACE(HdmiCecCallback);
    virtual void notifyCallback(const hdmi_cec_event_t* event) = 0;
};


class BnHdmiCecCallback : public BnInterface<IHdmiCecCallback> {
public:
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags = 0);
};

};//namespace android


#endif /* DROIDLOGIC_ICALLBACK_H_ */

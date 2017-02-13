package com.droidlogic.app.tv;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;
import java.util.Comparator;

import android.util.Log;
import android.os.Bundle;
import android.content.Context;
import android.media.tv.TvContract;
import android.provider.Settings;

public abstract class TvStoreManager {
    public static final String TAG = "TvStoreManager";

    private Context mContext;
    private TvDataBaseManager mTvDataBaseManager;

    private String mInputId;
    private int mInitialDisplayNumber;
    private int mInitialLcnNumber;

    private int mDisplayNumber;
    private Integer mDisplayNumber2 = null;

    private TvControlManager.ScanMode mScanMode = null;
    private TvControlManager.SortMode mSortMode = null;

    private ArrayList<TvControlManager.ScannerLcnInfo> mLcnInfo = null;

    /*for store in search*/
    private boolean isFinalStoreStage = false;
    private boolean isRealtimeStore = false;


    private ArrayList<ChannelInfo> mChannelsOld = null;
    private ArrayList<ChannelInfo> mChannelsNew = null;

    private boolean on_dtv_channel_store_tschanged = true;

    private int lcn_overflow_start;
    private int display_number_start;

    public TvStoreManager(Context context, String inputId, int initialDisplayNumber) {
        Log.d(TAG, "inputId["+inputId+"] initialDisplayNumber["+initialDisplayNumber+"]");

        mContext = context;
        mInputId = inputId;
        mInitialDisplayNumber = initialDisplayNumber;
        mInitialLcnNumber = mInitialDisplayNumber;

        mTvDataBaseManager = new TvDataBaseManager(mContext);

        display_number_start = mInitialDisplayNumber;
        lcn_overflow_start = mInitialLcnNumber;
    }

    public void setInitialLcnNumber(int initialLcnNumber) {
        Log.d(TAG, "initalLcnNumber["+initialLcnNumber+"]");
        mInitialLcnNumber = initialLcnNumber;
    }

    public void onEvent(String eventType, Bundle eventArgs) {}

    public void onUpdateCurrent(ChannelInfo channel, boolean store) {}

    public void onDtvNumberMode(String mode) {}

    public abstract void onScanEnd();

    public void onScanExit() {}


    private Bundle getScanEventBundle(TvControlManager.ScannerEvent mEvent) {
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TYPE, mEvent.type);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_PRECENT, mEvent.precent);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TOTALCOUNT, mEvent.totalcount);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_LOCK, mEvent.lock);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_CNUM, mEvent.cnum);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FREQ, mEvent.freq);
        bundle.putString(DroidLogicTvUtils.SIG_INFO_C_PROGRAMNAME, mEvent.programName);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SRVTYPE, mEvent.srvType);
        bundle.putString(DroidLogicTvUtils.SIG_INFO_C_MSG, mEvent.msg);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_STRENGTH, mEvent.strength);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_QUALITY, mEvent.quality);
        // ATV
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VIDEOSTD, mEvent.videoStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_AUDIOSTD, mEvent.audioStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_ISAUTOSTD, mEvent.isAutoStd);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FINETUNE, mEvent.fineTune);
        // DTV
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_MODE, mEvent.mode);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SR, mEvent.sr);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_MOD, mEvent.mod);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_BANDWIDTH, mEvent.bandwidth);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_OFM_MODE, mEvent.ofdm_mode);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_TS_ID, mEvent.ts_id);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_ORIG_NET_ID, mEvent.orig_net_id);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_SERVICEiD, mEvent.serviceID);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VID, mEvent.vid);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_VFMT, mEvent.vfmt);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AIDS, mEvent.aids);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AFMTS, mEvent.afmts);
        bundle.putStringArray(DroidLogicTvUtils.SIG_INFO_C_ALANGS, mEvent.alangs);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_ATYPES, mEvent.atypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_AEXTS, mEvent.aexts);
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_PCR, mEvent.pcr);

        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_STYPES, mEvent.stypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SIDS, mEvent.sids);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SSTYPES, mEvent.sstypes);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SID1S, mEvent.sid1s);
        bundle.putIntArray(DroidLogicTvUtils.SIG_INFO_C_SID2S, mEvent.sid2s);
        bundle.putStringArray(DroidLogicTvUtils.SIG_INFO_C_SLANGS, mEvent.slangs);

        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, -1);

        return bundle;
    }

    private Bundle getDisplayNumBunlde(int displayNum) {
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, displayNum);
        return bundle;
    }


    private void prepareStore(TvControlManager.ScannerEvent event) {
        mScanMode = new TvControlManager.ScanMode(event.scan_mode);
        mSortMode = new TvControlManager.SortMode(event.sort_mode);
        mDisplayNumber = mInitialDisplayNumber;
        mDisplayNumber2 = new Integer(mInitialDisplayNumber);
        isFinalStoreStage = false;
        isRealtimeStore = false;

        Bundle bundle = null;
        bundle = getScanEventBundle(event);
        onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_BEGIN_EVENT, bundle);
    }

    private void checkOrPatchBeginLost(TvControlManager.ScannerEvent event) {
        if (mScanMode == null) {
            Log.d(TAG, "!Lost EVENT_SCAN_BEGIN, assume began.");
            prepareStore(event);
        }
    }

    private void initChannelsExist() {
        //get all old channles exist.
        //init display number count.
        if (mChannelsOld == null) {
            mChannelsOld = mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO);
            mChannelsOld.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_AUDIO));
            //do not count service_other.
            mDisplayNumber = mChannelsOld.size() + 1;
            mChannelsOld.addAll(mTvDataBaseManager.getChannelList(mInputId, TvContract.Channels.SERVICE_TYPE_OTHER));
            Log.d(TAG, "Store> channel next:" + mDisplayNumber);
        }
    }

    private ChannelInfo createDtvChannelInfo(TvControlManager.ScannerEvent event) {
        String name = null;
        String serviceType;

        try {
            name = TVMultilingualText.getText(event.programName);
        } catch (Exception e) {
            e.printStackTrace();
            name = "????";
        }

        if (event.srvType == 1) {
            serviceType = TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO;
        } else if (event.srvType == 2) {
            serviceType = TvContract.Channels.SERVICE_TYPE_AUDIO;
        } else {
            serviceType = TvContract.Channels.SERVICE_TYPE_OTHER;
        }

        return new ChannelInfo.Builder()
               .setInputId(mInputId)
               .setType(new TvControlManager.TvMode(event.mode).toType())
               .setServiceType(serviceType)
               .setServiceId(event.serviceID)
               .setDisplayNumber(Integer.toString(mDisplayNumber))
               .setDisplayName(name)
               .setLogoUrl(null)
               .setOriginalNetworkId(event.orig_net_id)
               .setTransportStreamId(event.ts_id)
               .setVideoPid(event.vid)
               .setVideoStd(0)
               .setVfmt(event.vfmt)
               .setVideoWidth(0)
               .setVideoHeight(0)
               .setAudioPids(event.aids)
               .setAudioFormats(event.afmts)
               .setAudioLangs(event.alangs)
               .setAudioExts(event.aexts)
               .setAudioStd(0)
               .setIsAutoStd(event.isAutoStd)
               //.setAudioTrackIndex(getidxByDefLan(event.alangs))
               .setAudioCompensation(0)
               .setPcrPid(event.pcr)
               .setFrequency(event.freq)
               .setBandwidth(event.bandwidth)
               .setSymbolRate(event.sr)
               .setModulation(event.mod)
               .setFineTune(0)
               .setBrowsable(true)
               .setIsFavourite(false)
               .setPassthrough(false)
               .setLocked(false)
               .setSubtitleTypes(event.stypes)
               .setSubtitlePids(event.sids)
               .setSubtitleStypes(event.sstypes)
               .setSubtitleId1s(event.sid1s)
               .setSubtitleId2s(event.sid2s)
               .setSubtitleLangs(event.slangs)
               //.setSubtitleTrackIndex(getidxByDefLan(event.slangs))
               .setDisplayNameMulti(event.programName)
               .setFreeCa(event.free_ca)
               .setScrambled(event.scrambled)
               .setSdtVersion(event.sdtVersion)
               .build();
    }

    private ChannelInfo createAtvChannelInfo(TvControlManager.ScannerEvent event) {
        String ATVName = "ATV program";
        return new ChannelInfo.Builder()
               .setInputId(mInputId == null ? "NULL" : mInputId)
               .setType(new TvControlManager.TvMode(event.mode).toType())
               .setServiceType(TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO)//default is SERVICE_TYPE_AUDIO_VIDEO
               .setServiceId(0)
               .setDisplayNumber(Integer.toString(mDisplayNumber))
               .setDisplayName(ATVName+" "+Integer.toString(mDisplayNumber))
               .setLogoUrl(null)
               .setOriginalNetworkId(0)
               .setTransportStreamId(0)
               .setVideoPid(0)
               .setVideoStd(event.videoStd)
               .setVfmt(0)
               .setVideoWidth(0)
               .setVideoHeight(0)
               .setAudioPids(null)
               .setAudioFormats(null)
               .setAudioLangs(null)
               .setAudioExts(null)
               .setAudioStd(event.audioStd)
               .setIsAutoStd(event.isAutoStd)
               .setAudioTrackIndex(0)
               .setAudioCompensation(0)
               .setPcrPid(0)
               .setFrequency(event.freq)
               .setBandwidth(0)
               .setSymbolRate(0)
               .setModulation(0)
               .setFineTune(0)
               .setBrowsable(true)
               .setIsFavourite(false)
               .setPassthrough(false)
               .setLocked(false)
               .setDisplayNameMulti("xxx" + ATVName)
               .build();
    }

    private void cacheDTVChannel(TvControlManager.ScannerEvent event, ChannelInfo channel) {

        if (mScanMode == null) {
            Log.d(TAG, "mScanMode is null, store return.");
            return;
        }

        if (mChannelsNew == null)
            mChannelsNew = new ArrayList();

        mChannelsNew.add(channel);

        Log.d(TAG, "store save [" + channel.getNumber() + "][" + channel.getFrequency() + "][" + channel.getServiceType() + "][" + channel.getDisplayName() + "]");

        if (mScanMode.isDTVManulScan()) {
            if (on_dtv_channel_store_tschanged) {
                on_dtv_channel_store_tschanged = false;
                if (mChannelsOld != null) {
                    Log.d(TAG, "remove channels with freq!="+channel.getFrequency());
                    //remove channles with diff freq from old channles
                    Iterator<ChannelInfo> iter = mChannelsOld.iterator();
                    while (iter.hasNext()) {
                        ChannelInfo c = iter.next();
                        if (c.getFrequency() != channel.getFrequency())
                            iter.remove();
                    }
                }
            }
        }
    }


    private boolean isChannelInListbyId(ChannelInfo channel, ArrayList<ChannelInfo> list) {
        if (list == null)
            return false;

        for (ChannelInfo c : list)
            if (c.getId() == channel.getId())
                return true;

        return false;
    }

    private void updateChannelNumber(ChannelInfo channel) {
        updateChannelNumber(channel, null);
    }

    private void updateChannelNumber(ChannelInfo channel, ArrayList<ChannelInfo> channels) {
        boolean ignoreDBCheck = false;//mScanMode.isDTVAutoScan();
        int number = -1;
        ArrayList<ChannelInfo> chs = null;

        if (!ignoreDBCheck) {
            if (channels != null)
                chs = channels;
            else
                chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                        null, null);
            for (ChannelInfo c : chs) {
                if ((c.getNumber() >= display_number_start) && !isChannelInListbyId(c, mChannelsOld)) {
                    display_number_start = c.getNumber() + 1;
                }
            }
        }
        Log.d(TAG, "display number start from:" + display_number_start);

        Log.d(TAG, "Service["+channel.getOriginalNetworkId()+":"+channel.getTransportStreamId()+":"+channel.getServiceId()+"]");

        if (channel.getServiceType() == TvContract.Channels.SERVICE_TYPE_OTHER) {
            Log.d(TAG, "Service["+channel.getServiceId()+"] is Type OTHER, ignore NUMBER update and set to unbrowsable");
            channel.setBrowsable(false);
            return;
        }

        if (mChannelsOld != null) {//may only in manual search
            for (ChannelInfo c : mChannelsOld) {
                if ((c.getOriginalNetworkId() == channel.getOriginalNetworkId())
                    && (c.getTransportStreamId() == channel.getTransportStreamId())
                    && (c.getServiceId() == channel.getServiceId())) {
                    //same freq, reuse number if old channel is identical
                    Log.d(TAG, "found num:" + c.getDisplayNumber() + " by same old service["+c.getOriginalNetworkId()+":"+c.getTransportStreamId()+":"+c.getServiceId()+"]");
                    number = c.getNumber();
                }
            }
        }

        //service totally new
        if (number < 0)
            number = display_number_start++;

        Log.d(TAG, "update displayer number["+number+"]");

        channel.setDisplayNumber(String.valueOf(number));

        if (channels != null)
            channels.add(channel);
    }

    private void updateChannelLCN(ChannelInfo channel) {
        updateChannelLCN(channel, null);
    }

    private void updateChannelLCN(ChannelInfo channel, ArrayList<ChannelInfo> channels) {
        boolean ignoreDBCheck = false;//mScanMode.isDTVAutoScan();

        int lcn = -1;
        int lcn_1 = -1;
        int lcn_2 = -1;
        boolean visible = true;
        boolean swapped = false;

        ArrayList<ChannelInfo> chs = null;

        if (!ignoreDBCheck) {
            if (channels != null)
                chs = channels;
            else
                chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                        null, null);
            for (ChannelInfo c : chs) {
                if ((c.getLCN() >= lcn_overflow_start) && !isChannelInListbyId(c, mChannelsOld)) {
                    lcn_overflow_start = c.getLCN() + 1;
                }
            }
        }
        Log.d(TAG, "lcn overflow start from:"+lcn_overflow_start);

        Log.d(TAG, "Service["+channel.getOriginalNetworkId()+":"+channel.getTransportStreamId()+":"+channel.getServiceId()+"]");

        if (channel.getServiceType() == TvContract.Channels.SERVICE_TYPE_OTHER) {
            Log.d(TAG, "Service["+channel.getServiceId()+"] is Type OTHER, ignore LCN update and set to unbrowsable");
            channel.setBrowsable(false);
            return;
        }

        if (mLcnInfo != null) {
            for (TvControlManager.ScannerLcnInfo l : mLcnInfo) {
                if ((l.netId == channel.getOriginalNetworkId())
                    && (l.tsId == channel.getTransportStreamId())
                    && (l.serviceId == channel.getServiceId())) {

                    Log.d(TAG, "lcn found:");
                    Log.d(TAG, "\tlcn[0:"+l.lcn[0]+":"+l.visible[0]+":"+l.valid[0]+"]");
                    Log.d(TAG, "\tlcn[1:"+l.lcn[1]+":"+l.visible[1]+":"+l.valid[1]+"]");

                    // lcn found, use lcn[0] by default.
                    lcn_1 = l.valid[0] == 0 ? -1 : l.lcn[0];
                    lcn_2 = l.valid[1] == 0 ? -1 : l.lcn[1];
                    lcn = lcn_1;
                    visible = l.visible[0] == 0 ? false : true;

                    if ((lcn_1 != -1) && (lcn_2 != -1) && !ignoreDBCheck) {
                        // check for lcn already exist just on Maunual Scan
                        // look for service with sdlcn equal to l's hdlcn, if found, change the service's lcn to it's hdlcn
                        ChannelInfo ch = null;
                        if (channels != null) {
                            for (ChannelInfo c : channels) {
                                if (c.getLCN() == lcn_2)
                                    ch = c;
                            }
                        } else {
                            chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                                    ChannelInfo.COLUMN_LCN+"=?",
                                    new String[]{String.valueOf(lcn_2)});
                            if (chs.size() > 0) {
                                if (chs.size() > 1)
                                    Log.d(TAG, "Warning: found " + chs.size() + "channels with lcn="+lcn_2);
                                ch = chs.get(0);
                            }
                        }
                        if ((ch != null) && !isChannelInListbyId(ch, mChannelsOld)) {// do not check those will be deleted.
                            Log.d(TAG, "swap exist lcn["+ch.getLCN()+"] -> ["+ch.getLCN2()+"]");
                            Log.d(TAG, "\t for Service["+ch.getOriginalNetworkId()+":"+ch.getTransportStreamId()+":"+ch.getServiceId()+"]");

                            ch.setLCN(ch.getLCN2());
                            ch.setLCN1(ch.getLCN2());
                            ch.setLCN2(lcn_2);
                            if (channels == null)
                                mTvDataBaseManager.updateChannelInfo(ch);

                            swapped = true;
                        }
                    } else if (lcn_1 == -1) {
                        lcn = lcn_2;
                        visible = l.visible[1] == 0 ? false : true;
                        Log.d(TAG, "lcn[0] invalid, use lcn[1]");
                    }
                }
            }
        }

        Log.d(TAG, "Service visible = "+visible);
        channel.setBrowsable(visible);

        if (!swapped) {
            if (lcn >= 0) {
                ChannelInfo ch = null;
                if (channels != null) {
                    for (ChannelInfo c : channels) {
                        if (c.getLCN() == lcn)
                            ch = c;
                    }
                } else {
                    chs = mTvDataBaseManager.getChannelList(mInputId, ChannelInfo.COMMON_PROJECTION,
                            ChannelInfo.COLUMN_LCN+"=?",
                            new String[]{String.valueOf(lcn)});
                    if (chs.size() > 0) {
                        if (chs.size() > 1)
                            Log.d(TAG, "Warning: found " + chs.size() + "channels with lcn="+lcn);
                        ch = chs.get(0);
                    }
                }
                if (ch != null) {
                    if (!isChannelInListbyId(ch, mChannelsOld)) {//do not check those will be deleted.
                        Log.d(TAG, "found lcn conflct:" + lcn + " by service["+ch.getOriginalNetworkId()+":"+ch.getTransportStreamId()+":"+ch.getServiceId()+"]");
                        lcn = lcn_overflow_start++;
                    }
                }
            } else {
                Log.d(TAG, "no LCN info found for service");
                if (mChannelsOld != null) {//may only in manual search
                    for (ChannelInfo c : mChannelsOld) {
                        if ((c.getOriginalNetworkId() == channel.getOriginalNetworkId())
                            && (c.getTransportStreamId() == channel.getTransportStreamId())
                            && (c.getServiceId() == channel.getServiceId())) {
                            //same freq, reuse lcn if old channel is identical
                            Log.d(TAG, "found lcn:" + c.getLCN() + " by same old service["+c.getOriginalNetworkId()+":"+c.getTransportStreamId()+":"+c.getServiceId()+"]");
                            lcn = c.getLCN();
                        }
                    }
                }
                //service totally new
                if (lcn < 0)
                    lcn = lcn_overflow_start++;
            }
        }

        Log.d(TAG, "update LCN[0:"+lcn+" 1:"+lcn_1+" 2:"+lcn_2+"]");

        channel.setLCN(lcn);
        channel.setLCN1(lcn_1);
        channel.setLCN2(lcn_2);

        if (channels != null)
            channels.add(channel);
    }

    private void storeTvChannel(boolean isRealtimeStore, boolean isFinalStore) {
        Bundle bundle = null;

        Log.d(TAG, "isRealtimeStore:" + isRealtimeStore + " isFinalStore:"+ isFinalStore);

        if (mChannelsNew != null) {

            /*sort channels by serviceId*/
            Collections.sort(mChannelsNew, new Comparator<ChannelInfo> () {
                @Override
                public int compare(ChannelInfo a, ChannelInfo b) {
                    /*sort: frequency 1st, serviceId 2nd*/
                    int A = a.getFrequency();
                    int B = b.getFrequency();
                    return (A > B) ? 1 : ((A == B) ? (a.getServiceId() - b.getServiceId()) : -1);
                }
            });

            ArrayList<ChannelInfo> mChannels = new ArrayList();

            for (ChannelInfo c : mChannelsNew) {

                //Calc display number / LCN
                if (mSortMode.isLCNSort()) {
                    if (isRealtimeStore)
                        updateChannelLCN(c, mChannels);
                    else
                        updateChannelLCN(c);
                    c.setDisplayNumber(String.valueOf(c.getLCN()));
                    Log.d(TAG, "LCN DisplayNumber:"+ c.getDisplayNumber());
                    onDtvNumberMode("lcn");
                } else {
                    if (isRealtimeStore)
                        updateChannelNumber(c, mChannels);
                    else
                        updateChannelNumber(c);
                    Log.d(TAG, "NUM DisplayNumber:"+ c.getDisplayNumber());
                }

                if (isRealtimeStore)
                    mTvDataBaseManager.updateOrinsertDtvChannelWithNumber(c);
                else
                    mTvDataBaseManager.insertDtvChannel(c, c.getNumber());

                Log.d(TAG, ((isRealtimeStore) ? "update/insert [" : "insert [") + c.getNumber()
                    + "][" + c.getFrequency() + "][" + c.getServiceType() + "][" + c.getDisplayName() + "]");

                if (isFinalStore) {
                    bundle = getDisplayNumBunlde(c.getNumber());
                    onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                }
            }
        }

        if (mScanMode != null) {
            if (mScanMode.isDTVManulScan()) {
                if (mChannelsOld != null) {
                    mTvDataBaseManager.deleteChannels(mChannelsOld);
                    for (ChannelInfo c : mChannelsOld)
                        Log.d(TAG, "rm ch[" + c.getNumber() + "][" + c.getDisplayName() + "][" + c.getFrequency() + "]");
                }
            }
        }

        lcn_overflow_start = mInitialLcnNumber;
        display_number_start = mInitialDisplayNumber;
        on_dtv_channel_store_tschanged = true;
        mChannelsOld = null;
        mChannelsNew = null;
    }

    public void onStoreEvent(TvControlManager.ScannerEvent event) {
        ChannelInfo channel = null;
        String name = null;
        Bundle bundle = null;

        Log.d(TAG, "onEvent:" + event.type + " :" + mDisplayNumber);

        switch (event.type) {
        case TvControlManager.EVENT_SCAN_BEGIN:
            Log.d(TAG, "Scan begin");
            prepareStore(event);
            break;

        case TvControlManager.EVENT_LCN_INFO_DATA:

            checkOrPatchBeginLost(event);

            if (mLcnInfo == null)
                mLcnInfo = new ArrayList<TvControlManager.ScannerLcnInfo>();
            mLcnInfo.add(event.lcnInfo);
            Log.d(TAG, "Lcn["+event.lcnInfo.netId+":"+event.lcnInfo.tsId+":"+event.lcnInfo.serviceId+"]");
            Log.d(TAG, "\t[0:"+event.lcnInfo.lcn[0]+":"+event.lcnInfo.visible[0]+":"+event.lcnInfo.valid[0]+"]");
            Log.d(TAG, "\t[1:"+event.lcnInfo.lcn[1]+":"+event.lcnInfo.visible[1]+":"+event.lcnInfo.valid[1]+"]");
            break;

        case TvControlManager.EVENT_DTV_PROG_DATA:
            Log.d(TAG, "dtv prog data");

            checkOrPatchBeginLost(event);

            if (!isFinalStoreStage)
                isRealtimeStore = true;

            if (mScanMode == null) {
                Log.d(TAG, "mScanMode is null, store return.");
                return;
            }

            if (mScanMode.isDTVManulScan())
                initChannelsExist();

            channel = createDtvChannelInfo(event);

            if (mDisplayNumber2 != null)
                channel.setDisplayNumber(Integer.toString(mDisplayNumber2));
            else
                channel.setDisplayNumber(String.valueOf(mDisplayNumber));

            Log.d(TAG, "reset number to " + channel.getDisplayNumber());

            channel.print();
            cacheDTVChannel(event, channel);

            if (mDisplayNumber2 != null) {
                Log.d(TAG, "mid store, num:"+mDisplayNumber2);
                mDisplayNumber2++;//count for realtime stage
            } else {
                Log.d(TAG, "final store, num: " + mDisplayNumber);
                bundle = getDisplayNumBunlde(mDisplayNumber);
                onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                mDisplayNumber++;//count for store stage
            }
            break;

        case TvControlManager.EVENT_ATV_PROG_DATA:
            Log.d(TAG, "atv prog data");

            checkOrPatchBeginLost(event);

            if (!isFinalStoreStage)
                isRealtimeStore = true;

            initChannelsExist();

            channel = createAtvChannelInfo(event);
            channel.print();

            if (mScanMode.isATVManualScan())
                onUpdateCurrent(channel, true);
            else
                mTvDataBaseManager.updateOrinsertAtvChannelWithNumber(channel);

            Log.d(TAG, "onEvent,displayNum:" + mDisplayNumber);

            if (isFinalStoreStage) {
                bundle = getDisplayNumBunlde(mDisplayNumber);
                onEvent(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM_EVENT, bundle);
                mDisplayNumber++;//count for storestage
            }
            break;

        case TvControlManager.EVENT_SCAN_PROGRESS:
            Log.d(TAG, event.precent + "%\tfreq[" + event.freq + "] lock[" + event.lock + "] strength[" + event.strength + "] quality[" + event.quality + "]");

            checkOrPatchBeginLost(event);

            //take evt:progress as a store-loop end.
            if (!isFinalStoreStage
                && (event.mode != TVChannelParams.MODE_ANALOG)
                && !mScanMode.isDTVManulScan()) {
                storeTvChannel(isRealtimeStore, isFinalStoreStage);
                mDisplayNumber2 = mInitialDisplayNumber;//dtv pop all channels scanned every store-loop
            }

            bundle = getScanEventBundle(event);
            if ((event.mode == TVChannelParams.MODE_ANALOG) && (event.lock == 0x11)) {
                bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_DISPLAYNUM, mDisplayNumber);
                mDisplayNumber++;//count for progress stage
            }
            onEvent(DroidLogicTvUtils.SIG_INFO_C_PROCESS_EVENT, bundle);
            break;

        case TvControlManager.EVENT_STORE_BEGIN:
            Log.d(TAG, "Store begin");

            //reset for store stage
            isFinalStoreStage = true;
            mDisplayNumber = mInitialDisplayNumber;
            mDisplayNumber2 = null;
            if (mLcnInfo != null)
                mLcnInfo.clear();

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_STORE_BEGIN_EVENT, bundle);
            break;

        case TvControlManager.EVENT_STORE_END:
            Log.d(TAG, "Store end");

            storeTvChannel(isRealtimeStore, isFinalStoreStage);

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_STORE_END_EVENT, bundle);
            break;

        case TvControlManager.EVENT_SCAN_END:
            Log.d(TAG, "Scan end");

            onScanEnd();

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_END_EVENT, bundle);
            break;

        case TvControlManager.EVENT_SCAN_EXIT:
            Log.d(TAG, "Scan exit.");

            isFinalStoreStage = false;
            isRealtimeStore = false;
            mDisplayNumber = mInitialDisplayNumber;
            mDisplayNumber2 = null;
            if (mLcnInfo != null) {
                mLcnInfo.clear();
                mLcnInfo = null;
            }

            mScanMode = null;

            onScanExit();

            bundle = getScanEventBundle(event);
            onEvent(DroidLogicTvUtils.SIG_INFO_C_SCAN_EXIT_EVENT, bundle);
            break;

        default:
            break;
        }
    }

}


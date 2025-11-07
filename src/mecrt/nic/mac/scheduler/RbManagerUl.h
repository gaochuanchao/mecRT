//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    RbManagerUl.cc / RbManagerUl.h
//
//  Description:
//    This file implements the resource block manager for the uplink of NR in the MEC.
//    Currently only frequency division resource allocation is supported.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_RB_MANAGER_UL_H_
#define _MECRT_RB_MANAGER_UL_H_

#include "mecrt/nic/mac/GnbMac.h"

using namespace std;

struct AppGrantInfo
{
    AppId appId;
    int numGrantedBands;
    set<Band> tempBands; // the temporary granted bands for the app, used for band adjustment
    set<Band> grantedBandSet; // the granted bands for the app
    omnetpp::simtime_t maxOffloadTime;
    MacNodeId ueId;
    int inputSize;
    int outputSize;
    int processGnbPort;  // the port of the processing gNB
    MacNodeId processGnbId;  // the id of processing gNB
    MacNodeId offloadGnbId;  // the id of offloading gNB
    Ipv4Address processGnbAddr;  // the address of processing gNB
    Ipv4Address ueAddr;  // the address of the vehicle
};

class RbManagerUl
{
  protected:
    GnbMac* mac_;
    Binder* binder_;
    /// Lte AMC module
    LteAmc *amc_;

    Direction dir_;

    /***
     * Carrier Meta information, currently only one carrier is supported
     */
    unsigned int rbPerBand_;  // number of resource blocks per band
    int numBands_;    // number of bands in the carrier
    double frequency_;
    int numerology_;  // the numerology of the carrier
    int ttiPerMs_;  // the number of TTI per millisecond
    /***
     * during data transmission, there are multiple headers added to the data. In total, 33 bytes are added
     *      - UDP header (8B)
     *      - IP header (20B)
     *      - PdcpPdu header (1B)
     *      - RlcSdu header (2B) : RLC_HEADER_UM
     *      - MacPdu header (2B) : MAC_HEADER
     */
    int dataAddOn_;  // the additional data size for each app data packet


    /***
     * Resource allocation record for each app
     */
    set<AppId> scheduledApp_;  // app that has been scheduled by the scheduler
    set<AppId> pausedApp_;  // app that has been paused due to temporary bad channel quality
    set<AppId> appToBeInitialized_; // app that has not been initialized yet (failed when receiving the grant)
    map<MacNodeId, int> vehDataRate_;  // the data rate for each UE per band per TTI
    map<AppId, map<Band, unsigned int>> appGrantedRbMap_;  // the resource block map for each app
    map<AppId, map<Band, unsigned int>> appTempRbMap_;  // the resource block map for each app
    set<Band> flexibleBands_;  // the flexible bands, free bands other than granted bands
    map<AppId, AppGrantInfo> appGrantInfos_;  // the granted service for each app, used for scheduling

  public:
    RbManagerUl(GnbMac* mac, LteAmc *amc)
    {
        mac_ = mac;
        binder_ = getBinder();
        amc_ = amc;
        dir_ = UL;

        // initialize some parameters, these parameters should be set by calling the setter functions
        rbPerBand_ = 1;
        numBands_ = 1;
        frequency_ = 2;
        numerology_ = 0;
        ttiPerMs_ = 1;
        dataAddOn_ = 33;    // UDP header (8B) + IP header (20B) + PdcpPdu header (1B) + RlcSdu header (2B) + MacPdu header (2B)
    }

    virtual ~RbManagerUl() {};

    // schedule the granted app
    virtual bool scheduleGrantedApp(AppId appId);

    // schedule active (scheduled) app
    virtual bool scheduleActiveApp(AppId appId);

    // schedule paused app
    virtual bool schedulePausedApp(AppId appId);

    // check if the granted bands is enough for the app when the data rate is changed
    virtual bool isGrantEnough(AppId appId);
    virtual int getMinimumRequiredBands(AppId appId);

    // init band status for available bands and reserved bands
    virtual void initBandStatus();

    // release the temporary granted bands for the app
    virtual void releaseTempBands(AppId appId);

    // read the app resource block occupation status
    virtual void readAppRbOccupation(const AppId appId, std::map<Band, unsigned int>& rbMap);

    // terminate the service for the app
    virtual void terminateAppService(AppId appId);

    // reset the resource allocation status
    virtual void resetRbStatus();


    // ================================
    // setter and getter functions
    // ================================
    void setRbPerBand(unsigned int rbPerBand) { rbPerBand_ = rbPerBand; }
    unsigned int getRbPerBand() { return rbPerBand_; }

    void setFrequency(double frequency) { frequency_ = frequency; }
    double getFrequency() { return frequency_; }

    int getNumerology() { return numerology_; }
    void setNumerology(int numerology) 
    { 
        numerology_ = numerology; 
        ttiPerMs_ = pow(2, numerology);
    }    

    void setNumBands(int numBands) { numBands_ = numBands; }
    int getNumBands() { return numBands_; }

    void setVehDataRate(MacNodeId ueId, int dataRate) { vehDataRate_[ueId] = dataRate;}
    int getVehDataRate(MacNodeId ueId) { return vehDataRate_[ueId]; }

    void addScheduledApp(AppId appId) { scheduledApp_.insert(appId); }
    void removeScheduledApp(AppId appId) { scheduledApp_.erase(appId); }
    set<AppId> getScheduledApp() { return scheduledApp_; }

    void addPausedApp(AppId appId) { pausedApp_.insert(appId); }
    void removePausedApp(AppId appId) { pausedApp_.erase(appId); }
    set<AppId> getPausedApp() { return pausedApp_; }

    void addAppToBeInitialized(AppId appId) { appToBeInitialized_.insert(appId); }
    void removeAppToBeInitialized(AppId appId) { appToBeInitialized_.erase(appId); }
    set<AppId> getAppToBeInitialized() { return appToBeInitialized_; }

    void setAppGrantInfo(AppId appId, AppGrantInfo appGrantInfo) { appGrantInfos_[appId] = appGrantInfo; }
    AppGrantInfo& getAppGrantInfo(AppId appId) { return appGrantInfos_[appId]; }
    void removeAppGrantInfo(AppId appId) { appGrantInfos_.erase(appId); }
    bool hasAppGrantInfo(AppId appId) { return appGrantInfos_.find(appId) != appGrantInfos_.end(); }

    int getAvailableBands() { return flexibleBands_.size(); }
    int getAppAllocatedBands(AppId appId) { return appGrantInfos_[appId].grantedBandSet.size() + appGrantInfos_[appId].tempBands.size(); }

};

#endif  


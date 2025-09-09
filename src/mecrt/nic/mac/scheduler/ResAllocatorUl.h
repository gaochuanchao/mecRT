//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//

#ifndef _MECRT_GNB_RESALLOCATOR_UL_H_
#define _MECRT_GNB_RESALLOCATOR_UL_H_

#include "mecrt/nic/mac/GnbMac.h"

using namespace std;

enum BandAdjustStatus
{
    NO_ADJUSTMENT = 0,
    ADJUSTED = 1,
    STOP_SERVICE = 2
};

// struct CarrierMeta
// {
//     double carrierFreq = 2; // carrier frequency
//     int numBands;    // number of bands in the carrier
//     int threshold;   // the threshold for band allocation by the scheduler, band < threshold is available
//     double thresholdRatio;
//     // vector<bool> bandStatus = vector<bool>();
//     vector<MacNodeId> allocatStatus;
//     set<Band> reservedBand = set<Band>();    // number of bands for reservation
//     set<Band> allocatedBands = set<Band>(); //  allocated bands for scheduling algorithm
//     set<Band> availBands = set<Band>(); //  remaining free bands for scheduling algorithm
//     map<MacNodeId, int> vehDataRate;  // the data rate for each UE per band per TTI
// };

struct AppGrant
{
	AppId appId;
	int grantedBands;
	omnetpp::simtime_t maxOffloadTime;
	MacNodeId ueId;
	int inputSize;
	int outputSize;
    int serverPort;
};

class ResAllocatorUl
{
  protected:

    /***
     * Carrier Meta information, currently only one carrier is supported
     */
    unsigned int rbPerBand_;  // number of resource blocks per band
    int numBands_;    // number of bands in the carrier
    int threshold_;   // the threshold for band allocation by the scheduler, band < threshold is available
    double thresholdRatio_;
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

    set<Band> reservedBand_ = set<Band>();    // number of bands for reservation
    set<Band> availBands_ = set<Band>(); //  remaining free bands for scheduling algorithm
    map<MacNodeId, int> vehDataRate_;  // the data rate for each UE per band per TTI
    map<MacNodeId, int> lastVehDataRate_;  // the data rate for each UE per band per TTI in last feebdack

    /***
     * Resource allocation record for each app
     */
    map<AppId, std::map<Band, unsigned int>> appRbMap_;  // the resource block map for each app
    map<AppId, int> appAvailBands_;  // the number of bands (in available bands) allocated to the app, exposed to the scheduler
    map<AppId, set<Band>> appBands_;  // the allocated bands for each app (all bands), exposed to vehicle
	map<AppId, AppGrant> appGrants_;  // the granted service for each app
    set<AppId> scheduledApp_;  // app that has been scheduled by the scheduler
    set<AppId> pausedApp_;  // app that has been paused due to temporary bad channel quality

    map<AppId, omnetpp::simtime_t> appUploadTime_; // the time of the last app data is uploading

    GnbMac* mac_;
    Binder* binder_;
    /// Lte AMC module
    LteAmc *amc_;

    Direction dir_;

  public:

    ResAllocatorUl(GnbMac* mac, LteAmc *amc)
    {
        mac_ = mac;
        binder_ = getBinder();
        amc_ = amc;
        dir_ = UL;
        dataAddOn_ = 33;    // UDP header (8B) + IP header (20B) + PdcpPdu header (1B) + RlcSdu header (2B) + MacPdu header (2B)

        // initialize some parameters, these parameters should be set by calling the setter functions
        rbPerBand_ = 1;
        numBands_ = 1;
        threshold_ = 1;
        thresholdRatio_ = 1;
        frequency_ = 2;
        numerology_ = 0;
        ttiPerMs_ = 1;
    }

    virtual ~ResAllocatorUl() {};

    // check the status of scheduled app (if band allocation needs to adjusted)
    virtual BandAdjustStatus checkScheduledApp(AppId appId);

    // check the status of scheduled app (if band allocation needs to adjusted)
    virtual BandAdjustStatus checkPausedApp(AppId appId);

    // check the status of pending app (if remaining bands are enough for scheduling)
    virtual bool schedulePendingApp(AppId appId);

	// adjust band allocation for the given app (if the maximum offloading time cannot be met)
	// virtual bool adjustBandAllocation(AppId appId, set<Band>& candidateBands);

    // compute the transmission parameters and data rate (byte per band per TTI)
    // virtual int getBandRatePerTTI(AppId appId, set<Band>& candidateBands);

    // allocate suitable bands to the already scheduled app
    virtual void allocateBands(AppId appId, int numBand);

    // find the best few bands for the app
    // virtual set<Band> findBestBands(AppId appId, int numBand);

    // update the ue resource block map
    virtual void updateAppRbMap(AppId appId);
    virtual void readAppRbOccupation(const AppId appId, std::map<Band, unsigned int>& rbMap);

    // terminate the service for the app
    virtual void terminateService(AppId appId);

    // release the allocated bands for the app
    virtual void releaseAllocatedBands(AppId appId);

    // init band status for available bands and reserved bands
    virtual void initBandStatus();


    // ================================
    // setter and getter functions
    // ================================

    // get the number of remaining available bands for the gNB
    virtual int getNumAvailableBands() { return availBands_.size(); }
    virtual set<Band>& getAvailableBands() { return availBands_; }

    // get the number bands allocated for the app
    virtual int getAppAvailAssignedBands(AppId appId) { return appAvailBands_[appId]; }
    virtual set<Band>& getAppAllocatedBands(AppId appId) { return appBands_[appId]; }

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

    void setThreshold(int threshold) { threshold_ = threshold; }
    int getThreshold() { return threshold_; }

    void setThresholdRatio(double thresholdRatio) { thresholdRatio_ = thresholdRatio; }
    double getThresholdRatio() { return thresholdRatio_; }

    void setNumBands(int numBands) { numBands_ = numBands; }
    int getNumBands() { return numBands_; }

    void setVehDataRate(MacNodeId ueId, int dataRate) { vehDataRate_[ueId] = dataRate;}
    int getVehDataRate(MacNodeId ueId) { return vehDataRate_[ueId]; }

    void updateLastVehDataRate(MacNodeId ueId) { lastVehDataRate_[ueId] = vehDataRate_[ueId];}
    int getLastVehDataRate(MacNodeId ueId) { return lastVehDataRate_[ueId]; }

    void setAppGrant(AppId appId, AppGrant appGrant) { appGrants_[appId] = appGrant; }
    void removeAppGrant(AppId appId) { appGrants_.erase(appId); }
	AppGrant& getAppGrant(AppId appId) { return appGrants_[appId]; }

    void addScheduledApp(AppId appId) { scheduledApp_.insert(appId); }
    void removeScheduledApp(AppId appId) { scheduledApp_.erase(appId); }
    set<AppId> getScheduledApp() { return scheduledApp_; }

    void addPausedApp(AppId appId) { pausedApp_.insert(appId); }
    void removePausedApp(AppId appId) { pausedApp_.erase(appId); }
    set<AppId> getPausedApp() { return pausedApp_; }

    void setAppUploadTime(AppId appId, omnetpp::simtime_t uploadTime) { appUploadTime_[appId] = uploadTime; }
    omnetpp::simtime_t getAppUploadTime(AppId appId) { return appUploadTime_[appId]; }

    // void setCarrierMeta(CarrierMeta carrierMeta, double frequency = 2) { carrierStatus_[frequency] = carrierMeta; }
    // CarrierMeta& getCarrierMeta(double frequency = 2) 
    // {
    //     if (carrierStatus_.find(frequency) == carrierStatus_.end())
    //     {
    //         CarrierMeta carrierMeta;
    //         carrierMeta.carrierFreq = frequency;
    //         carrierStatus_[frequency] = carrierMeta;
    //     }  
    //     return carrierStatus_[frequency];
    // }

    // void setAppBand(AppId appId, set<Band> bands) { appBands_[appId] = bands; }
    // set<Band>& getAppBand(AppId appId) { return appBands_[appId]; }

    // int getAppDataRate(AppId appId) { return appDataRate_[appId]; }
};

#endif

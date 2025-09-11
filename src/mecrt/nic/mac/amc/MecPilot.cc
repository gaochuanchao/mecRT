//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecPilot.cc / MecPilot.h
//
//  Description:
//    This file implements the Modulation and Coding Scheme (MCS) selection strategy.
//    Original file: simu5g - "stack/mac/amc/AmcPilot.h"
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/nic/mac/amc/MecPilot.h"

using namespace inet;

const UserTxParams& MecPilot::computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    EV << NOW << " MecPilot::computeTxParams for UE " << id << ", direction " << dirToA(dir) << endl;

    // Check if user transmission parameters have been already allocated
    if(amc_->existTxParams(id, dir, carrierFrequency))
    {
        EV << NOW << " MecPilot::computeTxParams The Information for this user have been already assigned \n";
        return amc_->getTxParams(id, dir, carrierFrequency);
    }

    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */
    const LteSummaryFeedback& sfb = amc_->getFeedback(id, MACRO, txMode, dir, carrierFrequency);

    if (TxMode(txMode)==MULTI_USER) // Initialize MuMiMoMatrix
    amc_->muMimoMatrixInit(dir,id);


    sfb.print(0,id,dir,txMode,"MecPilot::computeTxParams");

    // get a vector of  CQI over first CW
    std::vector<Cqi> summaryCqi = sfb.getCqi(0);

    // get the usable bands for this user
    UsableBands* usableB = nullptr;
    getUsableBands(id, usableB);

    Band chosenBand = 0;
    double chosenCqi = 0;
    // double max = 0;
    BandSet bandSet;

    /// TODO collapse the following part into a single part (e.g. do not fork on mode_ or usableBandsList_ size)

    // check which CQI computation policy is to be used
    if(mode_ == MAX_CQI)
    {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == nullptr || usableB->empty())
        {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MAX
            for(Band b = 1; b < bands; ++b)
            {
                // For all Bands
                double s = (double)summaryCqi.at(b);
                if(chosenCqi < s)
                {
                    chosenBand = b;
                    chosenCqi = s;
                }
            }

            for(Band b = 0; b < bands; ++b)
            {
                if (summaryCqi.at(b) >= chosenCqi)
                {
                    Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                    bandSet.insert(cellWiseBand);
                }
            }

            EV << NOW <<" MecPilot::computeTxParams - no UsableBand available for this user." << endl;
        }
        else
        {
            // TODO Add MIN and MEAN cqi computation methods
            unsigned int bandIt = 0;
            unsigned short currBand = (*usableB)[bandIt];
            chosenBand = currBand;
            chosenCqi = summaryCqi.at(currBand);
            for(; bandIt < usableB->size(); ++bandIt)
            {
                currBand = (*usableB)[bandIt];
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if(chosenCqi < s)
                {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
            }

            for(bandIt = 0; bandIt < usableB->size(); ++bandIt)
            {
                currBand = (*usableB)[bandIt];
                if (summaryCqi.at(currBand) >= chosenCqi)
                {
                    Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, currBand);
                    bandSet.insert(cellWiseBand);
                }
            }

            EV << NOW <<" MecPilot::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }
    else if(mode_ == MIN_CQI)
    {
        // if there are no usable bands, compute the final CQI through all the bands
        if (usableB == nullptr || usableB->empty())
        {
            chosenBand = 0;
            chosenCqi = summaryCqi.at(chosenBand);
            unsigned int bands = summaryCqi.size();// number of bands
            // computing MIN
            for(Band b = 0; b < bands; ++b)
            {
                // For all LBs
                double s = (double)summaryCqi.at(b);
                if(chosenCqi > s)
                {
                    chosenBand = b;
                    chosenCqi = s;
                }
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                bandSet.insert(cellWiseBand);
            }
            EV << NOW <<" MecPilot::computeTxParams - no UsableBand available for this user." << endl;
        }
        else
        {
            unsigned int bandIt = 0;
            unsigned short currBand = (*usableB)[bandIt];
            chosenBand = currBand;
            chosenCqi = summaryCqi.at(currBand);
            for(; bandIt < usableB->size(); ++bandIt)
            {
                currBand = (*usableB)[bandIt];
                // For all available band
                double s = (double)summaryCqi.at(currBand);
                if(chosenCqi > s)
                {
                    chosenBand = currBand;
                    chosenCqi = s;
                }
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, currBand);
                bandSet.insert(cellWiseBand);
            }

            EV << NOW <<" MecPilot::computeTxParams - UsableBand of size " << usableB->size() << " available for this user" << endl;
        }
    }
    else if(mode_ == ROBUST_CQI)
    {
        int target = 0;
        // int s;
        unsigned int bands = summaryCqi.size();// number of bands

        EV << "MecPilot::computeTxParams - computing ROBUST CQI" << endl;

        // // computing MIN
        // for(Band b = 0; b < bands; ++b)
        // {
        //     // For all LBs
        //     s = summaryCqi.at(b);
        //     target += s;
        // }
        // target = target/bands;

        // EV << "\t target value[" << target << "]" << endl;

        // sort summaryCqi vector in descending order
        std::sort(summaryCqi.begin(),summaryCqi.end(),std::greater<int>());
        int quartilePoint = bands/4;
        target = summaryCqi[quartilePoint];

        EV << "\t target value[" << target << "]" << endl;

        for(Band b = 0; b < bands; ++b)
        {
            if( summaryCqi.at(b) >= target )
            {
                EV << b << ")" << summaryCqi.at(b) << "yes" << endl;
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, b);
                bandSet.insert(cellWiseBand);
            }
            else
                EV << b << ")" << summaryCqi.at(b) << "no" << endl;
        }
        chosenBand = 0;
        chosenCqi = target;
    }
    else if (mode_ == AVG_CQI)
    {
        // MEAN cqi computation method
        chosenCqi = getBinder()->meanCqi(sfb.getCqi(0),id,dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i)
        {
            if (sfb.getCqi(0).at(i) >= chosenCqi)
            {
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, i);
                bandSet.insert(cellWiseBand);
            }
        }
        chosenBand = 0;
    }
    else if (mode_ == MEDIAN_CQI)
    {
        // MEAN cqi computation method
        chosenCqi = getBinder()->medianCqi(sfb.getCqi(0),id,dir);
        for (Band i = 0; i < sfb.getCqi(0).size(); ++i)
        {
            if (sfb.getCqi(0).at(i) >= chosenCqi)
            {
                Band cellWiseBand = amc_->getCellInfo()->getCellwiseBand(carrierFrequency, i);
                bandSet.insert(cellWiseBand);
            }
        }
        chosenBand = 0;
    }

    // Set user transmission parameters only for the best band
    UserTxParams info;
    info.writeTxMode(txMode);
    info.writeRank(sfb.getRi());
    info.writeCqi(std::vector<Cqi>(1, chosenCqi));
    info.writePmi(sfb.getPmi(chosenBand));
    info.writeBands(bandSet);
    RemoteSet antennas;
    antennas.insert(MACRO);
    info.writeAntennas(antennas);

    // DEBUG
    EV << NOW << " MecPilot::computeTxParams NEW values assigned! - CQI =" << chosenCqi << "\n";
    info.print("MecPilot::computeTxParams");

    return amc_->setTxParams(id, dir, info, carrierFrequency);
}

std::vector<Cqi> MecPilot::getMultiBandCqi(MacNodeId id , const Direction dir, double carrierFrequency)
{
    EV << NOW << " MecPilot::getMultiBandCqi for UE " << id << ", direction " << dirToA(dir) << endl;

    // TODO make it configurable from NED
    // default transmission mode
    TxMode txMode = TRANSMIT_DIVERSITY;

    /**
     *  Select the band which has the best summary
     *  Note: this pilot is not DAS aware, so only MACRO antenna
     *  is used.
     */
    const LteSummaryFeedback& sfb = amc_->getFeedback(id, MACRO, txMode, dir, carrierFrequency);

    // get a vector of  CQI over first CW
    return sfb.getCqi(0);
}

void MecPilot::setUsableBands(MacNodeId id , UsableBands usableBands)
{
    EV << NOW << " MecPilot::setUsableBands - setting Usable bands: for node " << id<< " [" ;
    for(unsigned int i = 0 ; i<usableBands.size() ; ++i)
    {
        EV << usableBands[i] << ",";
    }
    EV << "]"<<endl;
    UsableBandsList::iterator it = usableBandsList_.find(id);

    // if usable bands for this node are already setm delete it (probably unnecessary)
    if(it!=usableBandsList_.end())
        usableBandsList_.erase(id);
    usableBandsList_.insert(std::pair<MacNodeId,UsableBands>(id,usableBands));
}

bool MecPilot::getUsableBands(MacNodeId id, UsableBands*& uBands)
{
    EV << NOW << " MecPilot::getUsableBands - getting Usable bands for node " << id;

    bool found = false;
    UsableBandsList::iterator it = usableBandsList_.find(id);
    if(it!=usableBandsList_.end())
    {
        found = true;
    }
    else
    {
        // usable bands for this id not found
        if (getNodeTypeById(id) == UE)
        {
            // if it is a UE, look for its serving cell
            MacNodeId cellId = getBinder()->getNextHop(id);
            it = usableBandsList_.find(cellId);
            if(it!=usableBandsList_.end())
                found = true;
        }
    }

    if (found)
    {
        uBands = &(it->second);
        EV << " [" ;
        for(unsigned int i = 0 ; i < it->second.size() ; ++i)
        {
            EV << it->second[i] << ",";
        }
        EV << "]"<<endl;

        return true;
    }

    EV << " [All bands are usable]" << endl ;
    uBands = nullptr;
    return false;
}

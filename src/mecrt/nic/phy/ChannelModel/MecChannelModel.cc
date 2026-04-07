// 
//  mecRT: A Mobile Edge Computing Simulation Framework
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 
#include "mecrt/nic/phy/ChannelModel/MecChannelModel.h"
#include "stack/phy/layer/LtePhyUe.h"

Define_Module(MecChannelModel);


double MecChannelModel::getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl)
{
   double movement = .0;
   double speed = .0;

   //COMPUTE 3D and 2D DISTANCE between ue and eNodeB
   double threeDimDistance = phy_->getCoord().distance(coord);
   double twoDimDistance = getTwoDimDistance(phy_->getCoord(), coord);

   if (dir == DL) // sender is UE
       speed = computeSpeed(nodeId, phy_->getCoord());
   else
       speed = computeSpeed(nodeId, coord);

   //If traveled distance is greater than correlation distance UE could have changed its state and
   // its visibility from eNodeb, hence it is correct to recompute the los probability
   if (movement > correlationDistance_
           || losMap_.find(nodeId) == losMap_.end())
   {
       computeLosProbability(twoDimDistance, nodeId);
   }

   if(dir == DL)
       emit(distance_,twoDimDistance);

   // Use the normal NR path-loss model only inside its valid 2D-distance range.
   bool los = losMap_[nodeId];
   double attenuation = MEC_OUT_OF_RANGE_ATTENUATION;
   if (twoDimDistance <= MEC_MAX_VALID_2D_DISTANCE)
   {
       attenuation = computePathLoss(threeDimDistance, twoDimDistance, los);

       // Apply shadowing only for links handled by the normal path-loss model.
       if (nodeId < BGUE_MIN_ID && shadowing_)
           attenuation += computeShadowing(twoDimDistance, nodeId, speed, cqiDl);
   }

   // update current user position

   //if sender is a eNodeB
   if (dir == DL)
       //store the position of user
       updatePositionHistory(nodeId, phy_->getCoord());
   else
       //sender is an UE
       updatePositionHistory(nodeId, coord);

   EV << "MecChannelModel::getAttenuation - computed attenuation at distance " << threeDimDistance << " for eNb is " << attenuation << endl;

   return attenuation;
}

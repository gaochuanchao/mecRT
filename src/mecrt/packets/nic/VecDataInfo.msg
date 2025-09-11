//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    VecDataInfo.msg
//
//  Description:
//    This file implements message tag VecDataInfo.
//    Used by the UE MAC layer.
//	  This tag is used to tell the UE physical layer how long the current data transmission will last.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
import inet.common.INETDefs;
import inet.common.TagBase;

class VecDataInfo extends inet::TagBase
{
    double duration;
}

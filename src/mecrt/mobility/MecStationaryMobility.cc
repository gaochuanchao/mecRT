//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecStationaryMobility.cc / MecStationaryMobility.h
//
//  Description:
//    This file implements the stationary mobility functionality in the MEC system.
//    The MecStationaryMobility module provides static position for ES (RSU).
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/mobility/MecStationaryMobility.h"
#include "mecrt/common/Database.h"

Define_Module(MecStationaryMobility);

void MecStationaryMobility::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "MecStationaryMobility::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        constraintAreaMin.x = par("constraintAreaMinX");
        constraintAreaMin.y = par("constraintAreaMinY");
        constraintAreaMin.z = par("constraintAreaMinZ");
        constraintAreaMax.x = par("constraintAreaMaxX");
        constraintAreaMax.y = par("constraintAreaMaxY");
        constraintAreaMax.z = par("constraintAreaMaxZ");
        format.parseFormat(par("displayStringTextFormat"));
        subjectModule = findSubjectModule();
        if (subjectModule != nullptr) {
            auto visualizationTarget = subjectModule->getParentModule();
            canvasProjection = CanvasProjection::getCanvasProjection(visualizationTarget->getCanvas());
        }
        WATCH(constraintAreaMin);
        WATCH(constraintAreaMax);
        WATCH(lastPosition);
        WATCH(lastOrientation);

        updateFromDisplayString = par("updateFromDisplayString");
        nodeVectorIdx_ = getParentModule()->getIndex();

        if (enableInitDebug_)
            std::cout << "MecStationaryMobility::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == INITSTAGE_SINGLE_MOBILITY) {
        if (enableInitDebug_)
            std::cout << "MecStationaryMobility::initialize - stage: INITSTAGE_SINGLE_MOBILITY - begins" << std::endl;

        initializeOrientation();
        initializePosition();

        if (enableInitDebug_)
            std::cout << "MecStationaryMobility::initialize - stage: INITSTAGE_SINGLE_MOBILITY - ends" << std::endl;
    }
}

void MecStationaryMobility::initializePosition()
{
    setInitialPosition();
    checkPosition();
    emitMobilityStateChangedSignal();
}

void MecStationaryMobility::setInitialPosition()
{
    // TODO: get the initial position from nodeInfo module
    Database* database = check_and_cast<Database*>(getSimulation()->getModuleByPath("database"));
    if (database == nullptr)
    {
        StationaryMobilityBase::setInitialPosition();
        return;
    }

    auto gnbPos = database->getGnbPosData(nodeVectorIdx_);
    lastPosition.x = gnbPos.first;
    lastPosition.y = gnbPos.second;

    if (par("updateDisplayString"))
        updateDisplayStringFromMobilityState();
}

void MecStationaryMobility::refreshDisplay() const
{
    if (updateFromDisplayString) {
        const_cast<MecStationaryMobility *>(this)->updateMobilityStateFromDisplayString();
        DirectiveResolver directiveResolver(const_cast<MecStationaryMobility *>(this));
        auto text = format.formatString(&directiveResolver);
        getDisplayString().setTagArg("t", 0, text.c_str());
    }
    else
        StationaryMobilityBase::refreshDisplay();
}

void MecStationaryMobility::updateMobilityStateFromDisplayString()
{
    char *end;
    double depth;
    cDisplayString& displayString = subjectModule->getDisplayString();
    canvasProjection->computeCanvasPoint(lastPosition, depth);
    double x = strtod(displayString.getTagArg("p", 0), &end);
    double y = strtod(displayString.getTagArg("p", 1), &end);
    auto newPosition = canvasProjection->computeCanvasPointInverse(cFigure::Point(x, y), depth);
    if (lastPosition != newPosition) {
        lastPosition = newPosition;
        emit(mobilityStateChangedSignal, const_cast<MecStationaryMobility *>(this));
    }
    Quaternion swing;
    Quaternion twist;
    Coord vector = canvasProjection->computeCanvasPointInverse(cFigure::Point(0, 0), 1);
    vector.normalize();
    lastOrientation.getSwingAndTwist(vector, swing, twist);
    double oldAngle;
    Coord axis;
    twist.getRotationAxisAndAngle(axis, oldAngle);
    double newAngle = math::deg2rad(strtod(displayString.getTagArg("a", 0), &end));
    if (oldAngle != newAngle) {
        lastOrientation *= Quaternion(vector, newAngle - oldAngle);
        emit(mobilityStateChangedSignal, const_cast<MecStationaryMobility *>(this));
    }
}

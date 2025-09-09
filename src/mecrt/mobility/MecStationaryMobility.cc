//
// Copyright (C) 2006 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#include "mecrt/mobility/MecStationaryMobility.h"

Define_Module(MecStationaryMobility);

void MecStationaryMobility::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        EV << "initializing MecStationaryMobility stage " << stage << endl;

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
    }
    else if (stage == INITSTAGE_SINGLE_MOBILITY) {
        EV << "initializing MecStationaryMobility stage " << stage << endl;

        initializeOrientation();
        initializePosition();

        unsigned short nodeId = getAncestorPar("macNodeId");
        database_->setRsuNodeId2Index(nodeId, nodeVectorIdx_);
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
    database_ = check_and_cast<Database*>(getSimulation()->getModuleByPath("database"));
    if (database_ == nullptr)
    {
        StationaryMobilityBase::setInitialPosition();
        return;
    }

    auto rsuPos = database_->getRsuPosData(nodeVectorIdx_);
    lastPosition.x = rsuPos.first;
    lastPosition.y = rsuPos.second;

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

//
// Copyright (C) 2015 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#include "mecrt/mobility/MecMobility.h"

#include <fstream>
#include <iostream>
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "inet/common/geometry/common/Quaternion.h"

Define_Module(MecMobility);

MecMobility::MecMobility()
{
    moveTimer_ = nullptr;
    showVehicle_ = nullptr;
    updateInterval_ = 0;
    stationary_ = false;
    lastPosition = Coord::ZERO;
    lastVelocity_ = Coord::ZERO;
    nextChange_ = -1;
    faceForward_ = false;
}

MecMobility::~MecMobility()
{
    if (moveTimer_ != nullptr)
        cancelAndDelete(moveTimer_);

    if (showVehicle_ != nullptr)
        cancelAndDelete(showVehicle_);
}

void MecMobility::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        EV << "MecMobility::initialize - initializing MecMobility stage INITSTAGE_LOCAL (" << stage << ")" << endl;
        // ==== MobilityBase::initialize ====
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
        WATCH(lastVelocity_);
        // ==== MecMobility::initialize ====
        
        moveTimer_ = new cMessage("move");
        updateInterval_ = par("updateInterval");
        faceForward_ = par("faceForward");

        maxSpeed_ = par("maxSpeed");
        targetPointIndex_ = 0;
        ground_ = findModuleFromPar<IGround>(par("groundModule"), this);
    }
    else if (stage == INITSTAGE_SINGLE_MOBILITY) {
        EV << "MecMobility::initialize - initializing MecMobility stage INITSTAGE_SINGLE_MOBILITY (" << stage << ")" << endl;

        vehIndex_ = getParentModule()->getIndex();
        std::string filePath = "./path/" + std::to_string(vehIndex_) + ".txt";
        EV << "MecMobility::initialize - reading waypoints from file: " << filePath << endl;
        readWaypointsFromFile(filePath.c_str());

        initializeOrientation();
        initializePosition();
    }
    else if (stage == INITSTAGE_LAST) {
        EV << "MecMobility::initialize - initializing MecMobility stage INITSTAGE_LAST (" << stage << ")" << endl;
        // hide the vehicle until the start time (make the icon small)
        getParentModule()->getDisplayString().setTagArg("i", 0, "invisible");

        // set a timer to show the vehicle at the start time
        showVehicle_ = new cMessage("showVehicle");
        scheduleAt(moveStartTime_, showVehicle_);
    }
}

void MecMobility::initializePosition()
{
    setInitialPosition();
    checkPosition();
    emitMobilityStateChangedSignal();
}

// called by MobilityBase::initialize(stage)
void MecMobility::setInitialPosition()
{
    lastPosition.x = waypoints_[targetPointIndex_].x;
    lastPosition.y = waypoints_[targetPointIndex_].y;

    EV << "MecMobility::setInitialPosition - vehicle " << vehIndex_ << " initial position: x=" 
        << lastPosition.x << ", y=" << lastPosition.y << endl;

    targetPointIndex_ = (targetPointIndex_ + 1) % waypoints_.size();
    nextChange_ = waypoints_[(targetPointIndex_ + 1) % waypoints_.size()].timestamp;

    Waypoint nextPosi = waypoints_[targetPointIndex_];
    double dt = nextPosi.timestamp;
    double dx = (nextPosi.x - lastPosition.x) / dt;
    double dy = (nextPosi.y - lastPosition.y) / dt;

    lastVelocity_.x = dx * updateInterval_.dbl();   // increment x for every updateInterval_
    lastVelocity_.y = dy * updateInterval_.dbl();   // increment y for every updateInterval_

    EV << "MecMobility::setInitialPosition - vehicle " << vehIndex_ << " initial velocity: x=" 
        << lastVelocity_.x << ", y=" << lastVelocity_.y << endl;

    Coord direction = lastVelocity_;
    direction.normalize();
    auto alpha = rad(atan2(direction.y, direction.x));
    auto beta = rad(-asin(direction.z));
    auto gamma = rad(0.0);
    lastOrientation = Quaternion(EulerAngles(alpha, beta, gamma));
        
    if (ground_) {
        lastPosition = ground_->computeGroundProjection(lastPosition);
        lastVelocity_ = ground_->computeGroundProjection(lastPosition + lastVelocity_) - lastPosition;
    }
}



void MecMobility::readWaypointsFromFile(const char *fileName)
{
    auto coordinateSystem = findModuleFromPar<IGeographicCoordinateSystem>(par("coordinateSystemModule"), this);
    char line[256];
    std::ifstream inputFile(fileName);
    if (!inputFile.is_open()) {
        throw cRuntimeError("Error: Could not open file '%s'", fileName);
    }

    double startTime = 10000000;    // a big value
    double stopTime = 0;
    while (true) {
        inputFile.getline(line, 256);
        if (!inputFile.fail()) {
            cStringTokenizer tokenizer(line, ",");
            double value1 = atof(tokenizer.nextToken());    // timestamp
            double value2 = atof(tokenizer.nextToken());    // x position
            double value3 = atof(tokenizer.nextToken());    // y position
            double timestamp;
            double xPosition;
            double yPosition;

            if (value1 > stopTime)
                stopTime = value1;

            if (value1 < startTime)
                startTime = value1;

            if (coordinateSystem == nullptr) {
                timestamp = value1;
                xPosition = value2;
                yPosition = value3;
            }
            else {
                Coord sceneCoordinate = coordinateSystem->computeSceneCoordinate(GeoCoord(deg(value1), deg(value2), m(value3)));
                xPosition = sceneCoordinate.x;
                yPosition = sceneCoordinate.y;
                timestamp = sceneCoordinate.z;
            }
            waypoints_.push_back(Waypoint(xPosition, yPosition, timestamp));
        }
        else
            break;
    }

    moveStartTime_ = startTime;
    moveStoptime_ = stopTime;
    EV << "MecMobility::readWaypointsFromFile - moveStartTime: " << moveStartTime_ << ", moveStoptime: " << moveStoptime_ << endl;
}

void MecMobility::handleSelfMessage(cMessage *message)
{
    EV << "MecMobility::handleSelfMessage - vehicle " << vehIndex_ << " self message " << message->getName() << " received" << endl;

    if (!strcmp(message->getName(), "showVehicle"))
    {
        // show the vehicle
        getParentModule()->getDisplayString().setTagArg("i", 0, "misc/car3_s");

        scheduleUpdate();
        return;
    }

    if (simTime() < moveStoptime_)
    {
        bool arriveTarget = (nextChange_ <= simTime());
        move(arriveTarget);
        orient(arriveTarget);
        emitMobilityStateChangedSignal();

        scheduleUpdate();
    }
    else
    {
        Waypoint lastPoint = waypoints_[targetPointIndex_];
        lastPosition = Coord(lastPoint.x, lastPoint.y);
        lastVelocity_ = Coord::ZERO;
        lastAngularVelocity_ = Quaternion::IDENTITY;
        emitMobilityStateChangedSignal();
        // hide the vehicle
        getParentModule()->getDisplayString().setTagArg("i", 0, "invisible");
    }
}

void MecMobility::scheduleUpdate()
{
    // cancelEvent(moveTimer_);
    if (!stationary_ && updateInterval_ != 0) {
        // periodic update is needed
        nextUpdate_ = simTime() + updateInterval_;
        if (nextChange_ != -1 && nextChange_ < nextUpdate_)
            // next change happens earlier than next update
            scheduleAt(nextChange_, moveTimer_);
        else
            // next update happens earlier than next change or there is no change at all
            scheduleAt(nextUpdate_, moveTimer_);
    }
    else if (nextChange_ != -1)
        // no periodic update is needed
        scheduleAt(nextChange_, moveTimer_);
}

void MecMobility::move(bool arriveTarget)
{
    // if reached the target point, change to the next one
    if (arriveTarget) 
    {
        Waypoint currPosi = waypoints_[targetPointIndex_];
        lastPosition = Coord(currPosi.x, currPosi.y);
        
        targetPointIndex_ = (targetPointIndex_ + 1) % waypoints_.size();
        nextChange_ = waypoints_[targetPointIndex_].timestamp;

        Waypoint nextPosi = waypoints_[targetPointIndex_];
        double dt = nextPosi.timestamp - currPosi.timestamp;
        if (dt <= 0)
        {
            std::cout << "MecMobility::move - invalid waypoint timestamp, vehicle " << vehIndex_ << " target point: x=" 
                << nextPosi.x << ", y=" << nextPosi.y << ", timestamp=" << nextPosi.timestamp << std::endl;
        }
        double dx = (nextPosi.x - currPosi.x) / dt;
        double dy = (nextPosi.y - currPosi.y) / dt;

        lastVelocity_.x = dx * updateInterval_.dbl();   // increment x for every updateInterval_
        lastVelocity_.y = dy * updateInterval_.dbl();   // increment y for every updateInterval_

        if (ground_) {
            lastPosition = ground_->computeGroundProjection(lastPosition);
            lastVelocity_ = ground_->computeGroundProjection(lastPosition + lastVelocity_) - lastPosition;
        }

        EV << "MecMobility::move - arrived at target point, vehicle " << vehIndex_ << " target point: x=" 
            << lastPosition.x << ", y=" << lastPosition.y << endl;
        EV << "MecMobility::move - vehicle " << vehIndex_ << " velocity: x=" << lastVelocity_.x << ", y=" << lastVelocity_.y << endl;
    }
    else    // a small increment towards the next point
    {
        lastPosition.x += lastVelocity_.x;
        lastPosition.y += lastVelocity_.y;

        EV << "MecMobility::move - still on the way to target point" << endl;
        EV << "MecMobility::move - vehicle " << vehIndex_ << " position: x=" << lastPosition.x << ", y=" << lastPosition.y << endl;
    }
}

void MecMobility::orient(bool arriveTarget)
{
    if (ground_) {
        Coord groundNormal = ground_->computeGroundNormal(lastPosition);

        // this will make the wheels follow the ground
        Quaternion quat = Quaternion::rotationFromTo(Coord(0, 0, 1), groundNormal);

        Coord groundTangent = groundNormal % lastVelocity_;
        groundTangent.normalize();
        Coord direction = groundTangent % groundNormal;
        direction.normalize(); // this is lastSpeed, normalized and adjusted to be perpendicular to groundNormal

        // our model looks in this direction if we only rotate the Z axis to match the ground normal
        Coord groundX = quat.rotate(Coord(1, 0, 0));

        double dp = groundX * direction;

        double angle;

        if (((groundX % direction) * groundNormal) > 0)
            angle = std::acos(dp);
        else
            // correcting for the case where the angle should be over 90 degrees (or under -90):
            angle = 2 * M_PI - std::acos(dp);

        // and finally rotating around the now-ground-orthogonal local Z
        quat *= Quaternion(Coord(0, 0, 1), angle);

        lastOrientation = quat;
    }
    else if (faceForward_ && arriveTarget) {
        // determine orientation based on direction
        if (lastVelocity_ != Coord::ZERO) {
            Coord direction = lastVelocity_;
            direction.normalize();
            auto alpha = rad(atan2(direction.y, direction.x));
            auto beta = rad(-asin(direction.z));
            auto gamma = rad(0.0);
            lastOrientation = Quaternion(EulerAngles(alpha, beta, gamma));
        }
    }
}

const Coord& MecMobility::getCurrentPosition()
{
    return lastPosition;
}

const Coord& MecMobility::getCurrentVelocity()
{
    return lastVelocity_;
}

const Quaternion& MecMobility::getCurrentAngularPosition()
{
    return lastOrientation;
}

const Quaternion& MecMobility::getCurrentAngularVelocity()
{
    return lastAngularVelocity_;
}

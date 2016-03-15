/** @file
    @brief Implementation

    @date 2016

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2016 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include <ViveServerDriverHost.h>

// Library/third-party includes
// - none

// Standard includes
#include <iostream>

using namespace vr;

#ifdef DISABLE_LOG_EVENTS
#define LOG_EVENTS(X)                                                          \
    do {                                                                       \
    } while (0)
#else
#define LOG_EVENTS(X)                                                          \
    do {                                                                       \
        std::cout << X << std::endl;                                           \
    } while (0)
#endif

vr::ViveServerDriverHost::ViveServerDriverHost() {}

bool ViveServerDriverHost::TrackedDeviceAdded(
    const char *pchDeviceSerialNumber) {
    LOG_EVENTS("TrackedDeviceAdded(" << pchDeviceSerialNumber << ")");
    return true;
}

void ViveServerDriverHost::TrackedDevicePoseUpdated(
    uint32_t unWhichDevice, const DriverPose_t &newPose) {

    LOG_EVENTS("TrackedDevicePoseUpdated(" << unWhichDevice << ", newPose)");
}

void ViveServerDriverHost::TrackedDevicePropertiesChanged(
    uint32_t unWhichDevice) {
    LOG_EVENTS("TrackedDevicePropertiesChanged(" << unWhichDevice << ")");
}

void ViveServerDriverHost::VsyncEvent(double vsyncTimeOffsetSeconds) {
    LOG_EVENTS("VsyncEvent(" << vsyncTimeOffsetSeconds << ")");
}

void ViveServerDriverHost::TrackedDeviceButtonPressed(uint32_t unWhichDevice,
                                                      EVRButtonId eButtonId,
                                                      double eventTimeOffset) {
    LOG_EVENTS("TrackedDeviceButtonPressed("
               << unWhichDevice << ", " << eButtonId << ", " << eventTimeOffset
               << ")");
}

void ViveServerDriverHost::TrackedDeviceButtonUnpressed(
    uint32_t unWhichDevice, EVRButtonId eButtonId, double eventTimeOffset) {

    LOG_EVENTS("TrackedDeviceButtonUnpressed("
               << unWhichDevice << ", " << eButtonId << ", " << eventTimeOffset
               << ")");
}

void ViveServerDriverHost::TrackedDeviceButtonTouched(uint32_t unWhichDevice,
                                                      EVRButtonId eButtonId,
                                                      double eventTimeOffset) {

    LOG_EVENTS("TrackedDeviceButtonTouched("
               << unWhichDevice << ", " << eButtonId << ", " << eventTimeOffset
               << ")");
}

void ViveServerDriverHost::TrackedDeviceButtonUntouched(
    uint32_t unWhichDevice, EVRButtonId eButtonId, double eventTimeOffset) {

    LOG_EVENTS("TrackedDeviceButtonUntouched("
               << unWhichDevice << ", " << eButtonId << ", " << eventTimeOffset
               << ")");
}

void ViveServerDriverHost::TrackedDeviceAxisUpdated(
    uint32_t unWhichDevice, uint32_t unWhichAxis,
    const VRControllerAxis_t &axisState) {

    LOG_EVENTS("TrackedDeviceAxisUpdated(" << unWhichDevice << ", "
                                           << unWhichAxis << ", axisState)");
}

void ViveServerDriverHost::MCImageUpdated() { LOG_EVENTS("MCImageUpdated()"); }

IVRSettings *
ViveServerDriverHost::GetSettings(const char *pchInterfaceVersion) {
    LOG_EVENTS("GetSettings(" << pchInterfaceVersion << ")");

    return m_vrSettings;
}

void ViveServerDriverHost::PhysicalIpdSet(uint32_t unWhichDevice,
                                          float fPhysicalIpdMeters) {

    LOG_EVENTS("PhysicalIpdSet(" << unWhichDevice << ", " << fPhysicalIpdMeters
                                 << ")");
}

void ViveServerDriverHost::ProximitySensorState(
    uint32_t unWhichDevice, bool bProximitySensorTriggered) {
    LOG_EVENTS("ProximitySensorState(" << unWhichDevice << ", "
                                       << std::boolalpha
                                       << bProximitySensorTriggered << ")");
}

void ViveServerDriverHost::VendorSpecificEvent(uint32_t unWhichDevice,
                                               vr::EVREventType eventType,
                                               const VREvent_Data_t &eventData,
                                               double eventTimeOffset) {

    LOG_EVENTS("VendorSpecificEvent("
               << unWhichDevice << ", eventType, eventData, " << eventTimeOffset
               << ")");
}

bool ViveServerDriverHost::IsExiting() {
    LOG_EVENTS("IsExiting()");
    return isExiting_;
}
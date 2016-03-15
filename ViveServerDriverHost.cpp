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
// - none

using namespace vr;

bool ViveServerDriverHost::TrackedDeviceAdded(
    const char *pchDeviceSerialNumber) {
    return true;
}

void ViveServerDriverHost::TrackedDevicePoseUpdated(
    uint32_t unWhichDevice, const DriverPose_t &newPose) {}

void ViveServerDriverHost::TrackedDevicePropertiesChanged(
    uint32_t unWhichDevice) {}

void ViveServerDriverHost::VsyncEvent(double vsyncTimeOffsetSeconds) {}

void ViveServerDriverHost::TrackedDeviceButtonPressed(uint32_t unWhichDevice,
                                                      EVRButtonId eButtonId,
                                                      double eventTimeOffset) {}

void ViveServerDriverHost::TrackedDeviceButtonUnpressed(
    uint32_t unWhichDevice, EVRButtonId eButtonId, double eventTimeOffset) {}

void ViveServerDriverHost::TrackedDeviceButtonTouched(uint32_t unWhichDevice,
                                                      EVRButtonId eButtonId,
                                                      double eventTimeOffset) {}

void ViveServerDriverHost::TrackedDeviceButtonUntouched(
    uint32_t unWhichDevice, EVRButtonId eButtonId, double eventTimeOffset) {}

void ViveServerDriverHost::TrackedDeviceAxisUpdated(
    uint32_t unWhichDevice, uint32_t unWhichAxis,
    const VRControllerAxis_t &axisState) {}

void ViveServerDriverHost::MCImageUpdated() {}

IVRSettings *
ViveServerDriverHost::GetSettings(const char *pchInterfaceVersion) {
    
}

void ViveServerDriverHost::PhysicalIpdSet(uint32_t unWhichDevice,
                                          float fPhysicalIpdMeters) {}

void ViveServerDriverHost::ProximitySensorState(
    uint32_t unWhichDevice, bool bProximitySensorTriggered) {}

void ViveServerDriverHost::VendorSpecificEvent(uint32_t unWhichDevice,
                                               vr::EVREventType eventType,
                                               const VREvent_Data_t &eventData,
                                               double eventTimeOffset) {}

bool ViveServerDriverHost::IsExiting() {}
/** @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Vuzix Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>
#include <osvr/Util/PlatformConfig.h>
#include "DriverLoader.h"
#include "FindDriver.h"
#include "GetProvider.h"
#include "ServerDriverHost.h"

// Generated JSON header file
#include "com_osvr_Vive_json.h"

// Library/third-party includes
#include <math.h>
#include <openvr_driver.h>

// Standard includes
#include <iostream>
#include <memory>
#include <vector>

using namespace vr;

// Anonymous namespace to avoid symbol collision
namespace {

class ViveDriverHost : ServerDriverHost {

  public:
      ViveDriverHost::ViveDriverHost(OSVR_PluginRegContext ctx) {
        /// Create the initialization options
        OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

        osvrDeviceTrackerConfigure(opts, &m_tracker);

        /// Create the sync device token with the options
        m_dev.initSync(ctx, "Vive", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(com_osvr_Vive_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);
    }

    OSVR_ReturnCode ViveDriverHost::update() { return OSVR_RETURN_SUCCESS; }

    void ViveDriverHost::TrackedDevicePoseUpdated(uint32_t unWhichDevice,
        const DriverPose_t &newPose) {

        std::cout << "TrackedDevicePoseUpdated(" << unWhichDevice << ", newPose)" << std::endl;

        OSVR_TimeValue now;
        osvrTimeValueGetNow(&now);
        if (newPose.poseIsValid) {
            OSVR_PoseState pose;
            pose.translation.data[0] = newPose.vecPosition[0];
            pose.translation.data[1] = newPose.vecPosition[1];
            pose.translation.data[2] = newPose.vecPosition[2];
            pose.rotation.data[0] = newPose.qRotation.w;
            pose.rotation.data[1] = newPose.qRotation.x;
            pose.rotation.data[2] = newPose.qRotation.y;
            pose.rotation.data[3] = newPose.qRotation.z;

            osvrDeviceTrackerSendPoseTimestamped(m_dev, m_tracker, &pose,
                unWhichDevice, &now);
        }
    }

  private:
    osvr::pluginkit::DeviceToken m_dev;
    OSVR_TrackerDeviceInterface m_tracker;
};

class HardwareDetection {
  public:
    HardwareDetection() {}
    OSVR_ReturnCode operator()(OSVR_PluginRegContext ctx) {

        std::cout << "PLUGIN: Got a hardware detection request" << std::endl;

        if (m_found) {
            return OSVR_RETURN_SUCCESS;
        }

        auto driverLocation = osvr::vive::findDriver();
        if (driverLocation.found) {
            std::cout << "Found the Vive driver at "
                      << driverLocation.driverFile << std::endl;
        } else {
            std::cout
                << "Could not find the native SteamVR Vive driver, exiting!"
                << std::endl;
            return OSVR_RETURN_FAILURE;
        }

        auto vive = osvr::vive::DriverLoader::make(driverLocation.driverRoot,
                                                   driverLocation.driverFile);
        if (vive->isHMDPresent()) {
            std::cout << "Vive is connected." << std::endl;
            std::unique_ptr<vr::ServerDriverHost> serverDriverHost(
                new vr::ServerDriverHost);

            osvr::vive::getProvider<vr::IServerTrackedDeviceProvider>(
                std::move(vive), nullptr, serverDriverHost.get(), ".");
        }
    }

  private:
    bool m_found;
};
} // namespace

OSVR_PLUGIN(com_osvr_Vive) {
    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}
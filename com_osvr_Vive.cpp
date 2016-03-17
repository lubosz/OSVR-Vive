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
#include "DriverWrapper.h"

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
static const auto PREFIX = "PLUGIN: ";
class ViveDriverHost : public ServerDriverHost {

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

        std::cout << PREFIX << "TrackedDevicePoseUpdated(" << unWhichDevice
                  << ", newPose)" << std::endl;

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

    void DeviceDescriptorUpdated(std::string const &json){
        
        m_dev.sendJsonDescriptor(json);
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

        auto vive = osvr::vive::DriverWrapper(new ViveDriverHost(ctx));

        if (vive.foundDriver()) {
            std::cout << PREFIX << "Found the Vive driver at "
                      << vive.getDriverFileLocation() << std::endl;
        }

        if (!vive.haveDriverLoaded()) {
            std::cout << PREFIX << "Could not open driver." << std::endl;
            return OSVR_RETURN_FAILURE;
        }

        if (vive.foundConfigDirs()) {
            std::cout << PREFIX
                      << "Driver config dir is: " << vive.getDriverConfigDir()
                      << std::endl;
        }

        if (!vive) {
            std::cerr << PREFIX
                      << "Error in first-stage Vive driver startup. Exiting"
                      << std::endl;
            return OSVR_RETURN_FAILURE;
        }

        if (!vive.isHMDPresent()) {
            std::cerr << PREFIX
                      << "Driver loaded, but no Vive is connected. Exiting"
                      << std::endl;
            return OSVR_RETURN_FAILURE;
        }

        std::cout << PREFIX << "Vive is connected." << std::endl;

        if (!vive.startServerDeviceProvider()) {
            std::cerr
                << PREFIX
                << "Error: could not start the server device provider in the "
                   "Vive driver. Exiting."
                << std::endl;
            return OSVR_RETURN_FAILURE;
        }

        /// Power the system up.
        vive.serverDevProvider().LeaveStandby();

        auto handleNewDevice = [&](const char *serialNum) {
            auto dev = vive.serverDevProvider().FindTrackedDeviceDriver(
                serialNum, vr::ITrackedDeviceServerDriver_Version);
            if (!dev) {
                std::cout
                    << PREFIX
                    << "Couldn't find the corresponding device driver for "
                    << serialNum << std::endl;
                return false;
            }
            auto ret = vive.addAndActivateDevice(dev);
            if (!ret.first) {
                std::cout << PREFIX << "Device with serial number " << serialNum
                          << " couldn't be added to the devices vector."
                          << std::endl;
                return false;
            }
            std::cout << "\n" << PREFIX << "Device with s/n " << serialNum
                      << " activated, assigned ID " << ret.second << std::endl;
            return true;
        };

        vive.driverHost().onTrackedDeviceAdded = handleNewDevice;

        {
            auto numDevices = vive.serverDevProvider().GetTrackedDeviceCount();
            std::cout << PREFIX << "Got " << numDevices
                      << " tracked devices at startup" << std::endl;
            for (decltype(numDevices) i = 0; i < numDevices; ++i) {
                auto dev = vive.serverDevProvider().GetTrackedDeviceDriver(
                    i, vr::ITrackedDeviceServerDriver_Version);
                vive.addAndActivateDevice(dev);
                std::cout << PREFIX << "Device " << i << std::endl;
                addDeviceType(dev);
            }
        }
        return OSVR_RETURN_SUCCESS;
    }

    static void addDeviceType(vr::ITrackedDeviceServerDriver *dev) {
        // @todo add device type (controller, tracker) to generate
        // dynamic device descriptor
    }

  private:
    bool m_found;
    std::vector<std::string> m_deviceTypes;
};
} // namespace

OSVR_PLUGIN(com_osvr_Vive) {
    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}
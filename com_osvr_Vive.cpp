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
#include "DriverWrapper.h"
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>
#include <osvr/Util/PlatformConfig.h>

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
static const auto PREFIX = "[OSVR-Vive] ";
class ViveDriverHost : public ServerDriverHost {
  public:
    /// @return false if we failed to start up for some reason.
    bool start(OSVR_PluginRegContext ctx, osvr::vive::DriverWrapper &&inVive) {
        if (!inVive) {
            std::cerr << PREFIX << "Error: called ViveDriverHost::start() with "
                                   "an invalid vive object!"
                      << std::endl;
        }
        /// Take ownership of the Vive.
        m_vive.reset(new osvr::vive::DriverWrapper(std::move(inVive)));

        /// Finish setting up the Vive.
        if (!m_vive->startServerDeviceProvider()) {
            std::cerr
                << PREFIX
                << "Error: could not start the server device provider in the "
                   "Vive driver. Exiting."
                << std::endl;
            return false;
        }

        /// Power the system up.
        m_vive->serverDevProvider().LeaveStandby();

        auto handleNewDevice =
            [&](const char *serialNum) {
                auto dev = m_vive->serverDevProvider().FindTrackedDeviceDriver(
                    serialNum, vr::ITrackedDeviceServerDriver_Version);
                if (!dev) {
                    std::cout
                        << PREFIX
                        << "Couldn't find the corresponding device driver for "
                        << serialNum << std::endl;
                    return false;
                }
                auto ret = m_vive->addAndActivateDevice(dev);
                if (!ret.first) {
                    std::cout << PREFIX << "Device with serial number "
                              << serialNum
                              << " couldn't be added to the devices vector."
                              << std::endl;
                    return false;
                }
                std::cout << "\n"
                          << PREFIX << "Device with s/n " << serialNum
                          << " activated, assigned ID " << ret.second
                          << std::endl;
                return true;
            };

        m_vive->driverHost().onTrackedDeviceAdded = handleNewDevice;

        {
            auto numDevices =
                m_vive->serverDevProvider().GetTrackedDeviceCount();
            std::cout << PREFIX << "Got " << numDevices
                      << " tracked devices at startup" << std::endl;
            for (decltype(numDevices) i = 0; i < numDevices; ++i) {
                auto dev = m_vive->serverDevProvider().GetTrackedDeviceDriver(
                    i, vr::ITrackedDeviceServerDriver_Version);
                m_vive->addAndActivateDevice(dev);
                std::cout << PREFIX << "Device " << i << std::endl;
#if 0
                addDeviceType(dev);
#endif
            }
        }

        /// Finish setting this up as an OSVR device.
        /// Create the initialization options
        OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

        osvrDeviceTrackerConfigure(opts, &m_tracker);

        /// Create the sync device token with the options
        m_dev.initSync(ctx, "Vive", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(com_osvr_Vive_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);

        return true;
    }

    OSVR_ReturnCode update() {
        m_vive->serverDevProvider().RunFrame();
        return OSVR_RETURN_SUCCESS;
    }

    void TrackedDevicePoseUpdated(uint32_t unWhichDevice,
                                  const DriverPose_t &newPose) override {
#if 0
        std::cout << PREFIX << "TrackedDevicePoseUpdated(" << unWhichDevice
                  << ", newPose)" << std::endl;
#endif
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

    void DeviceDescriptorUpdated(std::string const &json) {

        m_dev.sendJsonDescriptor(json);
    }

  private:
    osvr::pluginkit::DeviceToken m_dev;
    OSVR_TrackerDeviceInterface m_tracker;
    std::unique_ptr<osvr::vive::DriverWrapper> m_vive;
};
using DriverHostPtr = std::unique_ptr<ViveDriverHost>;

class HardwareDetection {

  public:
    HardwareDetection() {}
    OSVR_ReturnCode operator()(OSVR_PluginRegContext ctx) {

        std::cout << PREFIX << "Got a hardware detection request" << std::endl;

        if (m_driverHost) {
            // Already found a Vive.
            /// @todo what are the semantics of the return value from a hardware
            /// detect?
            return OSVR_RETURN_SUCCESS;
        }

        {
            DriverHostPtr host(new ViveDriverHost);

            auto vive = osvr::vive::DriverWrapper(host.get());

            if (vive.foundDriver()) {
                std::cout << PREFIX << "Found the Vive driver at "
                          << vive.getDriverFileLocation() << std::endl;
            }

            if (!vive.haveDriverLoaded()) {
                std::cout << PREFIX << "Could not open driver." << std::endl;
                return OSVR_RETURN_FAILURE;
            }

            if (vive.foundConfigDirs()) {
                std::cout << PREFIX << "Driver config dir is: "
                          << vive.getDriverConfigDir() << std::endl;
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

            /// Hand the Vive object off to the OSVR driver.
            auto startResult = host->start(ctx, std::move(vive));
            if (startResult) {
                /// and it started up the rest of the way just fine!
                /// We'll keep the driver around!
                m_driverHost = std::move(host);
                std::cout << PREFIX
                          << "Vive driver finished startup successfully!"
                          << std::endl;
                return OSVR_RETURN_SUCCESS;
            }
        }

        std::cout << PREFIX << "Vive driver startup failed somewhere, "
                               "unloading to perhaps try again later."
                  << std::endl;
        return OSVR_RETURN_FAILURE;
    }

    static void addDeviceType(vr::ITrackedDeviceServerDriver *dev) {
        // @todo add device type (controller, tracker) to generate
        // dynamic device descriptor
    }

  private:
    std::vector<std::string> m_deviceTypes;
    /// This is the OSVR driver object, which also serves as the "SteamVR"
    /// driver host. We can only run one Vive at a time.
    std::unique_ptr<ViveDriverHost> m_driverHost;
};
} // namespace

OSVR_PLUGIN(com_osvr_Vive) {
    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}

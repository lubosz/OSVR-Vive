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
#include "InterfaceTraits.h"
#include "PropertyHelper.h"
#include <osvr/PluginKit/AnalogInterfaceC.h>
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>
#include <osvr/Util/PlatformConfig.h>

// Generated JSON header file
#include "com_osvr_Vive_json.h"

// Library/third-party includes
#include <math.h>
#include <openvr_driver.h>

// Standard includes
#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace osvr::vive;

using namespace vr;

// Anonymous namespace to avoid symbol collision
namespace {

struct TrackingReport {
    OSVR_TimeValue timestamp;
    OSVR_PoseReport report;
};

using TrackingDeque = std::deque<TrackingReport>;

static const auto NUM_ANALOGS = 1;
static const auto IPD_ANALOG = 0;
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
                auto ret = activateDevice(dev);
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
                activateDevice(dev);

#if 0
                addDeviceType(dev);
#endif
            }
        }

        /// Finish setting this up as an OSVR device.
        /// Create the initialization options
        OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

        osvrDeviceTrackerConfigure(opts, &m_tracker);
        osvrDeviceAnalogConfigure(opts, &m_analog, NUM_ANALOGS);

        /// Because the callbacks may not come from the same thread that calls
        /// RunFrame, we need to be careful to not send directly from those
        /// callbacks.
        m_dev.initSync(ctx, "Vive", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(com_osvr_Vive_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);

        m_mainThread = std::this_thread::get_id();
        return true;
    }

    OSVR_ReturnCode update() {
        m_vive->serverDevProvider().RunFrame();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            /// Handle a fixed number of tracking reports that have been queued
            /// up.
            auto numTrackers = m_trackingReports.size();
            for (std::size_t i = 0; i < numTrackers; ++i) {
                auto &out = m_trackingReports.front();
                osvrDeviceTrackerSendPoseTimestamped(
                    m_dev, m_tracker, &out.report.pose, out.report.sensor,
                    &out.timestamp);
                m_trackingReports.pop_front();
            }
        }
        return OSVR_RETURN_SUCCESS;
    }

    std::pair<bool, std::uint32_t>
    activateDevice(vr::ITrackedDeviceServerDriver *dev) {
        auto ret = activateDeviceImpl(dev);
        auto mfrProp = getStringProperty(dev, vr::Prop_ManufacturerName_String);
        auto modelProp = getStringProperty(dev, vr::Prop_ModelNumber_String);
        auto serialProp = getStringProperty(dev, vr::Prop_SerialNumber_String);
        std::cout << PREFIX;
        if (ret.first) {
            std::cout << "Assigned sensor ID " << ret.second << " to ";
        } else {
            std::cout << "Could not assign a sensor ID to ";
        }
        std::cout << mfrProp.first << " " << modelProp.first << " "
                  << serialProp.first << std::endl;
        return ret;
    }
    std::pair<bool, std::uint32_t>
    activateDeviceImpl(vr::ITrackedDeviceServerDriver *dev) {
        auto &devs = m_vive->devices();
        if (getComponent<vr::IVRDisplayComponent>(dev)) {
            /// This is the HMD, since it has the display component.
            /// Always sensor 0.
            return devs.addAndActivateDeviceAt(dev, 0);
        }
        if (getComponent<vr::IVRControllerComponent>(dev)) {
            /// This is a controller.
            for (auto ctrlIdx : {1, 2}) {
                if (!devs.hasDeviceAt(ctrlIdx)) {
                    return devs.addAndActivateDeviceAt(dev, ctrlIdx);
                }
            }
        }
        /// This still may be a controller, if somehow there are more than 2...
        return devs.addAndActivateDevice(dev);
    }

    void TrackedDevicePoseUpdated(uint32_t unWhichDevice,
                                  const DriverPose_t &newPose) override {
#if 0
        if (std::this_thread::get_id() != m_mainThread) {
            std::cout << PREFIX << "Callback from a different thread!"
                      << std::endl;
        }
#endif
#if 0
        std::cout << PREFIX << "TrackedDevicePoseUpdated(" << unWhichDevice
                  << ", newPose)" << std::endl;
#endif
        OSVR_TimeValue now;
        osvrTimeValueGetNow(&now);
        if (newPose.poseIsValid) {
            TrackingReport out;
            out.timestamp = osvr::util::time::getNow();
            out.report.sensor = unWhichDevice;
            auto &pose = out.report.pose;
            pose.translation.data[0] = newPose.vecPosition[0];
            pose.translation.data[1] = newPose.vecPosition[1];
            pose.translation.data[2] = newPose.vecPosition[2];
            pose.rotation.data[0] = newPose.qRotation.w;
            pose.rotation.data[1] = newPose.qRotation.x;
            pose.rotation.data[2] = newPose.qRotation.y;
            pose.rotation.data[3] = newPose.qRotation.z;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_trackingReports.push_back(std::move(out));
#if 0
            osvrDeviceTrackerSendPoseTimestamped(m_dev, m_tracker, &pose,
                                                 unWhichDevice, &now);
#endif
            }
        }
    }

    void PhysicalIpdSet(uint32_t unWhichDevice,
                        float fPhysicalIpdMeters) override {
/// @todo
#if 0
        auto now = osvr::util::time::getNow();
        osvrDeviceAnalogSetValueTimestamped(m_dev, m_analog, fPhysicalIpdMeters,
                                            IPD_ANALOG, &now);
#endif
    }

    void ProximitySensorState(uint32_t unWhichDevice,
                              bool bProximitySensorTriggered) override {
        /// @todo
    }

    void DeviceDescriptorUpdated(std::string const &json) {

        m_dev.sendJsonDescriptor(json);
    }

  private:
    osvr::pluginkit::DeviceToken m_dev;
    OSVR_TrackerDeviceInterface m_tracker;
    OSVR_AnalogDeviceInterface m_analog;
    std::unique_ptr<osvr::vive::DriverWrapper> m_vive;
    std::thread::id m_mainThread;

    std::mutex m_mutex;
    TrackingDeque m_trackingReports;
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

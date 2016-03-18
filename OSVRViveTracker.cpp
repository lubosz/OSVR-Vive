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
#include "OSVRViveTracker.h"
#include "DriverWrapper.h"
#include "InterfaceTraits.h"
#include "PropertyHelper.h"

// Generated JSON header file
#include "com_osvr_Vive_json.h"

// Library/third-party includes
#include <osvr/Util/EigenCoreGeometry.h>
#include <osvr/Util/EigenInterop.h>

// Standard includes
// - none

namespace osvr {
namespace vive {

    namespace ei = osvr::util::eigen_interop;

    static const auto NUM_ANALOGS = 1;
    static const auto IPD_ANALOG = 0;

    static const auto PREFIX = "[OSVR-Vive] ";
    bool ViveDriverHost::start(OSVR_PluginRegContext ctx,
                               osvr::vive::DriverWrapper &&inVive) {
        if (!inVive) {
            std::cerr << PREFIX << "Error: called ViveDriverHost::start() with "
                                   "an invalid vive object!"
                      << std::endl;
        }
        /// Take ownership of the Vive.
        m_vive.reset(new osvr::vive::DriverWrapper(std::move(inVive)));

        /// Finish setting up the Vive.
        if (!m_vive->startServerDeviceProvider()) {
            std::cerr << PREFIX << "Error: could not start the server "
                                   "device provider in the "
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

        /// Reserve ID 0 for the HMD
        m_vive->devices().reserveIds(1);

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

        /// Because the callbacks may not come from the same thread that
        /// calls
        /// RunFrame, we need to be careful to not send directly from those
        /// callbacks.
        m_dev.initSync(ctx, "Vive", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(com_osvr_Vive_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);

        return true;
    }
    inline OSVR_ReturnCode ViveDriverHost::update() {
        m_vive->serverDevProvider().RunFrame();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            /// Copy a fixed number of tracking reports that have been
            /// queued up.
            auto numTrackers = m_trackingReports.size();
            for (std::size_t i = 0; i < numTrackers; ++i) {
                m_currentTrackingReports.push_back(m_trackingReports.front());
                m_trackingReports.pop_front();
            }

        } // unlock
        // Now that we're out of that mutex, we can go ahead and actually send
        // the reports.
        for (auto &out : m_currentTrackingReports) {
            convertAndSendTracker(out.timestamp, out.sensor, out.report);
        }
        // then clear this temporary buffer for next time.
        m_currentTrackingReports.clear();
        return OSVR_RETURN_SUCCESS;
    }

    std::pair<bool, std::uint32_t>
    ViveDriverHost::activateDevice(vr::ITrackedDeviceServerDriver *dev) {
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
    ViveDriverHost::activateDeviceImpl(vr::ITrackedDeviceServerDriver *dev) {
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
        /// This still may be a controller, if somehow there are more than
        /// 2...
        return devs.addAndActivateDevice(dev);
    }

    void ViveDriverHost::convertAndSendTracker(OSVR_TimeValue const &tv,
                                               OSVR_ChannelCount sensor,
                                               const DriverPose_t &newPose) {

        if (!newPose.poseIsValid) {
            /// @todo better handle non-valid states?
            return;
        }
        auto quatFromSteamVR = [](vr::HmdQuaternion_t const &q) {
            return Eigen::Quaterniond(q.w, q.x, q.y, q.z);
        };

        using namespace Eigen;

        auto qRotation = quatFromSteamVR(newPose.qRotation);

        auto driverFromHeadRotation =
            quatFromSteamVR(newPose.qDriverFromHeadRotation);

        Translation3d driverFromHeadTranslation(
            Vector3d::Map(newPose.vecDriverFromHeadTranslation));
        Isometry3d driverFromHead =
            driverFromHeadTranslation * driverFromHeadRotation;
#if 0
        driverFromHead.fromPositionOrientationScale(
            Vector3d::Map(newPose.vecDriverFromHeadTranslation),
            driverFromHeadRotation, Vector3d::Ones());
#endif
        auto worldFromDriverRotation =
            quatFromSteamVR(newPose.qWorldFromDriverRotation);
        Translation3d worldFromDriverTranslation(
            Vector3d::Map(newPose.vecWorldFromDriverTranslation));
        Isometry3d worldFromDriver =
            worldFromDriverTranslation * worldFromDriverRotation;
#if 0
        worldFromDriver.fromPositionOrientationScale(
            Eigen::Vector3d::Map(newPose.vecWorldFromDriverTranslation),
            worldFromDriverRotation, Eigen::Vector3d::Ones());
#endif
#if 0
        ei::map(pose.translation) =
            (worldFromDriverTranslation * worldFromDriverRotation *
                Eigen::Translation3d(Eigen::Vector3d::Map(newPose.vecPosition)) *
                driverFromHeadTranslation * worldFromDriverRotation)
            .translation();
#endif

        OSVR_Pose3 pose;
        ei::map(pose.translation) =
            (worldFromDriver *
             Eigen::Translation3d(Eigen::Vector3d::Map(newPose.vecPosition)) *
             driverFromHeadTranslation)
                .translation();
        ei::map(pose.rotation) =
            worldFromDriverRotation * qRotation * driverFromHeadRotation;

        osvrDeviceTrackerSendPoseTimestamped(m_dev, m_tracker, &pose, sensor,
                                             &tv);
    }

    void ViveDriverHost::TrackedDevicePoseUpdated(uint32_t unWhichDevice,
                                                  const DriverPose_t &newPose) {
#if 0
        std::cout << PREFIX << "TrackedDevicePoseUpdated(" << unWhichDevice
            << ", newPose)" << std::endl;
#endif
        TrackingReport out;
        out.timestamp = osvr::util::time::getNow();
        out.sensor = unWhichDevice;
        out.report = newPose;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_trackingReports.push_back(std::move(out));
        }
    }

    void ViveDriverHost::PhysicalIpdSet(uint32_t unWhichDevice,
                                        float fPhysicalIpdMeters) {
/// @todo - queue it up and send it from the main thread.
#if 0
        auto now = osvr::util::time::getNow();
        osvrDeviceAnalogSetValueTimestamped(m_dev, m_analog, fPhysicalIpdMeters,
            IPD_ANALOG, &now);
#endif
    }

    void ViveDriverHost::ProximitySensorState(uint32_t unWhichDevice,
                                              bool bProximitySensorTriggered) {
        /// @todo - queue it up and send it from the main thread.
    }

#if 0
    void
    ViveDriverHost::DeviceDescriptorUpdated(std::string const &json) {

        m_dev.sendJsonDescriptor(json);
    }
#endif

} // namespace vive
} // namespace osvr

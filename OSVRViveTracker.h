
/** @file
    @brief Header

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

#ifndef INCLUDED_OSVRViveTracker_h_GUID_BDA684D2_7F2D_4483_660D_C9D679BB1F67
#define INCLUDED_OSVRViveTracker_h_GUID_BDA684D2_7F2D_4483_660D_C9D679BB1F67

// Internal Includes
#include "ServerDriverHost.h"
#include <osvr/PluginKit/AnalogInterfaceC.h>
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>
#include <osvr/Util/ClientReportTypesC.h>
#include <osvr/Util/TimeValue.h>

// Library/third-party includes
// - none

// Standard includes
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace osvr {
namespace vive {
    struct TrackingReport {
        OSVR_TimeValue timestamp;
        OSVR_ChannelCount sensor;
        vr::DriverPose_t report;
    };

    using TrackingDeque = std::deque<TrackingReport>;
    using TrackingVector = std::vector<TrackingReport>;

    class DriverWrapper;

    class ViveDriverHost : public ServerDriverHost {
      public:
        /// @return false if we failed to start up for some reason.
        bool start(OSVR_PluginRegContext ctx,
                   osvr::vive::DriverWrapper &&inVive);

        /// Standard OSVR device callback
        OSVR_ReturnCode update();

        /// Called when we get a new device from the SteamVR driver that we need
        /// to activate. Delegates the real work - this just displays
        /// information.
        std::pair<bool, std::uint32_t>
        activateDevice(vr::ITrackedDeviceServerDriver *dev);

        /// @name ServerDriverHost overrides
        /// @{
        void TrackedDevicePoseUpdated(uint32_t unWhichDevice,
                                      const DriverPose_t &newPose) override;

        void PhysicalIpdSet(uint32_t unWhichDevice,
                            float fPhysicalIpdMeters) override;

        void ProximitySensorState(uint32_t unWhichDevice,
                                  bool bProximitySensorTriggered) override;
/// @}

#if 0
        void DeviceDescriptorUpdated(std::string const &json);
#endif

      private:
        /// Does the real work of adding a new device.
        std::pair<bool, std::uint32_t>
        activateDeviceImpl(vr::ITrackedDeviceServerDriver *dev);

        /// Called from main thread only!
        void convertAndSendTracker(OSVR_TimeValue const &tv,
                                   OSVR_ChannelCount sensor,
                                   const DriverPose_t &newPose);

        osvr::pluginkit::DeviceToken m_dev;
        OSVR_TrackerDeviceInterface m_tracker;
        OSVR_AnalogDeviceInterface m_analog;

        std::unique_ptr<osvr::vive::DriverWrapper> m_vive;

        std::mutex m_mutex;
        TrackingDeque m_trackingReports;

        /// Current reports - main thread only
        TrackingVector m_currentTrackingReports;
    };
    using DriverHostPtr = std::unique_ptr<ViveDriverHost>;

} // namespace vive
} // namespace osvr
#endif // INCLUDED_OSVRViveTracker_h_GUID_BDA684D2_7F2D_4483_660D_C9D679BB1F67

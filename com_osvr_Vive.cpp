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
#include "OSVRViveTracker.h"
#include "PropertyHelper.h"
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/Util/PlatformConfig.h>

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

// Anonymous namespace to avoid symbol collision
namespace {

static const auto PREFIX = "[OSVR-Vive] ";

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
            osvr::vive::DriverHostPtr host(new osvr::vive::ViveDriverHost);

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

  private:
    std::vector<std::string> m_deviceTypes;
    /// This is the OSVR driver object, which also serves as the "SteamVR"
    /// driver host. We can only run one Vive at a time.
    osvr::vive::DriverHostPtr m_driverHost;
};
} // namespace

OSVR_PLUGIN(com_osvr_Vive) {
    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}

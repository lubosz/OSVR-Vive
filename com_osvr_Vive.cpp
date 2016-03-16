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

#include <osvr/Util/PlatformConfig.h>
#include "DriverLoader.h"
#include "FindDriver.h"
#include "GetProvider.h"
#include "ServerDriverHost.h"

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
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
#include "DriverWrapper.h"

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
#include <chrono>
#include <iostream>
#include <thread>

namespace osvr {
namespace vive {} // namespace vive
} // namespace osvr

int main() {
    auto vive = osvr::vive::DriverWrapper();

    /// These lines are just informational printout - the real check is later -
    /// if (!vive)
    if (vive.foundDriver()) {
        std::cout << "Found the Vive driver at " << vive.getDriverFileLocation()
                  << std::endl;
    }

    if (!vive.haveDriverLoaded()) {
        std::cout << "Could not open driver." << std::endl;
    }

    if (vive.foundConfigDirs()) {
        std::cout << "Driver config dir is: " << vive.getDriverConfigDir()
                  << std::endl;
    }

    if (!vive) {
        std::cerr << "Error in first-stage Vive driver startup. Exiting"
                  << std::endl;
        return 1;
    }

    if (!vive.isHMDPresent()) {
        std::cerr << "Driver loaded, but no Vive is connected. Exiting"
                  << std::endl;
        return 0;
    }

    std::cout << "*** Vive is connected." << std::endl;

    if (!vive.startServerDeviceProvider()) {
        // can either check return value of this, or do another if (!vive) after
        // calling - equivalent.
        std::cerr << "Error: could not start the server device provider in the "
                     "Vive driver. Exiting."
                  << std::endl;
        return 1;
    }

    /// but now, we can do things with vive.serverDevProvider()

    /// Power the system up.
    vive.serverDevProvider().LeaveStandby();
    auto numDevices = vive.serverDevProvider().GetTrackedDeviceCount();
    std::cout << "*** Got " << numDevices << " tracked devices" << std::endl;
    for (decltype(numDevices) i = 0; i < numDevices; ++i) {
        auto dev = vive.serverDevProvider().GetTrackedDeviceDriver(
            i, vr::ITrackedDeviceServerDriver_Version);
        dev->Activate(i);
        std::cout << "Device " << i << std::endl;
        {
            auto disp = osvr::vive::getComponent<vr::IVRDisplayComponent>(dev);
            if (disp) {
                std::cout << "-- it's a display, too!" << std::endl;
            }
        }

        {
            auto controller =
                osvr::vive::getComponent<vr::IVRControllerComponent>(dev);
            if (controller) {
                std::cout << "-- it's a controller, too!" << std::endl;
            }
        }

        {
            auto cam = osvr::vive::getComponent<vr::IVRCameraComponent>(dev);
            if (cam) {
                std::cout << "-- it's a camera, too!" << std::endl;
            }
        }
    }

    std::cout << "*** Entering dummy mainloop" << std::endl;
    for (int i = 0; i < 3000; ++i) {
        vive.serverDevProvider().RunFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    for (decltype(numDevices) i = 0; i < numDevices; ++i) {
        auto dev = vive.serverDevProvider().GetTrackedDeviceDriver(
            i, vr::ITrackedDeviceServerDriver_Version);
        dev->Deactivate();
    }
    std::cout << "*** Done with dummy mainloop" << std::endl;
    return 0;
}

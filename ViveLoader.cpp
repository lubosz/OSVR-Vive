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
#include "GetComponent.h"

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace osvr {
namespace vive {} // namespace vive
} // namespace osvr

static const auto PREFIX = "[ViveLoader] ";
static void whatIsThisDevice(vr::ITrackedDeviceServerDriver *dev) {
    {
        auto disp = osvr::vive::getComponent<vr::IVRDisplayComponent>(dev);
        if (disp) {
            std::cout << PREFIX << "-- it's a display, too!" << std::endl;
            vr::ETrackedPropertyError err;
            auto universe = dev->GetUint64TrackedDeviceProperty(
                vr::Prop_CurrentUniverseId_Uint64, &err);
            if (vr::TrackedProp_Success == err) {
                std::cout << PREFIX << " -- In tracking universe " << universe
                          << std::endl;
            } else if (vr::TrackedProp_NotYetAvailable == err) {
                std::cout << PREFIX << " -- Tracking universe not yet known"
                          << std::endl;
            } else {
                std::cout << PREFIX << " -- Some other error trying to figure "
                                       "out the tracking universe: "
                          << err << std::endl;
            }
        }
    }

    {
        auto controller =
            osvr::vive::getComponent<vr::IVRControllerComponent>(dev);
        if (controller) {
            std::cout << PREFIX << "-- it's a controller, too!" << std::endl;
        }
    }

    {
        auto cam = osvr::vive::getComponent<vr::IVRCameraComponent>(dev);
        if (cam) {
            std::cout << PREFIX << "-- it's a camera, too!" << std::endl;
        }
    }
    std::cout << "\n";
}

int main() {
    auto vive = osvr::vive::DriverWrapper();

    /// These lines are just informational printout - the real check is later -
    /// if (!vive)
    if (vive.foundDriver()) {
        std::cout << PREFIX << "Found the Vive driver at "
                  << vive.getDriverFileLocation() << std::endl;
    }

    if (!vive.haveDriverLoaded()) {
        std::cout << PREFIX << "Could not open driver." << std::endl;
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
        return 1;
    }

    if (!vive.isHMDPresent()) {
        std::cerr << PREFIX
                  << "Driver loaded, but no Vive is connected. Exiting"
                  << std::endl;
        return 0;
    }

    std::cout << PREFIX << "Vive is connected." << std::endl;

    if (!vive.startServerDeviceProvider()) {
        // can either check return value of this, or do another if (!vive) after
        // calling - equivalent.
        std::cerr << PREFIX
                  << "Error: could not start the server device provider in the "
                     "Vive driver. Exiting."
                  << std::endl;
        return 1;
    }

    /// but now, we can do things with vive.serverDevProvider()

    /// Power the system up.
    vive.serverDevProvider().LeaveStandby();

    std::vector<std::string> knownSerialNumbers;
    auto handleNewDevice = [&](const char *serialNum) {
        auto dev = vive.serverDevProvider().FindTrackedDeviceDriver(
            serialNum, vr::ITrackedDeviceServerDriver_Version);
        if (!dev) {
            std::cout << PREFIX
                      << "Couldn't find the corresponding device driver for "
                      << serialNum << std::endl;
            return false;
        }
        auto ret = vive.devices().addAndActivateDevice(dev);
        if (!ret.first) {
            std::cout << PREFIX << "Device with serial number " << serialNum
                      << " couldn't be added to the devices vector."
                      << std::endl;
            return false;
        }
        std::cout << "\n"
                  << PREFIX << "Device with s/n " << serialNum
                  << " activated, assigned ID " << ret.second << std::endl;
        whatIsThisDevice(dev);
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
            vive.devices().addAndActivateDevice(dev);
            std::cout << PREFIX << "Device " << i << std::endl;
            whatIsThisDevice(dev);
        }
    }

    std::cout << "*** Entering dummy mainloop" << std::endl;
    for (int i = 0; i < 200; ++i) {
        vive.serverDevProvider().RunFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    /// The vive object will automatically deactivate all of them.

    /// This line will turn off the wireless wands.
    // vive.serverDevProvider().EnterStandby();

    std::cout << "*** Done with dummy mainloop" << std::endl;
    return 0;
}

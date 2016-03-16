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
#include "PropertyHelper.h"

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
// - none

static const auto PREFIX = "[DisplayExtractor] ";

using namespace osvr::vive;

void dumpStringProp(vr::ITrackedDeviceServerDriver *dev, const char name[],
                    vr::ETrackedDeviceProperty prop) {
    auto propVal = getStringProperty(dev, prop);
    if (vr::TrackedProp_Success == propVal.second) {
        std::cout << name << ": '" << propVal.first << "'" << std::endl;
    } else {
        std::cout << name << ": '" << propVal.first << "' (got an error, code "
                  << propVal.second << ")" << std::endl;
    }
}

#define DUMP_STRING_PROP(DEV, PROPNAME)                                        \
    do {                                                                       \
        dumpStringProp(DEV, #PROPNAME, vr::PROPNAME);                          \
    } while (0)

void handleDisplay(vr::ITrackedDeviceServerDriver *dev,
                   vr::IVRDisplayComponent *display) {

#if 0
        auto prop = getStringProperty(dev, vr::Prop_TrackingSystemName_String);
        if (vr::TrackedProp_Success == prop.second) {
            std::cout << "Prop_TrackingSystemName_String: '" << prop.first
                      << "'" << std::endl;
        } else {
            std::cout << "Prop_TrackingSystemName_String: '" << prop.first
                      << "' (got an error, code " << prop.second << ")"
                      << std::endl;
        }
    }
#endif
    DUMP_STRING_PROP(dev, Prop_ManufacturerName_String);
    DUMP_STRING_PROP(dev, Prop_ModelNumber_String);
    DUMP_STRING_PROP(dev, Prop_SerialNumber_String);
    DUMP_STRING_PROP(dev, Prop_RenderModelName_String);
    DUMP_STRING_PROP(dev, Prop_TrackingSystemName_String);
    DUMP_STRING_PROP(dev, Prop_AttachedDeviceId_String);
    DUMP_STRING_PROP(dev, Prop_CameraFirmwareDescription_String);
    DUMP_STRING_PROP(dev, Prop_ModeLabel_String);
}

int main() {
    auto vive = osvr::vive::DriverWrapper();
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
    if (!vive.startServerDeviceProvider()) {
        // can either check return value of this, or do another if (!vive)
        // after
        // calling - equivalent.
        std::cerr << PREFIX
                  << "Error: could not start the server device provider in the "
                     "Vive driver. Exiting."
                  << std::endl;
        return 1;
    }

    /// Power the system up.
    vive.serverDevProvider().LeaveStandby();
    {
        auto numDevices = vive.serverDevProvider().GetTrackedDeviceCount();
        std::cout << PREFIX << "Got " << numDevices
                  << " tracked devices at startup" << std::endl;
        for (decltype(numDevices) i = 0; i < numDevices; ++i) {
            auto dev = vive.serverDevProvider().GetTrackedDeviceDriver(
                i, vr::ITrackedDeviceServerDriver_Version);
            vive.addAndActivateDevice(dev);
            std::cout << PREFIX << "Device " << i << std::endl;
            auto disp = osvr::vive::getComponent<vr::IVRDisplayComponent>(dev);
            if (disp) {
                std::cout << PREFIX << "-- it's a display, too!" << std::endl;
                handleDisplay(dev, disp);
            }
        }
    }
    return 0;
}

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

#define _USE_MATH_DEFINES

// Internal Includes
#include "DisplayDescriptor.h"
#include "DriverWrapper.h"
#include "PropertyHelper.h"

#include "viveDisplayInput.h"

// Library/third-party includes
#include <openvr_driver.h>
#include <osvr/Util/StringLiteralFileToString.h>

// Standard includes
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <tuple>

static const auto PREFIX = "[DisplayExtractor] ";

using namespace osvr::vive;

std::unique_ptr<DisplayDescriptor> g_descriptor;
bool g_gotDisplay = false;

inline UnitClippingPlane getClippingPlanes(vr::IVRDisplayComponent *display,
                                           vr::EVREye eye) {
    UnitClippingPlane ret;
    /// Note that work with the steamvr-osvr driver has indicated that SteamVR
    /// apparently flips the top and bottom clipping plane location?
    display->GetProjectionRaw(eye, &ret.left, &ret.right, &ret.bottom,
                              &ret.top);
    return ret;
}

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

    g_gotDisplay = true;
    /// Verify/check the resolution
    {
        int32_t x, y;
        uint32_t width, height;
        display->GetWindowBounds(&x, &y, &width, &height);
        g_descriptor->setResolution(width, height);
    }

    /// Get the two eye/lens center of projections.
    using CenterOfProjectionIndices =
        std::tuple<std::size_t, vr::ETrackedDeviceProperty,
                   vr::ETrackedDeviceProperty>;
    for (auto &data :
         {CenterOfProjectionIndices{0, vr::Prop_LensCenterLeftU_Float,
                                    vr::Prop_LensCenterLeftV_Float},
          CenterOfProjectionIndices{1, vr::Prop_LensCenterRightU_Float,
                                    vr::Prop_LensCenterRightV_Float}}) {
        using std::get;
        ETrackedPropertyError err;
        auto x = dev->GetFloatTrackedDeviceProperty(get<1>(data), &err);
        auto y = dev->GetFloatTrackedDeviceProperty(get<2>(data), &err);

        g_descriptor->updateCenterOfProjection(get<0>(data), {{x, y}});
    }
    {
        /// Back-calculate the display parameters based on the projection
        /// clipping planes.
        auto leftClip = getClippingPlanes(display, vr::Eye_Left);
        auto rightClip = getClippingPlanes(display, vr::Eye_Right);
        std::cout << PREFIX << "leftClip: " << leftClip << std::endl;
        std::cout << PREFIX << "rightClip: " << rightClip << std::endl;

        auto leftFovs = clipPlanesToHalfFovs(leftClip);
        auto rightFovs = clipPlanesToHalfFovs(rightClip);

        auto fovsResult = twoEyeFovsToMonoWithOverlap(leftFovs, rightFovs);
        if (fovsResult.first) {
            // we successfully computed the conversion!
            g_descriptor->updateFovs(fovsResult.second);
        } else {
            // Couldn't compute the conversion - must not be symmetrical enough
            // at the moment.
            std::cout << PREFIX << "Will attempt to symmetrize and re-convert "
                                   "an approximation of the field of view."
                      << std::endl;
            averageAndSymmetrize(leftFovs, rightFovs);
            fovsResult = twoEyeFovsToMonoWithOverlap(leftFovs, rightFovs);
            if (fovsResult.first) {
                // we successfully computed the conversion (of an
                // approximation)!
                g_descriptor->updateFovs(fovsResult.second);

            } else {
                std::cout << PREFIX << "Still failed to produce a conversion."
                          << std::endl;
                g_gotDisplay = false;
            }
        }
    }
}

int main() {
    {
        g_descriptor.reset(
            new DisplayDescriptor(osvr::util::makeString(viveDisplayInput)));
        if (!(*g_descriptor)) {
            std::cerr << PREFIX
                      << "Could not parse template for display descriptor."
                      << std::endl;
            return 1;
        }
    }
    {
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
            // after calling - equivalent.
            std::cerr
                << PREFIX
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
                auto disp =
                    osvr::vive::getComponent<vr::IVRDisplayComponent>(dev);
                if (disp) {
                    std::cout << PREFIX << "-- it's a display, too!"
                              << std::endl;
                    handleDisplay(dev, disp);
                    break;
                }
            }
        }
        vive.stop();
        if (vive.serverDevProvider().ShouldBlockStandbyMode()) {
            std::cout << PREFIX << "Driver is reporting that it is busy and "
                                   "should block standby mode, so we will wait "
                                   "until it is finished to exit..."
                      << std::endl;
            do {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            } while (vive.serverDevProvider().ShouldBlockStandbyMode());
        }

        if (g_gotDisplay) {
            std::cout << "\n\n"
                      << g_descriptor->getDescriptor() << std::endl;
        }
        std::cout << PREFIX << "Press enter to quit..." << std::endl;
        std::cin.ignore();
    }

    return 0;
}

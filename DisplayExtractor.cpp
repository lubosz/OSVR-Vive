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
#include "DriverWrapper.h"
#include "PropertyHelper.h"

#include "viveDisplayInput.h"

// Library/third-party includes
#include <json/reader.h>
#include <json/writer.h>
#include <openvr_driver.h>
#include <osvr/Util/StringLiteralFileToString.h>

// Standard includes
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <tuple>

static const auto PREFIX = "[DisplayExtractor] ";

Json::Value g_displayDescriptor;
bool g_gotDisplay = false;

using namespace osvr::vive;

struct ClippingPlane;
struct Degrees;
template <typename Tag> struct Rect {
    float left;
    float right;
    float top;
    float bottom;
};

struct Fovs {
    double monoHoriz;
    double monoVert;
    double overlapPercent;
    double pitch = 0;
};

template <typename T> inline T radiansToDegrees(T const &rads) {
    return static_cast<T>(rads * 180 / M_PI /*3.141592653589*/);
}

inline float clipPlaneToHalfFov(float d) {
    return radiansToDegrees(std::atan(std::abs(d)));
}

/// Given clipping planes at unit distance, return the half-field of views in
/// degrees.
inline Rect<Degrees> clipPlanesToHalfFovs(Rect<ClippingPlane> const &r) {
    Rect<Degrees> ret;
    ret.left = clipPlaneToHalfFov(r.left);
    ret.right = clipPlaneToHalfFov(r.right);
    ret.top = clipPlaneToHalfFov(r.top);
    ret.bottom = clipPlaneToHalfFov(r.bottom);
    return ret;
}

inline bool approxEqual(float a, float b, float maxDiff,
                        float maxRelDiff = FLT_EPSILON) {
    /// Inspired by Bruce Dawson's AlmostEqualRelativeAndAbs
    /// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
    auto diff = std::abs(a - b);
    if (diff <= maxDiff) {
        return true;
    }
    auto larger = std::max(std::abs(a), std::abs(b));
    return diff <= larger * maxRelDiff;
}

inline Fovs twoEyeFovsToMonoWithOverlap(Rect<Degrees> const &leftFov,
                                        Rect<Degrees> const &rightFov) {
    Fovs ret;

    /// Lambda for spitting an error message, negating the validity of the
    /// display descriptor we've been working on, and returning the partial
    /// answer we had, for more concise error handling.
    auto withError = [&](const char *msg) {
        std::cerr << PREFIX << "***ERROR: Eyes are " << msg
                  << " - cannot be represented in this display schema!"
                  << std::endl;
        g_gotDisplay = false;
        return ret;
    };

    // lambda sets a max absolute difference for angles in degrees to be
    // considered equal in this entire function.
    auto notEqual = [](float a, float b) { return !approxEqual(a, b, 0.25); };

    {
        std::cout << PREFIX << "left: ";
        auto &fov = leftFov;
        std::cout << fov.left << " " << fov.right << " " << fov.top << " "
                  << fov.bottom << std::endl;
    }
    {
        std::cout << PREFIX << "right: ";
        auto &fov = rightFov;
        std::cout << fov.left << " " << fov.right << " " << fov.top << " "
                  << fov.bottom << std::endl;
    }

    if (notEqual(leftFov.left, rightFov.right) ||
        notEqual(rightFov.left, leftFov.right)) {
        return withError("not symmetrical in their frusta across the YZ plane");
    }

    ret.monoHoriz = leftFov.left + leftFov.right;
    if (notEqual(ret.monoHoriz, (rightFov.left + rightFov.right))) {
        return withError("not matching in horizontal field of view");
    }
    if (notEqual(leftFov.top, rightFov.top) ||
        notEqual(leftFov.bottom, rightFov.bottom)) {
        return withError(
            "not symmetrical in their vertical half fields of view");
    }
    ret.monoVert = leftFov.top + leftFov.bottom;
#if 0
    // check is redundant
    if (ret.monoVert != (rightFov.top + rightFov.bottom)) {
        return withError("not matching in vertical field of view");
    }
#endif

    {
        /// Related to the code in helper.cpp in angles_to_config.
        auto horizHalfFov = ret.monoHoriz / 2.;
        auto rotateEyesApart = leftFov.left - horizHalfFov;
        auto overlapFrac = 1. - 2. * rotateEyesApart / ret.monoHoriz;
        ret.overlapPercent = overlapFrac * 100;
    }
    ret.pitch = leftFov.bottom - (ret.monoVert / 2.);
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

inline Rect<ClippingPlane> getClippingPlanes(vr::IVRDisplayComponent *display,
                                             vr::EVREye eye) {
    Rect<ClippingPlane> ret;
    /// Note that work with the steamvr-osvr driver has indicated that SteamVR
    /// apparently flips the top and bottom clipping plane location?
    display->GetProjectionRaw(eye, &ret.left, &ret.right, &ret.bottom,
                              &ret.top);
    return ret;
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
    Json::Value &hmd = g_displayDescriptor["hmd"];
    {
        int32_t x, y;
        uint32_t width, height;
        display->GetWindowBounds(&x, &y, &width, &height);
        hmd["resolutions"][0]["width"] = width;
        hmd["resolutions"][0]["height"] = height;
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
        Json::Value &eye = hmd["eyes"][get<0>(data)];
        ETrackedPropertyError err;
        eye["center_proj_x"] =
            dev->GetFloatTrackedDeviceProperty(get<1>(data), &err);
        eye["center_proj_y"] =
            dev->GetFloatTrackedDeviceProperty(get<2>(data), &err);
    }
    {
        /// Back-calculate the display parameters based on the projection
        /// clipping planes.
        Rect<ClippingPlane> leftClip = getClippingPlanes(display, vr::Eye_Left);
        Rect<ClippingPlane> rightClip =
            getClippingPlanes(display, vr::Eye_Right);

        {
            std::cout << PREFIX << "leftClip: ";
            auto &fov = leftClip;
            std::cout << fov.left << " " << fov.right << " " << fov.top << " "
                      << fov.bottom << std::endl;
        }
        {
            std::cout << PREFIX << "rightClip: ";
            auto &fov = rightClip;
            std::cout << fov.left << " " << fov.right << " " << fov.top << " "
                      << fov.bottom << std::endl;
        }
        auto fovs = twoEyeFovsToMonoWithOverlap(
            clipPlanesToHalfFovs(leftClip), clipPlanesToHalfFovs(rightClip));

        Json::Value &fov = hmd["field_of_view"];
        fov["monocular_horizontal"] = fovs.monoHoriz;
        fov["monocular_vertical"] = fovs.monoVert;
        fov["overlap_percent"] = fovs.overlapPercent;
        fov["pitch_tilt"] = fovs.pitch;
    }
}

int main() {
    {
        /// Load the starting point for the display descriptor.
        Json::Reader reader;
        if (!reader.parse(osvr::util::makeString(viveDisplayInput),
                          g_displayDescriptor)) {
            std::cerr
                << PREFIX
                << "Could not parse template for display descriptor. Error: "
                << reader.getFormattedErrorMessages() << std::endl;
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
                      << g_displayDescriptor.toStyledString() << std::endl;
        }
        std::cout << PREFIX << "Press enter to quit..." << std::endl;
        std::cin.ignore();
    }

    return 0;
}

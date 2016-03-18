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
#include "RGBPoints.h"

#include "viveDisplayInput.h"

// Library/third-party includes
#include <openvr_driver.h>
#include <osvr/Util/StringLiteralFileToString.h>

// Standard includes
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>
#include <tuple>

/// the number of steps in each dimension
static const auto MESH_STEPS = 15;

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

void updateCenterOfProjection(DisplayDescriptor &descriptor,
                              vr::ITrackedDeviceServerDriver *dev) {
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
}

bool updateFOV(DisplayDescriptor &descriptor,
               vr::IVRDisplayComponent *display) {
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
        descriptor.updateFovs(fovsResult.second);
        return true;
    }

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
        descriptor.updateFovs(fovsResult.second);
        return true;
    }
    std::cout << PREFIX << "Still failed to produce a conversion." << std::endl;
    return false;
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

static size_t g_approxEqual = 0;
static size_t g_approxNe = 0;

inline void addMeshPoint(vr::IVRDisplayComponent *display, RGBPoints &mesh,
                         const float u, const float v) {
    static const float maxdiff = 0.001;
    for (std::size_t eye = 0; eye < 2; ++eye) {
        auto steamEye = eye == 0 ? vr::Eye_Left : vr::Eye_Right;
        auto meshEye = eye == 0 ? RGBPoints::Eye::Left : RGBPoints::Eye::Right;

        auto ret = display->ComputeDistortion(steamEye, u, v);

        if (approxEqual(ret.rfRed[0], ret.rfGreen[0], maxdiff) &&
            approxEqual(ret.rfRed[0], ret.rfBlue[0], maxdiff) &&
            approxEqual(ret.rfRed[1], ret.rfGreen[1], maxdiff) &&
            approxEqual(ret.rfRed[1], ret.rfBlue[1], maxdiff)) {
            g_approxEqual++;
        } else {
            g_approxNe++;
        }
        mesh.addSample(meshEye, {{u, v}}, {{ret.rfRed[0], ret.rfRed[1]}},
                       {{ret.rfGreen[0], ret.rfGreen[1]}},
                       {{ret.rfBlue[0], ret.rfBlue[1]}});
    }
}

std::string generateMeshFileContents(vr::IVRDisplayComponent *display,
                                     const std::size_t steps = 3) {
    RGBPoints mesh;
    const auto realSteps = steps - 1;
    const float stepSize = 1.f / static_cast<float>(steps);
    for (std::size_t uInt = 0; uInt < realSteps; ++uInt) {
        const auto u = uInt * stepSize;
        for (std::size_t vInt = 0; vInt < realSteps; ++vInt) {
            const auto v = vInt * stepSize;
            addMeshPoint(display, mesh, u, v);
        }
        // make sure we get v = 1.
        addMeshPoint(display, mesh, u, 1);
    }
    // and get (1, 1) as well.
    addMeshPoint(display, mesh, 1, 1);
#if 0
    std::cout << PREFIX << "MESH: " << mesh.getSeparateFileStyled()
              << std::endl;
#endif

    std::cout << PREFIX << "MESH COLOR MATCHING: " << g_approxEqual
              << " approx equal, " << g_approxNe << " not ("
              << (g_approxEqual / (g_approxEqual + g_approxNe) * 100) << "%)"
              << std::endl;
    return mesh.getSeparateFile();
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

    updateCenterOfProjection(*g_descriptor, dev);

    if (!updateFOV(*g_descriptor, display)) {
        g_gotDisplay = false;
        return;
    }

    auto meshContents = generateMeshFileContents(display, MESH_STEPS);
    static const auto MeshFilename = "ViveMesh.json";
    {
        std::ofstream os(MeshFilename);
        os << meshContents << std::flush;
        os.close();
    }
    g_descriptor->setRGBMeshExternalFile(MeshFilename);

    static const auto DisplayConfig = "ViveDisplay.json";
    {
        std::ofstream os(DisplayConfig);
        os << g_descriptor->getDescriptor() << std::flush;
        os.close();
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
                vive.devices().addAndActivateDevice(dev);
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
            std::cout << "\n\n" << g_descriptor->getDescriptor() << std::endl;
        }
        std::cout << PREFIX << "Press enter to quit..." << std::endl;
        std::cin.ignore();
    }

    return 0;
}

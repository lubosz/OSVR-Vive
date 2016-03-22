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

#ifndef INCLUDED_DisplayDescriptor_h_GUID_9CB6F1CE_30FC_4497_7073_7C1914707BFB
#define INCLUDED_DisplayDescriptor_h_GUID_9CB6F1CE_30FC_4497_7073_7C1914707BFB

// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace osvr {
namespace vive {
    struct ClippingPlaneTag;
    struct FoVDegreesTag;
    /// Structure, with tag type for safety, storing top left bottom right with
    /// some tag-defined meaning.
    template <typename Tag, typename Scalar = float> struct Rect {
        Scalar left;
        Scalar right;
        Scalar top;
        Scalar bottom;
    };

    using UnitClippingPlane = Rect<ClippingPlaneTag>;
    using HalfFieldsOfViewDegrees = Rect<FoVDegreesTag>;

    /// Output operator for any kind of rect
    template <typename OutputStream, typename Tag, typename Scalar>
    inline OutputStream &operator<<(OutputStream &os,
                                    Rect<Tag, Scalar> const &r) {
        os << r.left << " " << r.right << " " << r.top << " " << r.bottom;
        return os;
    }

    /// Given clipping planes at unit distance, return the half-fields of view
    /// in degrees.
    HalfFieldsOfViewDegrees clipPlanesToHalfFovs(UnitClippingPlane const &r);

    /// Structure holding the display specifications as used in the version 1
    /// schemas for OSVR.
    struct Fovs {
        double monoHoriz;
        double monoVert;
        double overlapPercent;
        double pitch = 0;
    };

    /// Given the half-fields of view in degrees for two eyes, convert to the
    /// monocular FoV, overlap, and pitch structure if possible. Returns a
    /// (success, result) pair.
    std::pair<bool, Fovs>
    twoEyeFovsToMonoWithOverlap(HalfFieldsOfViewDegrees const &leftFov,
                                HalfFieldsOfViewDegrees const &rightFov,
                                const bool verbose = false);

    /// As the current display model can't handle a variety of non-matching or
    /// asymmetrical displays, the twoEyeFovsToMonoWithOverlap function can fail
    /// in many ways. If you just need something approximate, this function will
    /// modify the input for that function so that it passes the checks, though
    /// this will result in a reduced quality of display.
    void averageAndSymmetrize(HalfFieldsOfViewDegrees &leftFov,
                              HalfFieldsOfViewDegrees &rightFov);

    class DisplayDescriptor {
      public:
        explicit DisplayDescriptor(
            std::string const &templateDisplayDescriptor);
        ~DisplayDescriptor();

        /// Is the display descriptor valid?
        explicit operator bool() const { return valid_; }

        void updateFovs(Fovs const &fovs);

        enum { LeftEye = 0, RightEye = 1 };

        void updateCenterOfProjection(std::size_t eyeIndex,
                                      std::array<float, 2> const &center);

        void updateCentersOfProjection(std::array<float, 2> const &left,
                                       std::array<float, 2> const &right);

        void setResolution(std::uint32_t width, std::uint32_t height);

        void setRGBMeshExternalFile(std::string const &fn);

        /// Get the descriptor.
        std::string getDescriptor() const;

      private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        bool valid_ = false;
    };

} // namespace vive
} // namespace osvr

#endif // INCLUDED_DisplayDescriptor_h_GUID_9CB6F1CE_30FC_4497_7073_7C1914707BFB

/** @file
    @brief Header

    @date 2016

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2016 Razer Inc.
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

#ifndef INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3
#define INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3

// Internal Includes
// - none

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
#include <assert.h>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace osvr {
namespace vive {
    std::pair<std::string, vr::ETrackedPropertyError> inline getStringProperty(
        vr::ITrackedDeviceServerDriver *dev, vr::ETrackedDeviceProperty prop) {
        assert(dev != nullptr &&
               "Tried to get a string property from a null device "
               "pointer.");
        static const auto INITIAL_BUFFER_SIZE = vr::k_unTrackingStringSize;
        /// Start with a buffer of k_unTrackingStringSize as suggested.
        std::vector<char> buf(INITIAL_BUFFER_SIZE, '\0');
        vr::ETrackedPropertyError err;
        auto ret = dev->GetStringTrackedDeviceProperty(
            prop, buf.data(), static_cast<uint32_t>(buf.size()), &err);
        if (0 == ret) {
            // property not available
            return std::make_pair(std::string{}, err);
        }

        if (ret > buf.size()) {
            std::cout << "[getStringProperty] Got an initial return value "
                         "larger than the buffer size: ret = "
                      << ret << ", buf.size() = " << buf.size() << std::endl;
        }
        if (vr::TrackedProp_BufferTooSmall == err) {
            // first buffer was too small, but now we know how big it should be,
            // per the docs.
            /// @todo remove this debug print
            std::cout << "[getStringProperty] Initial buffer size: "
                      << buf.size() << ", return value: " << ret << std::endl;
            buf.resize(ret + 1, '\0');
            ret = dev->GetStringTrackedDeviceProperty(
                prop, buf.data(), static_cast<uint32_t>(buf.size()), &err);
        }

        if (ret > buf.size()) {
            std::cout << "[getStringProperty] THIS SHOULDN'T HAPPEN: Got a "
                         "return value larger than the buffer size: ret = "
                      << ret << ", buf.size() = " << buf.size() << std::endl;

            return std::make_pair(std::string{}, err);
        }
        return std::make_pair(std::string{buf.data()}, err);
    }

} // namespace vive
} // namespace osvr

#endif // INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3

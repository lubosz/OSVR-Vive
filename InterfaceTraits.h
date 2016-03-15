/** @file
    @brief Header associating the interface name and version strings with the
   types for safe requesting and casting.

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

#ifndef INCLUDED_InterfaceTraits_h_GUID_36EC6818_80C9_4EA1_D86F_517E153574D0
#define INCLUDED_InterfaceTraits_h_GUID_36EC6818_80C9_4EA1_D86F_517E153574D0

// Internal Includes
// - none

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
// - none

namespace osvr {
namespace vive {
    template <typename InterfaceType> struct InterfaceNameTrait;

    template <> struct InterfaceNameTrait<vr::IClientTrackedDeviceProvider> {
        static const char *get() {
            return vr::IClientTrackedDeviceProvider_Version;
        }
    };
    template <> struct InterfaceNameTrait<vr::IServerTrackedDeviceProvider> {
        static const char *get() {
            return vr::IServerTrackedDeviceProvider_Version;
        }
    };

} // namespace vive
} // namespace osvr
#endif // INCLUDED_InterfaceTraits_h_GUID_36EC6818_80C9_4EA1_D86F_517E153574D0

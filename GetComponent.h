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

#ifndef INCLUDED_GetComponent_h_GUID_39A46167_7C88_4FB2_A4C4_EE955814C727
#define INCLUDED_GetComponent_h_GUID_39A46167_7C88_4FB2_A4C4_EE955814C727

// Internal Includes
#include "InterfaceTraits.h"

// Library/third-party includes
// - none

// Standard includes
// - none

namespace osvr {
namespace vive {
    /// Helper function for easy typesafe "query interface"-type accessing of
    /// tracked server driver components
    template <typename InterfaceType>
    inline InterfaceType *getComponent(vr::ITrackedDeviceServerDriver *driver) {
        static_assert(
            InterfaceExpectedFromGetComponent<InterfaceType>::value,
            "Can only call getComponent with interface types expected to be "
            "used with getComponent!");
        if (!driver) {
            return nullptr;
        }
        void *initialRet =
            driver->GetComponent(InterfaceNameTrait<InterfaceType>::get());
        if (!initialRet) {
            return nullptr;
        }
        return static_cast<InterfaceType *>(initialRet);
    }

} // namespace vive
} // namespace osvr

#endif // INCLUDED_GetComponent_h_GUID_39A46167_7C88_4FB2_A4C4_EE955814C727

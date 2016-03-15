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
#include "DriverLoader.h"
#include "InterfaceTraits.h"
#include "SearchPathExtender.h"

// Library/third-party includes
// - none

// Standard includes
#include <iostream>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace osvr {
namespace vive {

    static const auto ENTRY_POINT_FUNCTION_NAME = "HmdDriverFactory";

    struct DriverLoader::Impl {
#if defined(OSVR_WINDOWS)
        HMODULE driver_;
#else
#endif

    };
    DriverLoader::DriverLoader(std::string const & driverRoot, std::string const & driverFile) : impl_(new Impl) {
        /// Set the PATH to include the driver directory so it can
        /// find its deps.
        SearchPathExtender extender(driverRoot);
#if defined(OSVR_WINDOWS)
        impl_->driver_ = LoadLibraryA(driverFile.c_str());
        /// @todo check for NULL
        factory_ = reinterpret_cast<DriverFactory>(
            GetProcAddress(impl_->driver_, ENTRY_POINT_FUNCTION_NAME));
#else
#endif
        auto ret = invokeFactory<vr::IClientTrackedDeviceProvider>();
        if (ret.first) {
            std::cout
                << "Successfully got the IClientTrackedDeviceProvider!";
            auto clientProvider = ret.first;
            auto isPresent = clientProvider->BIsHmdPresent(".");
            std::cout << " is present? " << std::boolalpha << isPresent
                << std::endl;
        }
        else {
            std::cout << "Couldn't get it, error code " << ret.second
                << std::endl;
        }
    }
    DriverLoader::~DriverLoader() {
#if defined(OSVR_WINDOWS)
        FreeLibrary(impl_->driver_);
#else
#endif
    }
} // namespace vive
} // namespace osvr

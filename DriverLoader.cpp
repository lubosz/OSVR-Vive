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
        HMODULE driver_ = nullptr;
#else
#endif
        ~Impl() {

#if defined(OSVR_WINDOWS)
            if (driver_) {
                FreeLibrary(driver_);
            }
#else
#endif
        }
    };
    DriverLoader::DriverLoader(std::string const &driverRoot,
                               std::string const &driverFile)
        : impl_(new Impl) {
        /// Set the PATH to include the driver directory so it can
        /// find its deps.
        SearchPathExtender extender(driverRoot);
#if defined(OSVR_WINDOWS)
        impl_->driver_ = LoadLibraryA(driverFile.c_str());
        if (!impl_->driver_) {
            impl_.reset();
            throw CouldNotLoadDriverModule();
        }

        auto proc = GetProcAddress(impl_->driver_, ENTRY_POINT_FUNCTION_NAME);
        if (!proc) {
            impl_.reset();
            throw CouldNotLoadEntryPoint();
        }
        factory_ = reinterpret_cast<DriverFactory>(proc);
#else
#endif
    }

    std::unique_ptr<DriverLoader>
    DriverLoader::make(std::string const &driverRoot,
                       std::string const &driverFile) {
        std::unique_ptr<DriverLoader> ret(
            new DriverLoader(driverRoot, driverFile));
        return ret;
    }

    DriverLoader::~DriverLoader() {}

    DriverLoader::operator bool() const {
        /// Presence of a valid private impl object is equivalent to validity of
        /// the object overall.
        return static_cast<bool>(impl_);
    }

    bool DriverLoader::isHMDPresent(std::string const &userConfigDir) const {
        auto ret = getInterface<vr::IClientTrackedDeviceProvider>();
        if (ret.first) {
            std::cout << "Successfully got the IClientTrackedDeviceProvider!";
            auto clientProvider = ret.first;
            auto isPresent =
                clientProvider->BIsHmdPresent(userConfigDir.c_str());
            std::cout << " is present? " << std::boolalpha << isPresent
                      << std::endl;
            return isPresent;
        }
        std::cout << "Couldn't get it, error code " << ret.second << std::endl;
        return false;
    }

    void DriverLoader::reset() { impl_.reset(); }
} // namespace vive
} // namespace osvr

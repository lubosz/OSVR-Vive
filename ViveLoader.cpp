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
#include "FindDriver.h"
#include "InterfaceTraits.h"
#include "SearchPathExtender.h"

// Library/third-party includes
#include <openvr_driver.h>
#include <osvr/Util/PlatformConfig.h>

// Standard includes
#include <cstdlib>
#include <iostream>
#include <vector>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace osvr {
namespace vive {

    static const auto ENTRY_POINT_FUNCTION_NAME = "HmdDriverFactory";
    class ViveLoader {
      public:
        ViveLoader(std::string const &driverRoot,
                   std::string const &driverFile) {
            /// Set the PATH to include the driver directory so it can
            /// find its deps.
            SearchPathExtender extender(driverRoot);
#if defined(OSVR_WINDOWS)
            driver_ = LoadLibraryA(driverFile.c_str());
            /// @todo check for NULL
            factory_ = reinterpret_cast<DriverFactory>(
                GetProcAddress(driver_, ENTRY_POINT_FUNCTION_NAME));
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
            } else {
                std::cout << "Couldn't get it, error code " << ret.second
                          << std::endl;
            }
        }

        /// Template function to call the factory with the right string and do
        /// the right casting. Returns the pointer and the error code in a pair.
        template <typename InterfaceType>
        std::pair<InterfaceType *, int> invokeFactory() {
            InterfaceType *ret = nullptr;
            int returnCode = 0;
            void *product =
                factory_(InterfaceNameTrait<InterfaceType>::get(), &returnCode);
            if (product) {
                ret = static_cast<InterfaceType *>(product);
            }
            return std::make_pair(ret, returnCode);
        }

        ~ViveLoader() {
#if defined(OSVR_WINDOWS)
            FreeLibrary(driver_);
#else
#endif
        }

      private:
        using DriverFactory = void *(*)(const char *, int *);
// typedef void *(DriverFactory)(const char *, int *);
#if defined(OSVR_WINDOWS)
        HMODULE driver_;
#else
#endif
        DriverFactory factory_;
    };

} // namespace vive
} // namespace osvr

int main() {
    auto driverLocation = osvr::vive::findDriver();
    if (driverLocation.found) {
        std::cout << "Found the Vive driver at " << driverLocation.driverFile
                  << std::endl;
    } else {
        std::cout << "Could not find the native SteamVR Vive driver, exiting!"
                  << std::endl;
        return 1;
    }

    osvr::vive::ViveLoader vive(driverLocation.driverRoot,
                                driverLocation.driverFile);
    return 0;
}

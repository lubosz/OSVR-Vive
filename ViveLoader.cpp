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

// Library/third-party includes
#include <openvr_driver.h>
#include <osvr/Util/PlatformConfig.h>

// Standard includes
#include <iostream>
#include <limits.h>
#include <string>
#include <vector>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if _MSC_VER >= 1800
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

#if defined(OSVR_WINDOWS)
static const auto DRIVER_DIRNAME_BASE = "win";
static const auto DRIVER_EXTENSION = ".dll";
#elif defined(OSVR_MACOSX)
/// @todo Note that there are no 64-bit steamvr runtimes or drivers on OS X
static const auto DRIVER_DIRNAME_BASE = "osx";
#elif defined(OSVR_LINUX)
static const auto DRIVER_DIRNAME_BASE = "linux";
#else
#error "Sorry, Valve does not produce a SteamVR runtime for your platform."
#endif

static const auto DRIVER_NAME = "lighthouse";

inline std::string getDriverDirname() {
    return DRIVER_DIRNAME_BASE + std::to_string(sizeof(void *) * CHAR_BIT);
}

namespace osvr {
namespace vive {

#if _MSC_VER == 1800 || _MSC_VER == 1900
    using std::tr2::sys::path;
    using std::tr2::sys::exists;
#else
    using boost::filesystem::path;
    using boost::filesystem::exists;
#endif

#if defined(OSVR_WINDOWS)

    inline std::vector<path> getSteamVRRoots() {
        /// @todo make this better - there's Windows API for that.
        return std::vector<path>{
            path{"C:\\Program Files\\Steam\\steamapps\\common\\SteamVR"},
            path{"C:\\Program "
                 "Files (x86)\\Steam\\steamapps\\common\\SteamVR"}};
    }
#elif defined(OSVR_MACOSX)
    inline std::vector<path> getSteamVRRoots() {
#error "implementation not complete"
    }
#elif defined(OSVR_LINUX)

    inline std::vector<path> getSteamVRRoots() {
#error "implementation not complete"
    }
#endif

    inline path getDriverFilePath(path const &root) {
        auto p = root;
        p /= "drivers";
        p /= DRIVER_NAME;
        p /= "bin";
        p /= getDriverDirname();
        p /= (std::string("driver_") + DRIVER_NAME + DRIVER_EXTENSION);
        return p;
    }

    inline path getDriverFilePath() {
        for (auto &root : getSteamVRRoots()) {
            auto driverFile = getDriverFilePath(root);
            if (exists(driverFile)) {
                return driverFile;
            }
        }
        return path{};
    }

    std::string tryLoading() {
        auto driverFile = getDriverFilePath();
        std::cout << "Will try to load driver from:\n"
                  << driverFile.string() << std::endl;
        if (exists(driverFile)) {
            std::cout << "It exists!" << std::endl;
            return driverFile.string();
        }
        std::cout << "It doesn't exist." << std::endl;
        return std::string();
    }

    static const auto ENTRY_POINT_FUNCTION_NAME = "HmdDriverFactory";
    class ViveLoader {
      public:
        ViveLoader(std::string const &driverFile) {
#if defined(OSVR_WINDOWS)
            driver_ = LoadLibraryA(driverFile.c_str());
            /// @todo check for NULL
            factory_ = reinterpret_cast<DriverFactory>(
                GetProcAddress(driver_, ENTRY_POINT_FUNCTION_NAME));
#else
#endif
            int returnCode = 0;
            void *ret =
                factory_(vr::IClientTrackedDeviceProvider_Version, &returnCode);
            if (ret) {
                std::cout
                    << "Successfully got the IClientTrackedDeviceProvider!";
                auto clientProvider =
                    static_cast<vr::IClientTrackedDeviceProvider *>(ret);
                auto isPresent = clientProvider->BIsHmdPresent(".");
                std::cout << " is present? " << std::boolalpha << isPresent
                          << std::endl;
            } else {
                std::cout << "Couldn't get it, error code " << returnCode
                          << std::endl;
            }
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
    auto driverLocation = osvr::vive::tryLoading();
    osvr::vive::ViveLoader vive(driverLocation);
    return 0;
}

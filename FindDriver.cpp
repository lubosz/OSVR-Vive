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

// Library/third-party includes
#include <osvr/Util/PlatformConfig.h>

// Standard includes
#include <iostream>
#include <limits.h>
#include <vector>

#if _MSC_VER >= 1800
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

#if defined(OSVR_WINDOWS)
static const auto PLATFORM_DIRNAME_BASE = "win";
static const auto DRIVER_EXTENSION = ".dll";
static const auto TOOL_EXTENSION = ".exe";
#elif defined(OSVR_MACOSX)
/// @todo Note that there are no 64-bit steamvr runtimes or drivers on OS X
static const auto PLATFORM_DIRNAME_BASE = "osx";
static const auto TOOL_EXTENSION = "";
#elif defined(OSVR_LINUX)
static const auto PLATFORM_DIRNAME_BASE = "linux";
static const auto DRIVER_EXTENSION = ".so";
static const auto TOOL_EXTENSION = "";
#else
#error "Sorry, Valve does not produce a SteamVR runtime for your platform."
#endif

namespace osvr {
namespace vive {

#if _MSC_VER == 1800 || _MSC_VER == 1900
    using std::tr2::sys::path;
    using std::tr2::sys::exists;
#else
    using boost::filesystem::path;
    using boost::filesystem::exists;
#endif

    inline std::string getPlatformDirname() {
        return PLATFORM_DIRNAME_BASE +
               std::to_string(sizeof(void *) * CHAR_BIT);
    }
#if defined(OSVR_WINDOWS)

    inline std::vector<std::string> getSteamVRRoots() {
        /// @todo make this better - there's Windows API for that.
        return std::vector<std::string>{
            std::string{"C:\\Program Files\\Steam\\steamapps\\common\\SteamVR"},
            std::string{
                "C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR"}};
    }
#elif defined(OSVR_MACOSX)
    inline std::vector<std::string> getSteamVRRoots() {
#error "implementation not complete"
    }
#elif defined(OSVR_LINUX)

    inline std::vector<std::string> getSteamVRRoots() {
#error "implementation not complete"
    }
#endif

    inline void computeDriverRootAndFilePath(DriverLocationInfo &info,
                                             std::string const &driver) {
        auto p = path{info.steamVrRoot};
        p /= "drivers";
        p /= driver;
        p /= "bin";
        p /= getPlatformDirname();
        info.driverRoot = p.string();
        p /= ("driver_" + driver + DRIVER_EXTENSION);
        info.driverFile = p.string();
    }

    DriverLocationInfo findDriver(std::string const &driver) {
        DriverLocationInfo info;
        for (auto &root : getSteamVRRoots()) {
            info.steamVrRoot = root;
            computeDriverRootAndFilePath(info, driver);

            std::cout << "Will try to load driver from:\n"
                      << info.driverFile << std::endl;

            if (exists(path{info.driverRoot})) {
                std::cout << "Driver root exists" << std::endl;
            }
            if (exists(path{info.driverFile})) {
                std::cout << "Driver file exists" << std::endl;
            }
            if (exists(path{info.driverRoot}) &&
                exists(path{info.driverFile})) {
                info.found = true;
                return info;
            }
        }
        info = DriverLocationInfo{};
        return info;
    }
    std::string getToolLocation(std::string const &toolName,
                                std::string const &steamVrRoot) {
        std::vector<std::string> searchPath;
        if (!steamVrRoot.empty()) {
            searchPath = {steamVrRoot};
        } else {
            searchPath = getSteamVRRoots();
        }

        for (auto &root : searchPath) {
            auto p = path{root};
            p /= "bin";
            p /= getPlatformDirname();
            p /= (toolName + TOOL_EXTENSION);
            if (exists(p)) {
                return p.string();
            }
        }
        return std::string{};
    }

} // namespace vive
} // namespace osvr

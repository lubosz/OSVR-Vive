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
#include <boost/iostreams/stream.hpp>
#include <boost/process.hpp>
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

namespace osvr {
namespace vive {
#if defined(OSVR_WINDOWS)
    static const auto PLATFORM_DIRNAME_BASE = "win";
    static const auto DRIVER_EXTENSION = ".dll";
    static const auto TOOL_EXTENSION = ".exe";
    static const auto PATH_SEP = "\\";
#elif defined(OSVR_MACOSX)
    /// @todo Note that there are no 64-bit steamvr runtimes or drivers on OS X
    static const auto PLATFORM_DIRNAME_BASE = "osx";
    static const auto TOOL_EXTENSION = "";
    static const auto PATH_SEP = "/";
#elif defined(OSVR_LINUX)
    static const auto PLATFORM_DIRNAME_BASE = "linux";
    static const auto DRIVER_EXTENSION = ".so";
    static const auto TOOL_EXTENSION = "";
    static const auto PATH_SEP = "/";
#else
#error "Sorry, Valve does not produce a SteamVR runtime for your platform."
#endif

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
            info.driverName = driver;

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

    ConfigDirs findConfigDirs(std::string const &steamVrRoot,
                              std::string const &driver) {
        static const auto LINE_PREFIX = "Config path = ";
        ConfigDirs ret;
        bool foundLine = false;
        auto vrpathregFile =
            osvr::vive::getToolLocation("vrpathreg", steamVrRoot);
        if (vrpathregFile.empty()) {
            return ret;
        }
        std::string pathLine;
        {
            using namespace boost::process;
            using namespace boost::process::initializers;
            using namespace boost::iostreams;
            auto p = create_pipe();
            file_descriptor_sink sink(p.sink, close_handle);
            auto c = execute(run_exe(vrpathregFile),
#ifdef _WIN32

                             show_window(SW_HIDE),
#endif
                             bind_stdout(sink), bind_stderr(sink));

            file_descriptor_source source(p.source, close_handle);
            stream<file_descriptor_source> is(source);

            for (int i = 0; i < 4; ++i) {
                std::getline(is, pathLine);
                if (pathLine.find(LINE_PREFIX) != std::string::npos) {
                    foundLine = true;
                    break;
                }
            }
            wait_for_exit(c);
        }
        if (!foundLine) {
            return ret;
        }
        while (!pathLine.empty() &&
               (pathLine.back() == '\r' || pathLine.back() == '\n')) {
            pathLine.pop_back();
        }
        std::cout << "Path line is: " << pathLine << std::endl;
        auto prefixLoc = pathLine.find(LINE_PREFIX);
        auto sub = pathLine.substr(std::string(LINE_PREFIX).size() + prefixLoc);
        std::cout << "substring is: " << sub << std::endl;
        ret.rootConfigDir = sub;
        ret.driverConfigDir = sub + PATH_SEP + driver;

        if (exists(path{ret.rootConfigDir})) {
            std::cout << "root config dir exists" << std::endl;
            ret.valid = true;
        }

        return ret;
    }

} // namespace vive
} // namespace osvr

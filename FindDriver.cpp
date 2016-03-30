/** @file
    @brief Implementation

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

// Internal Includes
#include "FindDriver.h"

// Library/third-party includes
#include <boost/iostreams/stream.hpp>
#include <boost/process.hpp>
#include <json/reader.h>
#include <json/value.h>
#include <osvr/Util/Finally.h>
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
#include <shlobj.h>
#endif

#undef VIVELOADER_VERBOSE

using osvr::util::finally;

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
    static const auto DRIVER_EXTENSION = ".dylib";
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

#if defined(OSVR_USING_FILESYSTEM_TR2)
    using std::tr2::sys::path;
    using std::tr2::sys::wpath;
    using std::tr2::sys::exists;
#elif defined(OSVR_USING_BOOST_FILESYSTEM)
    using boost::filesystem::path;
    using boost::filesystem::exists;
#endif

    inline std::string getPlatformDirname() {
        return PLATFORM_DIRNAME_BASE +
               std::to_string(sizeof(void *) * CHAR_BIT);

    inline void parsePathConfigFile(std::istream &is, Json::Value &ret) {
        Json::Reader reader;
        if (!reader.parse(is, ret)) {
            std::cerr << "Error parsing file containing path configuration - "
                         "have you run SteamVR yet?"
                      << std::endl;
            std::cerr << reader.getFormattedErrorMessages() << std::endl;
        }
    }
#if defined(OSVR_WINDOWS)

    inline Json::Value getPathConfig() {
        PWSTR outString = nullptr;
        Json::Value ret;
        // It's OK to use Vista+ stuff here, since openvr_api.dll uses Vista+
        // stuff too.
        auto hr =
            SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &outString);
        if (!SUCCEEDED(hr)) {
            std::cerr << "Could not get local app data directory!" << std::endl;
            return ret;
        }
        // Free the string returned when we're all done.
        auto freeString = [&] { CoTaskMemFree(outString); };
        // Build the path to the file.
        auto vrPaths =
            wpath(outString) / wpath(L"openvr") / wpath(L"openvrpaths.vrpath");
        std::ifstream is(vrPaths.string());
        if (!is) {
            std::wcerr << L"Could not open file containing path configuration "
                          L"- have you run SteamVR yet? "
                       << vrPaths << L"\n";
            return ret;
        }
        parsePathConfigFile(is, ret);
        return ret;
    }
#elif defined(OSVR_MACOSX)
    inline Json::Value getPathConfig() {
#error "implementation not complete"
    }
#elif defined(OSVR_LINUX)

    inline Json::Value getPathConfig() {
#error "implementation not complete"
    }
#endif

#if 0
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
#endif

    inline std::vector<std::string> getSteamVRRoots(Json::Value const &json) {
        std::vector<std::string> ret;
        auto &runtimes = json["runtime"];
        if (!runtimes.isArray()) {
            return ret;
        }
        for (auto &runtime : runtimes) {
            ret.emplace_back(runtime.asString());
        }
        return ret;
    }
    inline std::vector<std::string> getSteamVRRoots() {
        return getSteamVRRoots(getPathConfig());
    }

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
#ifdef VIVELOADER_VERBOSE
            std::cout << "Will try to load driver from:\n"
                      << info.driverFile << std::endl;
            if (exists(path{info.driverRoot})) {
                std::cout << "Driver root exists" << std::endl;
            }
            if (exists(path{info.driverFile})) {
                std::cout << "Driver file exists" << std::endl;
            }
#endif
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
    ConfigDirs findConfigDirs(std::string const & /*steamVrRoot*/,
                              std::string const &driver) {
        ConfigDirs ret;
        auto json = getPathConfig();
        auto const &configLocations = json["config"];
        if (!configLocations.isArray()) {
            return ret;
        }
        for (auto &configDir : configLocations) {
            auto configPath = path{configDir.asString()};
            if (!exists(configPath)) {
                continue;
            }
            ret.rootConfigDir = configPath.string();
            ret.driverConfigDir = (configPath / path{driver}).string();
            ret.valid = true;
            return ret;
        }
        ret = ConfigDirs{};
        return ret;
    }
#if 0
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

#ifdef VIVELOADER_VERBOSE
        std::cout << "Path line is: " << pathLine << std::endl;
#endif
        auto prefixLoc = pathLine.find(LINE_PREFIX);
        auto sub = pathLine.substr(std::string(LINE_PREFIX).size() + prefixLoc);
#ifdef VIVELOADER_VERBOSE
        std::cout << "substring is: " << sub << std::endl;
#endif
        ret.rootConfigDir = sub;
        ret.driverConfigDir = sub + PATH_SEP + driver;

        if (exists(path{ret.rootConfigDir})) {
#ifdef VIVELOADER_VERBOSE
            std::cout << "root config dir exists" << std::endl;
#endif
            ret.valid = true;
        }

        return ret;
    }
#endif

} // namespace vive
} // namespace osvr

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

#ifndef INCLUDED_FindDriver_h_GUID_ED298337_8481_4594_6C82_340AC2A704D6
#define INCLUDED_FindDriver_h_GUID_ED298337_8481_4594_6C82_340AC2A704D6

// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
#include <string>

namespace osvr {
namespace vive {

    static const auto DRIVER_NAME = "lighthouse";

    struct DriverLocationInfo {
        std::string steamVrRoot;
        std::string driverRoot;
        std::string driverFile;
        std::string driverName;

        /// If this field is false, the other field contain invalid data -
        /// the driver could not be found.
        bool found = false;
    };

    /// Get the location information on the given driver
    DriverLocationInfo
    findDriver(std::string const &driver = std::string(DRIVER_NAME));

    /// Get the full path of the an executable steamvr tool, or empty string
    /// if not found.
    /// If you happen to know the steam vr root, you can pass it.
    std::string
    getToolLocation(std::string const &toolName = std::string{"vrpathreg"},
                    std::string const &steamVrRoot = std::string());

    struct ConfigDirs {
        std::string rootConfigDir;
        std::string driverConfigDir;
        bool valid = false;
    };

    ConfigDirs findConfigDirs(std::string const &steamVrRoot,
                              std::string const &driver);

    inline ConfigDirs findConfigDirs(DriverLocationInfo const &info) {
        if (!info.found) {
            return ConfigDirs{};
        }
        return findConfigDirs(info.steamVrRoot, info.driverName);
    }
} // namespace vive
} // namespace osvr

#endif // INCLUDED_FindDriver_h_GUID_ED298337_8481_4594_6C82_340AC2A704D6

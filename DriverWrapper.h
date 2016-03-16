/** @file
    @brief Header

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

#ifndef INCLUDED_DriverWrapper_h_GUID_F3D0F445_80E9_4C85_2CE4_E36293ED03F8
#define INCLUDED_DriverWrapper_h_GUID_F3D0F445_80E9_4C85_2CE4_E36293ED03F8

// Internal Includes
#include "DriverLoader.h"
#include "FindDriver.h"
#include "GetProvider.h"
#include "ViveServerDriverHost.h"

// Library/third-party includes
// - none

// Standard includes
#include <memory>
#include <stdexcept>

namespace osvr {
namespace vive {
    class DriverWrapper {
      public:
        DriverWrapper()
            : serverDriverHost_(new vr::ViveServerDriverHost),
              driverLocation_(findDriver()) {
            if (!foundDriver()) {
                return;
            }
            loader_ = DriverLoader::make(driverLocation_.driverRoot,
                                         driverLocation_.driverFile);
            if (!haveDriverLoaded()) {
                return;
            }
            configDirs_ = findConfigDirs(driverLocation_);
        }
        ~DriverWrapper() { serverDriverHost_->setExiting(); }

        DriverWrapper(DriverWrapper const &) = delete;
        DriverWrapper &operator=(DriverWrapper const &) = delete;

        /// Move constructor
        DriverWrapper(DriverWrapper &&other)
            : serverDriverHost_(std::move(other.serverDriverHost_)),
              driverLocation_(std::move(other.driverLocation_)),
              configDirs_(std::move(other.configDirs_)),
              loader_(std::move(other.loader_)),
              serverDeviceProvider_(std::move(other.serverDeviceProvider_)) {}

        explicit operator bool() const {
            /// If you have the driver loaded and the config dirs found, that
            /// implies you found the driver.
            return foundConfigDirs() && haveDriverLoaded();
        }

        bool foundDriver() const { return driverLocation_.found; }

        std::string const &getDriverFileLocation() const {
            return driverLocation_.driverFile;
        }

        bool foundConfigDirs() const { return configDirs_.valid; }

        std::string const &getDriverConfigDir() const {
            return configDirs_.driverConfigDir;
        }

        bool haveDriverLoaded() const {
            // only one of these pointers will ever be initialized at a time,
            // since we move from loader_ to initialize serverDeviceProvider_

            // loader_ has an explicit operator bool to check its success, hence
            // the longer check.
            return (static_cast<bool>(loader_) &&
                    static_cast<bool>(*loader_)) ||
                   static_cast<bool>(serverDeviceProvider_);
        }

        /// This method must be called before calling
        /// startServerDeviceProvider()
        bool isHMDPresent() {
            if (!(foundDriver() && foundConfigDirs() && haveDriverLoaded())) {
                return false;
            }
            if (!loader_) {
                throw std::logic_error("Calls to isHMDPresent must occur "
                                       "before startServerDeviceProvider!");
            }
            return loader_->isHMDPresent(configDirs_.rootConfigDir);
        }

        /// This must be called before accessing the server device provider.
        /// Returns whether
        bool startServerDeviceProvider() {
            if (!(foundDriver() && foundConfigDirs() && haveDriverLoaded())) {
                return false;
            }
            if (serverDeviceProvider_) {
                return true;
            }
            serverDeviceProvider_ =
                getProvider<vr::IServerTrackedDeviceProvider>(
                    std::move(loader_), nullptr, serverDriverHost_.get(),
                    configDirs_.driverConfigDir);
            return static_cast<bool>(serverDeviceProvider_);
        }

        vr::IServerTrackedDeviceProvider &serverDevProvider() const {
            if (!haveDriverLoaded() || !serverDeviceProvider_) {
                throw std::logic_error("Attempted to access server device "
                                       "provider when driver loading or device "
                                       "provider initialization failed or was "
                                       "not yet completed!!");
            }
            return *serverDeviceProvider_;
        }

      private:
        std::unique_ptr<vr::ViveServerDriverHost> serverDriverHost_;

        DriverLocationInfo driverLocation_;
        ConfigDirs configDirs_;

        std::unique_ptr<DriverLoader> loader_;
        ProviderPtr<vr::IServerTrackedDeviceProvider> serverDeviceProvider_;
    };
} // namespace vive
} // namespace osvr

#endif // INCLUDED_DriverWrapper_h_GUID_F3D0F445_80E9_4C85_2CE4_E36293ED03F8

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
#include "ServerDriverHost.h"

// Library/third-party includes
// - none

// Standard includes
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace osvr {
namespace vive {
    class DriverWrapper {
      public:
        using DriverVector = std::vector<vr::ITrackedDeviceServerDriver *>;

        /// Constructor for when you have your own subclass of ServerDriverHost
        /// to pass in.
        explicit DriverWrapper(vr::ServerDriverHost *serverDriverHost)
            : serverDriverHost_(serverDriverHost),
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

        /// Default constructor: we make and own our own ServerDriverHost.
        DriverWrapper()
            : owningServerDriverHost_(new vr::ServerDriverHost),
              serverDriverHost_(owningServerDriverHost_.get()),
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

        ~DriverWrapper() {
            serverDriverHost_->setExiting();
            if (deactivateOnShutdown_) {
                for (auto &dev : devices_) {
                    dev->Deactivate();
                }
            }
        }

        DriverWrapper(DriverWrapper const &) = delete;
        DriverWrapper &operator=(DriverWrapper const &) = delete;

/// Move constructor
#if defined(_MSC_VER) && _MSC_VER < 1900
        /// VS2013 doesn't know how to make a default move constructor :(
        DriverWrapper(DriverWrapper &&other)
            : owningServerDriverHost_(std::move(other.owningServerDriverHost_)),
              serverDriverHost_(std::move(other.serverDriverHost_)),
              driverLocation_(std::move(other.driverLocation_)),
              configDirs_(std::move(other.configDirs_)),
              loader_(std::move(other.loader_)),
              serverDeviceProvider_(std::move(other.serverDeviceProvider_)),
              devices_(std::move(other.devices_)),
              deactivateOnShutdown_(other.deactivateOnShutdown_) {}
#else

        DriverWrapper(DriverWrapper &&other) = default;
#endif

        explicit operator bool() const {
            /// If you have the driver loaded and the config dirs found, that
            /// implies you found the driver.
            return foundConfigDirs() && haveDriverLoaded() &&
                   haveServerDeviceHost();
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

        bool haveServerDeviceHost() const {
            return static_cast<bool>(serverDriverHost_);
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
            if (!(foundDriver() && foundConfigDirs() && haveDriverLoaded() &&
                  haveServerDeviceHost())) {
                return false;
            }
            if (serverDeviceProvider_) {
                return true;
            }
            serverDeviceProvider_ =
                getProvider<vr::IServerTrackedDeviceProvider>(
                    std::move(loader_), nullptr, serverDriverHost_,
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

        vr::ServerDriverHost &driverHost() const { return *serverDriverHost_; }

        DriverVector const &devices() const { return devices_; }

        std::pair<bool, std::uint32_t>
        addAndActivateDevice(vr::ITrackedDeviceServerDriver *dev) {
            /// check to make sure it's not already in there?
            auto it = std::find(begin(devices_), end(devices_), dev);
            if (it != end(devices_)) {
                return std::make_pair(false, 0);
            }
            auto newId = static_cast<std::uint32_t>(devices_.size());
            devices_.push_back(dev);
            dev->Activate(newId);
            return std::make_pair(true, newId);
        }

        /// Set whether all devices should be deactivated on shutdown - defaults
        /// to true, so you might just want to set to false if, for instance,
        /// you deactivate and power off the devices on shutdown yourself.
        void shouldDeactivateDeviceOnShutdown(bool deactivate) {
            deactivateOnShutdown_ = deactivate;
        }

      private:
        /// This pointer manages lifetime if we created our own host but isn't
        /// accessed beyond that.
        std::unique_ptr<vr::ServerDriverHost> owningServerDriverHost_;
        /// This pointer is the one used, whether we create our own or get one
        /// passed in.
        vr::ServerDriverHost *serverDriverHost_;

        DriverLocationInfo driverLocation_;
        ConfigDirs configDirs_;

        std::unique_ptr<DriverLoader> loader_;
        ProviderPtr<vr::IServerTrackedDeviceProvider> serverDeviceProvider_;

        DriverVector devices_;

        bool deactivateOnShutdown_ = true;
    };
} // namespace vive
} // namespace osvr

#endif // INCLUDED_DriverWrapper_h_GUID_F3D0F445_80E9_4C85_2CE4_E36293ED03F8

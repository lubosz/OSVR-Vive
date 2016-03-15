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

#ifndef INCLUDED_GetProvider_h_GUID_299C10C7_DE4D_4DB0_14DB_5CCAC70F3C11
#define INCLUDED_GetProvider_h_GUID_299C10C7_DE4D_4DB0_14DB_5CCAC70F3C11

// Internal Includes
#include "DriverLoader.h"
#include "InterfaceTraits.h"

// Library/third-party includes
#include <openvr_driver.h>

// Standard includes
#include <iostream>
#include <memory>
#include <string>

namespace osvr {
namespace vive {
#if 0
    template <typename Provider> struct DriverProviderCleanupDeleter {
        static_assert(
            InterfaceExpectedFromEntryPointTrait<Provider>::value,
            "Deleter only valid for those 'provider' interface types expected "
            "to be provided by the driver entry point.");

        void operator()(Provider *ptr) const {
            if (ptr) {
                ptr->Cleanup();
            }
        }
    };
#endif
    template <typename InterfaceType>
    using ProviderPtr = std::shared_ptr<InterfaceType>;

    /// Gets one of the main interfaces provided by the driver entry point.
    /// Ownership of the driver loader is transferred (and maintained in the
    /// background by the returned interface smart pointer), so std::move that
    /// thing in.
    template <typename InterfaceType>
    inline ProviderPtr<InterfaceType>
    getProvider(std::unique_ptr<DriverLoader> &&loader,
                vr::IDriverLog *driverLog, InterfaceHost<InterfaceType> *host,
                std::string const &userDriverConfigDir) {
        using return_type = ProviderPtr<InterfaceType>;
        static_assert(
            InterfaceExpectedFromEntryPointTrait<InterfaceType>::value,
            "Function only valid for those 'provider' interface types expected "
            "to be provided by the driver entry point.");

        if (!loader) {
            return return_type{};
        }
        /// Move into local pointer, so if something goes wrong, the driver gets
        /// unloaded.
        std::unique_ptr<DriverLoader> myLoader(std::move(loader));
        auto rawPtr = myLoader->getInterfaceThrowing<InterfaceType>();
        auto initResults =
            rawPtr->Init(driverLog, host, userDriverConfigDir.c_str(),
                         myLoader->getDriverRoot().c_str());
        if (vr::VRInitError_None != initResults) {
            /// Failed, reset the loader pointer to unload the driver.
            std::cout << "Got error code " << initResults << std::endl;
            myLoader.reset();
            return return_type{};
        }

        /// OK, so this is the interface. Move the loader into a shared pointer,
        /// make the loader responsible for cleanup of the interface, and get
        /// the shared pointer of the interface pointer returned.
        std::shared_ptr<DriverLoader> sharedLoader(std::move(myLoader));
        sharedLoader->cleanupInterfaceOnDestruction(rawPtr);
        /// This is the so-called "aliasing" constructor - this pointer will
        /// actually keep the DriverLoader alive.
        return_type ret(sharedLoader, rawPtr);

        return ret;
    }
} // namespace vive
} // namespace osvr

#endif // INCLUDED_GetProvider_h_GUID_299C10C7_DE4D_4DB0_14DB_5CCAC70F3C11

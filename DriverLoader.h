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

#ifndef INCLUDED_DriverLoader_h_GUID_882F2FD5_F218_42BE_3088_31CF712EC455
#define INCLUDED_DriverLoader_h_GUID_882F2FD5_F218_42BE_3088_31CF712EC455

// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
#include <memory>
#include <string>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace osvr {
namespace vive {

    class DriverLoader {
      public:
        DriverLoader(std::string const &driverRoot,
                     std::string const &driverFile);

        ~DriverLoader();

        DriverLoader(DriverLoader const &) = delete;
        DriverLoader &operator=(DriverLoader const &) = delete;

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

      private:
        using DriverFactory = void *(*)(const char *, int *);
        // typedef void *(DriverFactory)(const char *, int *);

        struct Impl;
        std::unique_ptr<Impl> impl_;

        DriverFactory factory_;
    };

} // namespace vive
} // namespace osvr
#endif // INCLUDED_DriverLoader_h_GUID_882F2FD5_F218_42BE_3088_31CF712EC455

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

#ifndef INCLUDED_VerifyLocked_h_GUID_E7A7AC6C_77E3_45AE_975D_3335B04DC12A
#define INCLUDED_VerifyLocked_h_GUID_E7A7AC6C_77E3_45AE_975D_3335B04DC12A

// Internal Includes
// - none

// Library/third-party includes
// - none

// Standard includes
#include <mutex>

namespace osvr {
namespace vive {
    /// Purpose: to provide a unified way to deal with unique_lock and
    /// lock_guard to ensure they are locked, even though lock_guard is, by
    /// definition, locked...
    template <typename T> struct VerifyLocked;

    /// strip const
    template <typename T> struct VerifyLocked<const T> : VerifyLocked<T> {};

    template <typename MutexType>
    struct VerifyLocked<std::lock_guard<MutexType>> {
        static bool verify(std::lock_guard<MutexType> & /*lock*/) {
            return true;
        }
    };
    template <typename MutexType>
    struct VerifyLocked<std::unique_lock<MutexType>> {
        static bool verify(std::unique_lock<MutexType> &lock) {
            return lock.owns_lock();
        }
    };

    /// Function template to check if a lock is locked, whether or not it's by
    /// definition or at runtime.
    template <typename LockType> inline bool verifyLocked(LockType &lock) {
        return VerifyLocked<LockType>::verify(lock);
    }

} // namespace vive
} // namespace osvr
#endif // INCLUDED_VerifyLocked_h_GUID_E7A7AC6C_77E3_45AE_975D_3335B04DC12A

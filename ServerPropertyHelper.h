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

#ifndef INCLUDED_ServerPropertyHelper_h_GUID_9835023A_E639_409F_3A68_CFB4029FBBE7
#define INCLUDED_ServerPropertyHelper_h_GUID_9835023A_E639_409F_3A68_CFB4029FBBE7

#include <openvr_driver.h>

#include "PropertyHelper.h"

namespace osvr {
namespace vive {
    /// Pass a vr::Prop_... enum value as the template parameter, your tracked
    /// device as the parameter, and get back a pair containing the
    /// correctly-typed result and your error code.
    ///
    /// @sa generic::getProperty()
    template <std::size_t EnumVal>
    inline detail::EnumGetterReturn<EnumVal>
    getProperty(vr::ITrackedDeviceServerDriver *dev) {
        return generic::getProperty<EnumVal>(dev);
    }

    /// @overload
    /// Takes a Props:: enum class value as template parameter instead.
    ///
    /// @sa generic::getProperty()
    template <Props EnumVal>
    inline detail::EnumClassGetterReturn<EnumVal>
    getProperty(vr::ITrackedDeviceServerDriver *dev) {
        return generic::getProperty<EnumVal>(dev);
    }

    /// @overload
    /// @sa getPropertyOfType()
    /// @sa generic::getPropertyOfType()
    ///
    /// This overload would be ambiguous with variadics, but here, it's
    /// perfectly fine.
    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getProperty(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
        return generic::getPropertyOfType<T>(dev, prop);
    }
    /// @overload
    /// @sa getPropertyOfType()
    /// @sa generic::getPropertyOfType()
    ///
    /// This overload would be ambiguous with variadics, but here, it's
    /// perfectly fine.
    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getProperty(vr::ITrackedDeviceServerDriver *dev, Props prop) {
        return generic::getPropertyOfType<T>(dev, prop);
    }
    /// For when you must pass the enum at runtime - less safe - the template
    /// parameter is the type, then, and the parameters are the device and the
    /// property enum.
    ///
    /// @sa generic::getPropertyOfType()
    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getPropertyOfType(vr::ITrackedDeviceServerDriver *dev,
                      vr::ETrackedDeviceProperty prop) {
        return generic::getPropertyOfType<T>(dev, prop);
    }
    /// @overload
    /// Takes a Props:: enum class value as a parameter instead.
    ///
    /// @sa generic::getPropertyOfType()
    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getPropertyOfType(vr::ITrackedDeviceServerDriver *dev, Props prop) {
        return generic::getPropertyOfType<T>(dev, prop);
    }
} // namespace vive
} // namespace osvr

#endif // INCLUDED_ServerPropertyHelper_h_GUID_9835023A_E639_409F_3A68_CFB4029FBBE7

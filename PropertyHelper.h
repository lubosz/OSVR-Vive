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

#ifndef INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3
#define INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3

#include "PropertyTraits.h"
#include <openvr_driver.h>

// Standard includes
#include <assert.h>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace osvr {
namespace vive {
    std::pair<std::string, vr::ETrackedPropertyError> inline getStringProperty(
        vr::ITrackedDeviceServerDriver *dev, vr::ETrackedDeviceProperty prop) {
        assert(dev != nullptr &&
               "Tried to get a string property from a null device "
               "pointer.");
        static const auto INITIAL_BUFFER_SIZE = vr::k_unTrackingStringSize;
        /// Start with a buffer of k_unTrackingStringSize as suggested.
        std::vector<char> buf(INITIAL_BUFFER_SIZE, '\0');
        vr::ETrackedPropertyError err;
        auto ret = dev->GetStringTrackedDeviceProperty(
            prop, buf.data(), static_cast<uint32_t>(buf.size()), &err);
        if (0 == ret) {
            // property not available
            return std::make_pair(std::string{}, err);
        }

        if (ret > buf.size()) {
            std::cout << "[getStringProperty] Got an initial return value "
                         "larger than the buffer size: ret = "
                      << ret << ", buf.size() = " << buf.size() << std::endl;
        }
        if (vr::TrackedProp_BufferTooSmall == err) {
            // first buffer was too small, but now we know how big it should be,
            // per the docs.
            /// @todo remove this debug print
            std::cout << "[getStringProperty] Initial buffer size: "
                      << buf.size() << ", return value: " << ret << std::endl;
            buf.resize(ret + 1, '\0');
            ret = dev->GetStringTrackedDeviceProperty(
                prop, buf.data(), static_cast<uint32_t>(buf.size()), &err);
        }

        if (ret > buf.size()) {
            std::cout << "[getStringProperty] THIS SHOULDN'T HAPPEN: Got a "
                         "return value larger than the buffer size: ret = "
                      << ret << ", buf.size() = " << buf.size() << std::endl;

            return std::make_pair(std::string{}, err);
        }
        return std::make_pair(std::string{buf.data()}, err);
    }
    namespace detail {
        template <typename PropertyType>
        using PropertyGetterReturn =
            std::pair<PropertyType, vr::ETrackedPropertyError>;

        template <std::size_t EnumVal>
        using EnumGetterReturn = PropertyGetterReturn<PropertyType<EnumVal>>;

        template <Props EnumVal>
        struct EnumClassToSizeT
            : std::integral_constant<std::size_t,
                                     static_cast<std::size_t>(
                                         static_cast<int>(EnumVal))> {};

        template <Props EnumVal>
        using EnumClassGetterReturn =
            EnumGetterReturn<EnumClassToSizeT<EnumVal>::value>;

        inline vr::ETrackedDeviceProperty castToProperty(std::size_t prop) {
            return static_cast<vr::ETrackedDeviceProperty>(prop);
        }
        inline vr::ETrackedDeviceProperty castToProperty(Props val) {
            return static_cast<vr::ETrackedDeviceProperty>(
                static_cast<int>(val));
        }

        template <typename EnumType> struct PropertyGetter;

        template <> struct PropertyGetter<bool> {
            static PropertyGetterReturn<bool>
            get(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
                vr::ETrackedPropertyError err;
                bool val = dev->GetBoolTrackedDeviceProperty(prop, &err);
                return std::make_pair(val, err);
            }
        };
        template <> struct PropertyGetter<float> {
            static PropertyGetterReturn<float>
            get(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
                vr::ETrackedPropertyError err;
                float val = dev->GetFloatTrackedDeviceProperty(prop, &err);
                return std::make_pair(val, err);
            }
        };
        template <> struct PropertyGetter<int32_t> {
            static PropertyGetterReturn<int32_t>
            get(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
                vr::ETrackedPropertyError err;
                int32_t val = dev->GetInt32TrackedDeviceProperty(prop, &err);
                return std::make_pair(val, err);
            }
        };
        template <> struct PropertyGetter<vr::HmdMatrix34_t> {
            static PropertyGetterReturn<vr::HmdMatrix34_t>
            get(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
                vr::ETrackedPropertyError err;
                vr::HmdMatrix34_t val =
                    dev->GetMatrix34TrackedDeviceProperty(prop, &err);
                return std::make_pair(val, err);
            }
        };
        template <> struct PropertyGetter<std::string> {
            static PropertyGetterReturn<std::string>
            get(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
                return getStringProperty(dev, prop);
            }
        };
        template <> struct PropertyGetter<uint64_t> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<uint64_t>
            get(T *self, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err;
                uint64_t val =
                    self->GetUint64TrackedDeviceProperty(args..., prop, &err);
                return std::make_pair(val, err);
            }
        };
        template <std::size_t EnumVal>
        using PropertyGetterFromSizeT = PropertyGetter<PropertyType<EnumVal>>;

        template <Props EnumVal>
        using PropertyGetterFromEnumClass =
            PropertyGetterFromSizeT<EnumClassToSizeT<EnumVal>::value>;
    } // namespace detail

    template <std::size_t EnumVal>
    inline detail::EnumGetterReturn<EnumVal>
    getProperty(vr::ITrackedDeviceServerDriver *dev) {
        return detail::PropertyGetterFromSizeT<EnumVal>::get(
            dev, detail::castToProperty(EnumVal));
    }
    template <Props EnumVal>
    inline detail::EnumClassGetterReturn<EnumVal>
    getProperty(vr::ITrackedDeviceServerDriver *dev) {
        return detail::PropertyGetterFromEnumClass<EnumVal>::get(
            dev, detail::castToProperty(EnumVal));
    }

    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getProperty(vr::ITrackedDeviceServerDriver *dev,
                vr::ETrackedDeviceProperty prop) {
        return detail::PropertyGetter<T>::get(dev, prop);
    }

    template <typename T>
    inline detail::PropertyGetterReturn<T>
    getProperty(vr::ITrackedDeviceServerDriver *dev, Props prop) {
        return detail::PropertyGetter<T>::get(dev,
                                              detail::castToProperty(prop));
    }
} // namespace vive
} // namespace osvr

#endif // INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3

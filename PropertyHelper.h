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

#include <cstddef>

#include "PropertyTraits.h"

// Standard includes
#include <assert.h>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace osvr {
namespace vive {

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
            template <typename T, typename... Args>
            static PropertyGetterReturn<bool>
            get(T *dev, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
                bool val =
                    dev->GetBoolTrackedDeviceProperty(args..., prop, &err);
                return std::make_pair(val, err);
            }
        };

        template <> struct PropertyGetter<float> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<float>
            get(T *dev, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
                float val = dev->GetFloatTrackedDeviceProperty(
                    std::forward<Args>(args)..., prop, &err);
                return std::make_pair(val, err);
            }
        };

        template <> struct PropertyGetter<int32_t> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<int32_t>
            get(T *dev, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
                int32_t val =
                    dev->GetInt32TrackedDeviceProperty(args..., prop, &err);
                return std::make_pair(val, err);
            }
        };

        template <> struct PropertyGetter<vr::HmdMatrix34_t> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<vr::HmdMatrix34_t>
            get(T *dev, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
                vr::HmdMatrix34_t val =
                    dev->GetMatrix34TrackedDeviceProperty(args..., prop, &err);
                return std::make_pair(val, err);
            }
        };

        template <> struct PropertyGetter<std::string> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<std::string>
            get(T *dev, vr::ETrackedDeviceProperty prop, Args... args) {

                assert(dev != nullptr &&
                       "Tried to get a string property from a null device "
                       "pointer.");
                static const auto INITIAL_BUFFER_SIZE =
                    vr::k_unTrackingStringSize;
                /// Start with a buffer of k_unTrackingStringSize as suggested.
                std::vector<char> buf(INITIAL_BUFFER_SIZE, '\0');
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
                auto ret = dev->GetStringTrackedDeviceProperty(
                    args..., prop, buf.data(),
                    static_cast<uint32_t>(buf.size()), &err);
                if (0 == ret) {
                    // property not available
                    return std::make_pair(std::string{}, err);
                }

                if (ret > buf.size()) {
                    std::cout
                        << "[getStringProperty] Got an initial return value "
                           "larger than the buffer size: ret = "
                        << ret << ", buf.size() = " << buf.size() << std::endl;
                }
                if (vr::TrackedProp_BufferTooSmall == err) {
                    // first buffer was too small, but now we know how big it
                    // should be, per the docs.
                    /// @todo remove this debug print
                    std::cout << "[getStringProperty] Initial buffer size: "
                              << buf.size() << ", return value: " << ret
                              << std::endl;
                    buf.resize(ret + 1, '\0');
                    ret = dev->GetStringTrackedDeviceProperty(
                        args..., prop, buf.data(),
                        static_cast<uint32_t>(buf.size()), &err);
                }

                if (ret > buf.size()) {
                    std::cout
                        << "[getStringProperty] THIS SHOULDN'T HAPPEN: Got a "
                           "return value larger than the buffer size: ret = "
                        << ret << ", buf.size() = " << buf.size() << std::endl;

                    return std::make_pair(std::string{}, err);
                }
                return std::make_pair(std::string{buf.data()}, err);
            }
        };

        template <> struct PropertyGetter<uint64_t> {
            template <typename T, typename... Args>
            static PropertyGetterReturn<uint64_t>
            get(T *self, vr::ETrackedDeviceProperty prop, Args... args) {
                vr::ETrackedPropertyError err = vr::TrackedProp_Success;
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

    namespace generic {
        /// Get a property - pass the vr::Prop_.... as the template parameter,
        /// and the object (vr::ITrackedDeviceServerDriver *, vr::IVRSystem*)
        /// and any additional parameters required (vr::TrackedDeviceIndex_t
        /// for IVRSystem, for instance), and get back a pair with your result,
        /// correctly typed, and your error code.
        template <std::size_t EnumVal, typename Obj, typename... Args>
        inline detail::EnumGetterReturn<EnumVal> getProperty(Obj *obj,
                                                             Args... args) {
            return detail::PropertyGetterFromSizeT<EnumVal>::get(
                obj, detail::castToProperty(EnumVal), args...);
        }

        /// @overload
        ///
        /// Get a property - pass the Props:: enum class shortened name as the
        /// template parameter, and the object (vr::ITrackedDeviceServerDriver
        /// *, vr::IVRSystem*) and any additional parameters required
        /// (vr::TrackedDeviceIndex_t for IVRSystem, for instance), and get back
        /// a pair with your result, correctly typed, and your error code.
        template <Props EnumVal, typename Obj, typename... Args>
        inline detail::EnumClassGetterReturn<EnumVal>
        getProperty(Obj *obj, Args... args) {
            return detail::PropertyGetterFromEnumClass<EnumVal>::get(
                obj, detail::castToProperty(EnumVal), args...);
        }

        /// Get a property when you only have the type at compile time, not the
        /// enum itself (not as asfe as getProperty())
        ///
        /// Pass the type you'd like to receive as the template parameter (one
        /// of std::string, bool, int32_t, uint64_t, and vr::HmdMatrix34_t), and
        /// pass the and the object (vr::ITrackedDeviceServerDriver *,
        /// vr::IVRSystem*), the vr::Prop_.... enum, and any additional
        /// parameters required (vr::TrackedDeviceIndex_t for IVRSystem, for
        /// instance), and get back a pair with your result as requested, and
        /// your error code.
        template <typename T, typename Obj, typename... Args>
        inline detail::PropertyGetterReturn<T>
        getPropertyOfType(Obj *obj, vr::ETrackedDeviceProperty prop,
                          Args... args) {
            return detail::PropertyGetter<T>::get(obj, prop, args...);
        }

        /// @overload
        ///
        /// Takes a Props:: enum class shortened name instead of a vr::Prop_...
        /// enum.
        template <typename T, typename Obj, typename... Args>
        inline detail::PropertyGetterReturn<T>
        getPropertyOfType(Obj *obj, Props prop, Args... args) {
            return detail::PropertyGetter<T>::get(
                obj, detail::castToProperty(prop), args...);
        }
    } // namespace generic
} // namespace vive
} // namespace osvr

#endif // INCLUDED_PropertyHelper_h_GUID_08BEA00F_2E0C_4FA5_DF5B_31BE33EF27A3

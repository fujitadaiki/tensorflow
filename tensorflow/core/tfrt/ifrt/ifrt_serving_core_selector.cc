/* Copyright 2024 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/tfrt/ifrt/ifrt_serving_core_selector.h"

#include <cstdint>

#include "absl/strings/str_cat.h"
#include "xla/tsl/framework/serving_device_selector.h"

namespace tensorflow {
namespace ifrt_serving {

IfrtServingCoreSelector::IfrtServingCoreSelector(
    tsl::ServingDeviceSelector* device_selector)
    : device_selector_(device_selector) {}

tsl::DeviceReservation IfrtServingCoreSelector::ReserveDevice(
    int64_t program_id) {
  return device_selector_->ReserveDevice(absl::StrCat(program_id));
}

}  // namespace ifrt_serving
}  // namespace tensorflow

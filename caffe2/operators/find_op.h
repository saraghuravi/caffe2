/**
 * Copyright (c) 2016-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CAFFE2_OPERATORS_FIND_OP_H_
#define CAFFE2_OPERATORS_FIND_OP_H_

#include "caffe2/core/context.h"
#include "caffe2/core/logging.h"
#include "caffe2/core/operator.h"

#include <unordered_map>

namespace caffe2 {

template <class Context>
class FindOp final : public Operator<Context> {
 public:
  FindOp(const OperatorDef& operator_def, Workspace* ws)
      : Operator<Context>(operator_def, ws),
        missing_value_(
            OperatorBase::GetSingleArgument<int>("missing_value", -1)) {}
  USE_OPERATOR_CONTEXT_FUNCTIONS;
  USE_DISPATCH_HELPER;

  bool RunOnDevice() {
    return DispatchHelper<TensorTypes<int, long>>::call(this, Input(0));
  }

 protected:
  template <typename T>
  bool DoRunWithType() {
    auto& idx = Input(0);
    auto& needles = Input(1);
    auto* res_indices = Output(0);
    res_indices->ResizeLike(needles);

    const T* idx_data = idx.template data<T>();
    const T* needles_data = needles.template data<T>();
    T* res_data = res_indices->template mutable_data<T>();
    auto idx_size = idx.size();

    // Use an arbitrary cut-off for when to use brute-force
    // search. For larger needle sizes we first put the
    // index into a map
    if (needles.size() < 16) {
      // Brute force O(nm)
      for (int i = 0; i < needles.size(); i++) {
        T x = needles_data[i];
        T res = static_cast<T>(missing_value_);
        for (int j = idx_size - 1; j >= 0; j--) {
          if (idx_data[j] == x) {
            res = j;
            break;
          }
        }
        res_data[i] = res;
      }
    } else {
      // O(n + m)
      std::unordered_map<T, int> idx_map;
      for (int j = 0; j < idx_size; j++) {
        idx_map[idx_data[j]] = j;
      }
      for (int i = 0; i < needles.size(); i++) {
        T x = needles_data[i];
        auto it = idx_map.find(x);
        res_data[i] = (it == idx_map.end() ? missing_value_ : it->second);
      }
    }

    return true;
  }

 protected:
  int missing_value_;
};

} // namespace caffe2

#endif // CAFFE2_OPERATORS_FIND_OP_H_
// Ceres Solver - A fast non-linear least squares minimizer
// Copyright 2023 Google Inc. All rights reserved.
// http://ceres-solver.org/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of Google Inc. nor the names of its contributors may be
//   used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Author: sameeragarwal@google.com (Sameer Agarwal)

#include "ceres/residual_block_utils.h"

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

#include "absl/log/check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "ceres/array_utils.h"
#include "ceres/internal/eigen.h"
#include "ceres/internal/export.h"
#include "ceres/parameter_block.h"
#include "ceres/residual_block.h"

namespace ceres::internal {

void InvalidateEvaluation(const ResidualBlock& block,
                          double* cost,
                          double* residuals,
                          double** jacobians) {
  const int num_parameter_blocks = block.NumParameterBlocks();
  const int num_residuals = block.NumResiduals();

  InvalidateArray(1, cost);
  InvalidateArray(num_residuals, residuals);
  if (jacobians != nullptr) {
    for (int i = 0; i < num_parameter_blocks; ++i) {
      const int parameter_block_size = block.parameter_blocks()[i]->Size();
      InvalidateArray(num_residuals * parameter_block_size, jacobians[i]);
    }
  }
}

std::string EvaluationToString(const ResidualBlock& block,
                               double const* const* parameters,
                               double* cost,
                               double* residuals,
                               double** jacobians) {
  CHECK(cost != nullptr);
  CHECK(residuals != nullptr);

  const int num_parameter_blocks = block.NumParameterBlocks();
  const int num_residuals = block.NumResiduals();
  std::string result = absl::StrFormat(
      "Residual Block size: %d parameter blocks x %d residuals\n\n",
      num_parameter_blocks,
      num_residuals);

  // clang-format off
  absl::StrAppend(&result,
                  "For each parameter block, the value of the parameters are "
                  "printed in the first column   \n"
                  "and the value of the jacobian under the corresponding "
                  "residual. If a ParameterBlock was \n"
                  "held constant then the corresponding jacobian is printed as "
                  "'Not Computed'. If an entry \n"
                  "of the Jacobian/residual array was requested but was not "
                  "written to by user code, it is \n"
                  "indicated by 'Uninitialized'. This is an error. Residuals "
                  "or Jacobian values evaluating \n"
                  "to Inf or NaN is also an error.  \n\n");
  // clang-format on

  absl::StrAppend(&result, "Residuals:     ");
  AppendArrayToString(num_residuals, residuals, &result);
  absl::StrAppend(&result, "\n\n");

  for (int i = 0; i < num_parameter_blocks; ++i) {
    const int parameter_block_size = block.parameter_blocks()[i]->Size();
    absl::StrAppendFormat(
        &result, "Parameter Block %d, size: %d\n", i, parameter_block_size);
    absl::StrAppend(&result, "\n");
    for (int j = 0; j < parameter_block_size; ++j) {
      AppendArrayToString(1, parameters[i] + j, &result);
      absl::StrAppend(&result, "| ");
      for (int k = 0; k < num_residuals; ++k) {
        AppendArrayToString(1,
                            (jacobians != nullptr && jacobians[i] != nullptr)
                                ? jacobians[i] + k * parameter_block_size + j
                                : nullptr,
                            &result);
      }
      absl::StrAppend(&result, "\n");
    }
    absl::StrAppend(&result, "\n");
  }
  absl::StrAppend(&result, "\n");
  return result;
}

// TODO(sameeragarwal) Check cost value validness here
// Cost value is a part of evaluation but not checked here since according to
// residual_block.cc cost is not valid at the time this method is called
bool IsEvaluationValid(const ResidualBlock& block,
                       double const* const* /*parameters*/,
                       double* residuals,
                       double** jacobians) {
  const int num_parameter_blocks = block.NumParameterBlocks();
  const int num_residuals = block.NumResiduals();

  if (!IsArrayValid(num_residuals, residuals)) {
    return false;
  }

  if (jacobians != nullptr) {
    for (int i = 0; i < num_parameter_blocks; ++i) {
      const int parameter_block_size = block.parameter_blocks()[i]->Size();
      if (!IsArrayValid(num_residuals * parameter_block_size, jacobians[i])) {
        return false;
      }
    }
  }

  return true;
}

}  // namespace ceres::internal

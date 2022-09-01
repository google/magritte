//
// Copyright 2021-2022 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "magritte/calculators/new_canvas_calculator.h"

#include <utility>

#include "magritte/calculators/new_canvas_calculator.pb.h"
#include "mediapipe/framework/port/gmock.h"
#include "mediapipe/framework/port/gtest.h"
#include "mediapipe/framework/port/parse_text_proto.h"

namespace magritte {
namespace {

struct OptionsToSizeTestCase {
  const std::string test_name;
  const NewCanvasCalculatorOptions options;
  const int original_width;
  const int original_height;
  const std::pair<int, int> expected;
};

class NewCanvasCalculatorTest
    : public testing::TestWithParam<OptionsToSizeTestCase> {};

TEST_P(NewCanvasCalculatorTest, GetSizeFromOptionsTest) {
  const OptionsToSizeTestCase& test_case = GetParam();

  std::pair<int, int> result = NewCanvasCalculator::GetSizeFromOptions(
      test_case.options, test_case.original_width, test_case.original_height);

  ASSERT_EQ(result, test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(
    NewCanvasCalcolatorTests, NewCanvasCalculatorTest,
    testing::ValuesIn<OptionsToSizeTestCase>({
        {.test_name = "no_scaling",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb()pb"),
         .original_width = 640,
         .original_height = 480,
         .expected = {640, 480}},

        {.test_name = "factor_scaling",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb(scale_factor: 0.5)pb"),
         .original_width = 640,
         .original_height = 480,
         .expected = {320, 240}},

        {.test_name = "only_target_width",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb(target_width: 100)pb"),
         .original_width = 2,
         .original_height = 1,
         .expected = {100, 50}},

        {.test_name = "only_target_height",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb(target_height: 100)pb"),
         .original_width = 2,
         .original_height = 1,
         .expected = {200, 100}},

        {.test_name = "both_target_width_and_height",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb(target_width: 123 target_height: 456)pb"),
         .original_width = 640,
         .original_height = 480,
         .expected = {123, 456}},
         
        {.test_name = "everything_set_factor_takes_precedence",
         .options = mediapipe::ParseTextProtoOrDie<NewCanvasCalculatorOptions>(
             R"pb(scale_factor: 0.5 target_width: 123 target_height: 456)pb"),
         .original_width = 640,
         .original_height = 480,
         .expected = {320, 240}},
    }),
    [](const testing::TestParamInfo<NewCanvasCalculatorTest::ParamType>& info) {
      return info.param.test_name;
    });

}  // namespace
}  // namespace magritte

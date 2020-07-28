/* Copyright 2020 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include <fstream>
#include <iostream>

#include "gflags/gflags.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "ml_metadata/tools/mlmd_bench/benchmark.h"
#include "ml_metadata/tools/mlmd_bench/proto/mlmd_bench.pb.h"
#include "ml_metadata/tools/mlmd_bench/thread_runner.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/platform/env.h"

namespace ml_metadata {
namespace {

// Initializes the `mlmd_bench_config` with the specified configuration .pbtxt
// file.
tensorflow::Status InitAndValidateMLMDBenchConfig(
    const std::string& config_file_path, MLMDBenchConfig& mlmd_bench_config) {
  if (tensorflow::Env::Default()->FileExists(config_file_path).ok()) {
    return ReadTextProto(tensorflow::Env::Default(), config_file_path,
                         &mlmd_bench_config);
  }
  return tensorflow::errors::NotFound(
      "Could not find mlmd_bench configuration .pb or "
      ".pbtxt at supplied input file path: ",
      config_file_path);
}

// Writes the performance report into a specified file in output path.
tensorflow::Status WriteProtoResultToDisk(
    const std::string& output_directory,
    const MLMDBenchReport& mlmd_bench_report) {
  if (tensorflow::Env::Default()->IsDirectory(output_directory).ok()) {
    return tensorflow::WriteTextProto(
        tensorflow::Env::Default(),
        absl::StrCat(output_directory, "mlmd_bench_report.txt"),
        mlmd_bench_report);
  }
  return tensorflow::errors::NotFound(
      "Could not find valid output directory as: ", output_directory);
}

}  // namespace
}  // namespace ml_metadata

// Input and output file directory specified by the user.
DEFINE_string(config_file_path, "",
              "The input mlmd_bench configuration .pbtxt file path.");
DEFINE_string(
    output_directory, "",
    "The output directory of the performance report: mlmd_bench_report.txt.");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Configurations for mlmd_bench.
  ml_metadata::MLMDBenchConfig mlmd_bench_config;
  TF_CHECK_OK(ml_metadata::InitAndValidateMLMDBenchConfig(
      FLAGS_config_file_path, mlmd_bench_config));

  // Feeds the `mlmd_bench_config` into the benchmark for generating executable
  // workloads.
  ml_metadata::Benchmark benchmark(mlmd_bench_config);
  // Executes the workloads inside the benchmark with the thread runner.
  ml_metadata::ThreadRunner runner(
      mlmd_bench_config.mlmd_config(),
      mlmd_bench_config.thread_env_config().num_threads());
  TF_CHECK_OK(runner.Run(benchmark));

  TF_CHECK_OK(ml_metadata::WriteProtoResultToDisk(
      FLAGS_output_directory, benchmark.mlmd_bench_report()));

  return 0;
}

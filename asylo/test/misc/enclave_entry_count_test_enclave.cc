/*
 *
 * Copyright 2019 Asylo authors
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
 *
 */

#include <unistd.h>

#include <atomic>

#include "absl/synchronization/mutex.h"
#include "asylo/platform/arch/include/trusted/host_calls.h"
#include "asylo/platform/primitives/trusted_runtime.h"
#include "asylo/test/misc/enclave_entry_count_test.pb.h"
#include "asylo/test/util/enclave_test_application.h"
#include "asylo/util/status.h"

namespace asylo {

constexpr int kTimeout = 5;

// Mutex used in the Mutex-enabled routine.
absl::Mutex mu;

static bool count_finished = false;
static volatile std::atomic<int> active_entries(0);

class EnclaveEntryCountTest : public EnclaveTestCase {
 public:
  EnclaveEntryCountTest() = default;

  Status Run(const EnclaveInput &input, EnclaveOutput *output) {
    if (!input.HasExtension(enclave_entry_count_test_input)) {
      return Status(error::GoogleError::INVALID_ARGUMENT,
                    "Missing input extension");
    }
    EnclaveEntryCountTestInput test_input =
        input.GetExtension(enclave_entry_count_test_input);
    if (!test_input.has_thread_type()) {
      return Status(error::GoogleError::INVALID_ARGUMENT,
                    "Missing thread type");
    }

    if (test_input.thread_type() == EnclaveEntryCountTestInput::DONOTHING) {
      return Status::OkStatus();
    }

    active_entries++;
    if (test_input.thread_type() == EnclaveEntryCountTestInput::WAIT) {
      // Stays idle until the count finishes.
      {
        absl::MutexLock lock(&mu);
        if (!mu.AwaitWithTimeout(absl::Condition(&count_finished),
                                 absl::Seconds(kTimeout))) {
          return Status(error::GoogleError::INTERNAL,
                        "Timeout waiting for count");
        }
      }
    } else if (test_input.thread_type() == EnclaveEntryCountTestInput::COUNT) {
      if (!test_input.has_expected_entries()) {
        return Status(error::GoogleError::INVALID_ARGUMENT,
                      "Missing input extension");
      }
      // Wait till all threads has entered.
      {
        absl::MutexLock lock(&mu);
        if (!mu.AwaitWithTimeout(absl::Condition(
                                     +[](EnclaveEntryCountTestInput *input) {
                                       return active_entries ==
                                              input->expected_entries();
                                     },
                                     &test_input),
                                 absl::Seconds(kTimeout))) {
          return Status(error::GoogleError::INTERNAL,
                        "Timeout waiting for entries");
        }
      }
      // Counts the total active enclave entries saved by the enclave.
      output->SetExtension(number_entries, get_active_enclave_entries());
      count_finished = true;
    } else {
      return Status(error::GoogleError::INVALID_ARGUMENT,
                    "Unknown thread type");
    }
    return Status::OkStatus();
  }
};

TrustedApplication *BuildTrustedApplication() {
  return new EnclaveEntryCountTest;
}

}  // namespace asylo

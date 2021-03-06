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

#include "asylo/test/util/enclave_assertion_authority_configs.h"

#include "asylo/util/logging.h"
#include "asylo/identity/descriptions.h"
#include "asylo/identity/enclave_assertion_authority_config.pb.h"
#include "asylo/identity/sgx/sgx_local_assertion_authority_config.pb.h"

namespace asylo {

// The attestation domain is expected to be a 16-byte unique identifier.
const char kAttestationDomain[] = "A 16-byte string";

EnclaveAssertionAuthorityConfig GetNullAssertionAuthorityTestConfig() {
  EnclaveAssertionAuthorityConfig test_config;
  SetNullAssertionDescription(test_config.mutable_description());
  // No configuration needed for the null assertion authority.
  return test_config;
}

EnclaveAssertionAuthorityConfig GetSgxLocalAssertionAuthorityTestConfig() {
  EnclaveAssertionAuthorityConfig test_config;
  SetSgxLocalAssertionDescription(test_config.mutable_description());

  SgxLocalAssertionAuthorityConfig config;
  config.set_attestation_domain(kAttestationDomain);
  CHECK(config.SerializeToString(test_config.mutable_config()));

  return test_config;
}

}  // namespace asylo

/*
 *
 * Copyright 2018 Asylo authors
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
#include <iostream>
#include <string>
#include <vector>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_split.h"
#include "asylo/client.h"
#include "asylo/examples/hello_world/hello.pb.h"
#include "asylo/util/logging.h"
#include <signal.h>

asylo::Status status; 
asylo::EnclaveManager *manager;
asylo::EnclaveClient *client;
ABSL_FLAG(std::string, enclave_path, "", "Path to enclave to load");
ABSL_FLAG(std::string, names, "",
		"A comma-separated list of names to pass to the enclave");

struct sigaction old_sa;
struct sigaction new_sa;
int hello(int, char** );
int g_argc;
char **g_argv;

void signal_handler(int signo)
{
	std::cout<<"sigusr1 received"<<std::endl;
	asylo::EnclaveFinal final_input;
	status = manager->DestroyEnclave(client, final_input);
	if (!status.ok()) {
		//LOG(QFATAL) << "Destroy " << absl::GetFlag(FLAGS_enclave_path)
		LOG(INFO) << "Destroy " << absl::GetFlag(FLAGS_enclave_path)
			<< " failed: " << status;
	}

	std::cout << "restart main";
	hello(g_argc, g_argv);
	exit(0);
}

int main(int argc, char*argv[]){
	g_argc = argc;
	g_argv = argv;
	memset(&new_sa, 0, sizeof(new_sa));
	new_sa.sa_handler=&signal_handler;
	sigaction(SIGUSR1,&new_sa,&old_sa);	
	hello(argc, argv);
} 

int hello(int argc, char *argv[]) {
	// Part 0: Setup
	absl::ParseCommandLine(argc, argv);
	if (absl::GetFlag(FLAGS_names).empty()) {
		LOG(QFATAL) << "Must supply a non-empty list of names with --names";
	}
	std::vector<std::string> names =
		absl::StrSplit(absl::GetFlag(FLAGS_names), ',');
	// Part 1: Initialization
	asylo::EnclaveManager::Configure(asylo::EnclaveManagerOptions());
	auto manager_result = asylo::EnclaveManager::Instance();
	if (!manager_result.ok()) {
		LOG(QFATAL) << "EnclaveManager unavailable: " << manager_result.status();
	}
	manager = manager_result.ValueOrDie();
	std::cout << "Loading " << absl::GetFlag(FLAGS_enclave_path) << std::endl;
	asylo::SimLoader loader(absl::GetFlag(FLAGS_enclave_path), /*debug=*/true);
	status = manager->LoadEnclave("hello_enclave", loader);
	if (!status.ok()) {
		LOG(QFATAL) << "Load " << absl::GetFlag(FLAGS_enclave_path)
			<< " failed: " << status;
	}
	// Part 2: Secure execution
	client = manager->GetClient("hello_enclave");
	for (const auto &name : names) {
		asylo::EnclaveInput input;
		input.MutableExtension(hello_world::enclave_input_hello)
			->set_to_greet(name);
		asylo::EnclaveOutput output;
		status = client->EnterAndRun(input, &output);
		if (!status.ok()) {
			LOG(QFATAL) << "EnterAndRun failed: " << status;
		}
		if (!output.HasExtension(hello_world::enclave_output_hello)) {
			LOG(QFATAL) << "Enclave did not assign an ID for " << name;
		}
		std::cout << "Message from enclave: "
			<< output.GetExtension(hello_world::enclave_output_hello)
			.greeting_message()
			<< std::endl;
	}
	// Part 3: Finalization
	asylo::EnclaveFinal final_input;
	status = manager->DestroyEnclave(client, final_input);
	if (!status.ok()) {
		LOG(QFATAL) << "Destroy " << absl::GetFlag(FLAGS_enclave_path)
			<< " failed: " << status;
	}


	return 0;
}

#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_error.hpp"
#include "podman_t.hpp"
#include "podman_dump.hpp"
#include "podman_constants.hpp"
#include "podman_scheduler.hpp"
#include "podman_loop.hpp"

bool shouldExit = false;

const auto starttime = std::chrono::system_clock::now();

uint64_t timestamp(void) {
	const auto p1 = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(p1 - starttime).count();
}

int main() {

	std::cout << "podman_example: podman statistics displayer\n" <<
			"written by Oskari Rauta\n\n" <<
			"wait for initialization, this might take few seconds..\n" << std::endl;

/*
	// Debug logs:
	log_trace = true;
	log::output_level[log::verbose] = true;
	log::output_level[log::vverbose] = true;
	log::output_level[log::debug] = true;
*/

	Podman::podman_t *podman_data = new Podman::podman_t(Podman::init);

	std::cout << "podman_data initialized.\n" << std::endl;

	mutex.podman.lock();
	if ( podman_data -> status != Podman::RUNNING ) {
		std::cout << "podman_example: failed to retrieve podman stats, podman not running or socket unavailable.\n" <<
				"unix socket was searched from path " << Podman::API_SOCKET << "\n" << std::endl;
		mutex.podman.lock();
		free(podman_data);
		return -1;
	}
	mutex.podman.unlock();

	std::cout << "Podman system info:\n" << Podman::dump::system(podman_data) << std::endl;
	std::cout << "Podman networks:\n" << Podman::dump::networks(podman_data) << std::endl;
	std::cout << "Pods:\n" << Podman::dump::pods(podman_data) << std::endl;

	Podman::Scheduler scheduler(podman_data);

	std::cout << "cpu stats for containers:" << std::endl;

	while ( !shouldExit ) {

		std::string line;
		for ( int p = 0; p < podman_data -> pods.size(); p++ )
			for ( int c = 0; c < podman_data -> pods[p].containers.size(); c++ )
				line += ( line.empty() ? "" : " " ) + podman_data -> pods[p].containers[c].name + " " + podman_data -> pods[p].containers[c].cpu.text;

		if ( scheduler.nextTask() == Podman::Scheduler::Task::UPDATE_CONTAINERS )
			std::cout << line << std::endl;

		scheduler.run();
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
	};

	free(podman_data);
	return 0;
}

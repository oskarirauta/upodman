#include <thread>

#include "log.hpp"
#include "podman.hpp"

std::string libpod_version_override;

void Podman::init(Podman::podman_t *podman) {

	if ( !podman -> update_system())
		log::info << "Failed to fetch podman system info\n" << std::endl;
	else {
		log::debug << "Podman system info fetched" << std::endl;

		if ( !podman -> update_networks())
			log::info << "Failed to fetch podman networks" << std::endl;
		else
			log::debug << "Podman networks fetched" << std::endl;

		if ( !podman -> update_containers())
			log::info << "Failed to fetch podman containers" << std::endl;
		else
			log::debug << "Containers updated" << std::endl;

		if ( !podman -> update_pods())
			log::info << "Failed to update pods" << std::endl;
		else
			log::debug << "Pods updated" << std::endl;

		if ( !podman -> update_stats())
			log::vverbose << "Failed to fetch podman stats" << std::endl;
		else
			log::debug << "Container stats updated" << std::endl;

		if ( !podman -> update_logs())
			log::vverbose << "Failed to fetch podman logs" << std::endl;
		else
			log::debug << "Container logs retrieved" << std::endl;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

Podman::podman_t *podman_data;
Podman::Scheduler *podman_scheduler;

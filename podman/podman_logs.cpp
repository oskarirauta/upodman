#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

#include "common.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "constants.hpp"
#include "podman_t.hpp"
#include "podman_node.hpp"
#include "podman_query.hpp"
#include "podman_container.hpp"

bool Podman::Container::update_logs(Podman::Socket *socket) {

	// no logging needed since we are working on a copy of container

	if ( this -> isInfra ) {
		this -> logs = { "No logs available for infra containers." };
		return true;
	}

	if ( !this -> isRunning ) {
		if ( log_trace )
			log::debug << "logs not updated for container " << this -> name << ", container is not running" << std::endl;
		return true;
	}

	if ( this -> id.empty())
		return false;

	Podman::Query::Response response;
	Podman::Query query = { .group = "containers",
				.id = this -> id.empty() ? this -> name : this -> id,
				.action = "logs",
				.query = "follow=false&stderr=true&stdout=true&tail=" + std::to_string(CONTAINER_LOG_SIZE) + "&timestamps=false",
				.chunks_to_array = true,
				.parseJson = false };

	if ( !socket -> execute(query, response)) { // do not report error if log size is 0
		return response.body.length() == 0 ? true : false;
	}


	std::string body = response.body;
	std::vector<std::string> logs;

	while ( body.size() > 0 ) {

		body.erase(0, 1); // Some value is in the beginning
		int c_idx = 0;
		char stype, size1, size2, size3, size4;

		for ( char const &c: body ) {
			if ( c_idx == 0 ) stype = c;
			else if ( c_idx == 4 ) size1 = c;
			else if ( c_idx == 5 ) size2 = c;
			else if ( c_idx == 6 ) size3 = c;
			else if ( c_idx == 7 ) size4 = c;
			if ( ++c_idx > 7 ) break;
		}

		uint32_t len = 	(uint32_t)size1 << 24 |
				(uint32_t)size2 << 16 |
				(uint32_t)size3 << 8  |
				(uint32_t)size4;

		if ( c_idx < 8 || len < 1 ) break; // header missing
		body.erase(0, 8);
		if ( body.size() < len ) len = body.size();
		std::string entry = body.substr(0, len);
		body.erase(0, len);

		while ( entry.front() == '\n' || entry.front() == '\r' || entry.front() == '\t' ||
			entry.front() == '\f' || entry.front() == '\v' || entry.front() == ' ' ) entry.erase(0, 1);

		while ( entry.back() == '\n' || entry.back() == '\r' || entry.back() == '\t' ||
			entry.back() == '\f' || entry.back() == '\v' ||entry.back() == ' ' ) entry.pop_back();

		if ( !entry.empty())
			logs.push_back(entry);

	}

	this -> logs = logs;
	return true;
}

const bool Podman::podman_t::update_logs(void) {

	if ( !log_trace )
		log::debug << "updating logs" << std::endl;

	mutex.podman.lock();
	if ( this -> state.containers != Podman::Node::OK ) {
		log::vverbose << "error: failed to update logs, container array not ready" << std::endl;
		mutex.podman.unlock();
		return false;
	}

	std::vector<Podman::Pod> pods = this -> pods;
	mutex.podman.unlock();

	for ( int p = 0; p < pods.size(); p++ ) {
		for ( int c = 0; c < pods[p].containers.size(); c++ ) {
			if ( !pods[p].containers[c].update_logs(&this -> socket)) {
				log::vverbose << "error: failed to retrieve logs for " << pods[p].containers[c].name << std::endl;
				return false;
			}
		}
	}

	mutex.podman.lock();
	this -> pods = pods;
	mutex.podman.unlock();

	return true;
}

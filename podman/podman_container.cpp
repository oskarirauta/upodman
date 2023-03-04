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

Podman::Container::Container(std::string name, std::string id) {

	this -> name = name;
	this -> id = id;
	this -> image = "";
	this -> pod = "";
	this -> isInfra = false;
	this -> isRunning = false;
	this -> isRestarting = false;
	this -> pid = -1;
	this -> state = "";
	this -> startedAt = 0;
	this -> uptime = 0;
	this -> pids = std::vector<pid_t>();
	this -> logs = std::vector<std::string>();
	this -> cpu = Podman::Container::CpuStats();
	this -> ram = Podman::Container::MemoryStats();
}

void Podman::Container::copy_details(Podman::Container *other) {

	this -> pids.clear();
	this -> logs.clear();

	for ( int i = 0; i < other -> pids.size(); i++ )
		this -> pids.push_back(other -> pids[i]);

	for ( int i = 0; i < other -> logs.size(); i++ )
		this -> logs.push_back(other -> logs[i]);

	this -> cpu.percent = other -> cpu.percent;
	this -> cpu.text = other -> cpu.text;

	this -> ram.used = other -> ram.used;
	this -> ram.max = other -> ram.max;
	this -> ram.free = other -> ram.free;
	this -> ram.percent = other -> ram.percent;

	this -> busyState = other -> busyState;
}

const bool Podman::podman_t::update_containers(void) {

	if ( !log_trace )
		log::vverbose << "updating containers" << std::endl;

	Podman::Query::Response response;
	Podman::Query query = { .group = "containers", .action = "json", .query = "all=true" };

	mutex.podman.lock();

	if ( !socket.execute(query, response)) {
		this -> hash.containers = 0;
		this -> state.containers = Podman::Node::INCOMPLETE;
		log::debug << "update containers failed, containers array state set to incomplete" << std::endl;
		mutex.podman.unlock();
		return false;
	}

	uint64_t hashValue = response.hash();

	if ( hashValue == this -> hash.containers ) {
		this -> state.pods = Podman::Node::OK;
		this -> state.containers = Podman::Node::OK;
		mutex.podman.unlock();
		if ( log_trace )
			log::debug << "container state hash remained the same, success without updating pods and containers" << std::endl;
		return true;
	}

	if ( !json_object_is_type(response.json, json_type_array)) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		mutex.podman.unlock();
		return false;
	}

	mutex.podman.unlock();

	std::vector<Podman::Pod> new_pods;
	std::vector<Podman::Container> podless;

	int array_size = json_object_array_length(response.json);

	for ( int i = 0; i < array_size; i++ ) {

		Podman::Container new_cntr("", "");

		struct json_object *jvalue = json_object_array_get_idx(response.json, i);

		if ( !this -> parse_cntr(jvalue, new_cntr)) {
			log::debug << "container parser failure - skipped member #" << i << std::endl;
			continue;
		}

		mutex.podman.lock();
		for ( int _idx = 0; _idx < this -> pods.size(); _idx++ ) {
			if ( this -> pods[_idx].id == new_cntr.pod ) {
				for ( int _idx2 = 0; _idx2 < this -> pods[_idx].containers.size(); _idx2++ ) {
					if ( this -> pods[_idx].containers[_idx2].id == new_cntr.id ) {
						new_cntr.copy_details(&this -> pods[_idx].containers[_idx2]);
						break;
					}
				}
				break;
			}
		}
		mutex.podman.unlock();

		if ( new_cntr.pod.empty()) {

			podless.push_back(new_cntr);

		} else {

			int idx = -1;
			for ( int _idx = 0; _idx < new_pods.size(); _idx++ ) {
				if ( new_pods[_idx].id == new_cntr.pod ) {
					idx = _idx;
					break;
				}
			}

			if ( idx == -1 ) {

				Podman::Pod new_pod(new_cntr.pod, new_cntr.podName);

				mutex.podman.lock();
				for ( int idx2 = 0; idx2 < this -> pods.size(); idx2++ ) {
					if ( this -> pods[idx2].id == new_cntr.pod ) {
						new_pod.status = this -> pods[idx2].status;
						new_pod.isRunning = this -> pods[idx2].isRunning;
						break;
					}
				}
				mutex.podman.unlock();
				if ( new_cntr.isInfra ) new_pod.infraId = new_cntr.id;
				new_pod.containers.push_back(new_cntr);
				new_pods.push_back(new_pod);
			} else {
				new_pods[idx].containers.push_back(new_cntr);
				if ( new_cntr.isInfra ) new_pods[idx].infraId = new_cntr.id;
			}
		}
	}

	mutex.podman.lock();

	this -> pods = new_pods;

	if ( !podless.empty()) {
		Podman::Pod nonamePod("", "");
		nonamePod.containers = podless;
		this -> pods.push_back(nonamePod);
	}

	this -> hash.containers = hashValue;
	this -> state.pods = Podman::Node::OK;
	this -> state.containers = Podman::Node::OK;
	mutex.podman.unlock();

	return true;
}

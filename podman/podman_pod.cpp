#include <string>
#include <vector>
#include <algorithm>

#include "common.hpp"
#include "log.hpp"
#include "podman_t.hpp"
#include "podman_pod.hpp"

Podman::Pod::Pod(const std::string id, const std::string name) {

	this -> id = id;
	this -> name = name;
	this -> status = "unknown";
	this -> isRunning = false;
	this -> infraId = "";
	this -> containers = std::vector<Podman::Container>();
}

const int Podman::podman_t::podIndex(const std::string name) {

	std::lock_guard<std::mutex> guard(mutex.podman);

	for ( int i = 0; i < this -> pods.size(); i++ )
		if ( this -> pods[i].name == name )
			return i;

	return -1;
}

const bool Podman::podman_t::update_pods(void) {

	if ( !log_trace )
		log::vverbose << "updating pods" << std::endl;

	Podman::Query::Response response;
	Podman::Query query = { .group = "pods", .action = "json" };

	if ( !socket.execute(query, response))
		return false;

	uint64_t hashValue = response.hash();

	mutex.podman.lock();
	if ( hashValue == this -> hash.pods ) {
		mutex.podman.unlock();
		return true;
	}

	if ( !json_object_is_type(response.json, json_type_array)) {
		log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
		log::vverbose << "error: json result is not array" << std::endl;
		mutex.podman.unlock();
		return false;
	}

	bool changed = false;
	std::vector<Podman::Pod> pods = this -> pods;
	int array_size = json_object_array_length(response.json);

	mutex.podman.unlock();

	for ( int i = 0; i < array_size; i++ ) {

		struct json_object *elem = json_object_array_get_idx(response.json, i);
		if ( !json_object_is_type(elem, json_type_object)) {
			log::debug << "error while parsing pod details, element is not object" << std::endl;
			continue;
		}

		int podI = -1;
		if ( struct json_object *jid = json_object_object_get(elem, "Id"); json_object_is_type(jid, json_type_string)) {
			for ( int i2 = 0; i2 < pods.size() && podI == -1; i2++ )
				if ( pods[i2].id == std::string(json_object_get_string(jid)))
					podI = i2;
			if ( podI == -1 )
				log::debug << "warning, found pod details for unknown pod with id " << json_object_get_string(jid) << std::endl;
		} else {
			log::vverbose << "error while parsing pod details, object has no Id field" << std::endl;
			continue;
		}

		if ( struct json_object *jstatus = json_object_object_get(elem, "Status"); json_object_is_type(jstatus, json_type_string)) {
			std::string status(json_object_get_string(jstatus));
			if ( status.empty()) status = "unknown";
			pods[podI].status = status;
			pods[podI].isRunning = ( common::to_lower(status) == "running" ||
						common::to_lower(status) == "degraded" ) ? true : false;
		} else {
			pods[podI].status = "unknown";
			pods[podI].isRunning = false;
		}

		if ( struct json_object *jinfra = json_object_object_get(elem, "InfraId"); json_object_is_type(jinfra, json_type_string)) {
			std::string infraId(json_object_get_string(jinfra));
			if ( !infraId.empty())
				pods[podI].infraId = infraId;
		}

		changed = true;
	}

	mutex.podman.lock();

	if ( !changed ) {
		this -> hash.pods = 0;
		mutex.podman.unlock();
		return false;
	}

	this -> pods = pods;
	this -> hash.pods = pods.empty() ? 0 : hashValue;
	this -> state.pods = Podman::Node::OK;
	if ( pods.empty()) {
		this -> hash.containers = 0;
		if ( log_trace )
			log::debug << "pods array was empty, setting containers array hash to 0 to force refresh" << std::endl;
		this -> state.containers = Podman::Node::NEEDS_UPDATE;
	}

	mutex.podman.unlock();
	return true;
}

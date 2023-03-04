#include <string>
#include <vector>
#include <json-c/json.h>

#include "common.hpp"
#include "constants.hpp"
#include "podman_network.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_validate.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "podman_t.hpp"

Podman::podman_t::podman_t(std::string socket_path) {

	this -> needsSystemUpdate = true;
	this -> status = UNKNOWN;
	this -> version = "";
	this -> networks = std::vector<Podman::Network>();
	this -> pods = std::vector<Podman::Pod>();

	this -> state.networks = Podman::Node::INCOMPLETE;
	this -> state.pods = Podman::Node::INCOMPLETE;
	this -> state.containers = Podman::Node::INCOMPLETE;

	this -> hash.networks = 0;
	this -> hash.pods = 0;
	this -> hash.containers = 0;

	socket.timeout = Podman::API_TIMEOUT;
	if ( socket_path != Podman::API_SOCKET )
		this -> socket.path = socket_path;

}

Podman::podman_t::podman_t(void (*creator_func)(Podman::podman_t*), std::string socket_path) {

	this -> needsSystemUpdate = true;
	this -> status = UNKNOWN;
	this -> version = "";
	this -> networks = std::vector<Podman::Network>();
	this -> pods = std::vector<Podman::Pod>();

	this -> state.networks = Podman::Node::INCOMPLETE;
	this -> state.pods = Podman::Node::INCOMPLETE;
	this -> state.containers = Podman::Node::INCOMPLETE;

	this -> hash.networks = 0;
	this -> hash.pods = 0;
	this -> hash.containers = 0;

	socket.timeout = Podman::API_TIMEOUT;
	if ( socket_path != Podman::API_SOCKET )
		this -> socket.path = socket_path;

	creator_func(this);
}

const bool Podman::podman_t::update_system(void) {

	Podman::Query::Response response;
	Podman::Query query = { .action = "info" };

	if ( !socket.execute(query, response))
		return false;

	if ( !Podman::verifyJsonElements(response.json, { "host", "version" }, &query)) {
		log::debug << "podman info error, json validation mismatch" << std::endl;
		return false;
	}

	bool hostMissing = false;
	bool versionMissing = false;

	mutex.podman.lock();

	this -> needsSystemUpdate = false;

	if ( this -> status != RUNNING ) {

		if ( struct json_object *hostObj = json_object_object_get(response.json, "host"); json_object_is_type(hostObj, json_type_object)) {

			if ( struct json_object *jval = json_object_object_get(hostObj, "arch"); json_object_is_type(jval, json_type_string))
				this -> arch = std::string(json_object_get_string(jval));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "failed to retrieve host arch" << std::endl;
			}

			if ( struct json_object *jval = json_object_object_get(hostObj, "conmon"); json_object_is_type(jval, json_type_object)) {

				if ( struct json_object *jver = json_object_object_get(jval, "version"); json_object_is_type(jver, json_type_string)) {
					std::string conmon = std::string(json_object_get_string(jver));
					conmon = common::lines(conmon)[0];
					this -> conmon = common::lines(conmon, ',')[0];
				} else {
					this -> needsSystemUpdate = true;
					log::debug << "warning, failed to parse conmon version" << std::endl;
				}

			} else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, info missing conmon object" << std::endl;
			}

			std::string hostos, hostdist, ociversion;

			if ( struct json_object *jos = json_object_object_get(hostObj, "os"); json_object_is_type(jos, json_type_string))
				hostos = std::string(json_object_get_string(jos));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning. unable to retrieve host os" << std::endl;
			}

			if ( struct json_object *jdist = json_object_object_get(hostObj, "distribution"); json_object_is_type(jdist, json_type_object)) {

				if ( struct json_object *jdistname = json_object_object_get(jdist, "distribution"); json_object_is_type(jdistname, json_type_string))
					hostdist = std::string(json_object_get_string(jdistname));

				if (( hostos.empty() && hostdist.empty()) || hostdist.empty()) {
					this -> needsSystemUpdate = true;
					log::debug << "warning, unable to retrieve distribution" << std::endl;
				}

				if ( !hostos.empty() || !!hostdist.empty())
					this -> os = common::trim_ws(hostdist + ( hostdist.empty() ? "" : " " ) + hostos);
				else {
					this -> needsSystemUpdate = true;
					log::debug << "warning, failed to parse os and distribution identifiers" << std::endl;
				}

				if ( struct json_object *josvers = json_object_object_get(jdist, "version"); json_object_is_type(josvers, json_type_string))
					this -> os_version = std::string(json_object_get_string(josvers));
				else log::debug << "warning, failed to parse os version identifier" << std::endl;

			} else {

				log::debug << "warning, info missing distribution object" << std::endl;

				if ( !hostos.empty())
					this -> os = common::trim_ws(hostos);
			}

			if ( struct json_object *joci = json_object_object_get(hostObj, "ociRuntime"); json_object_is_type(joci, json_type_object)) {

				if ( struct json_object *jociver = json_object_object_get(joci, "version"); json_object_is_type(jociver, json_type_string))
					ociversion = std::string(json_object_get_string(jociver));
				else log::debug << "warning, failed to parse oci version" << std::endl;
			} else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, failed to parse ociRuntime" << std::endl;
			}

			if ( !ociversion.empty())
				this -> ociRuntime = common::lines(ociversion)[0];
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, ociRuntime is empty" << std::endl;
			}

			if ( struct json_object *jhostname = json_object_object_get(hostObj, "hostname"); json_object_is_type(jhostname, json_type_string))
				this -> hostname = std::string(json_object_get_string(jhostname));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, failed to parse hostname" << std::endl;
			}

			if ( struct json_object *jkernel = json_object_object_get(hostObj, "kernel"); json_object_is_type(jkernel, json_type_string))
				this -> kernel = std::string(json_object_get_string(jkernel));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, failed to parse kernel version" << std::endl;
			}

			if ( struct json_object *jmem = json_object_object_get(hostObj, "memTotal"); json_object_is_type(jmem, json_type_int))
				this -> memTotal = common::memToStr((double)json_object_get_uint64(jmem));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, failed to parse total system memory size" << std::endl;
			}

			if ( struct json_object *jmemfree = json_object_object_get(hostObj, "memFree"); json_object_is_type(jmemfree, json_type_int))
				this -> memFree = common::memToStr((double)json_object_get_uint64(jmemfree));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, failed to parse size of free memory" << std::endl;
			}

			if ( struct json_object *jswap = json_object_object_get(hostObj, "swapTotal"); json_object_is_type(jswap, json_type_int))
				this -> swapTotal = common::memToStr((double)json_object_get_uint64(jswap));
			else log::debug << "warning, failed to parse total system swap size" << std::endl;

			if ( struct json_object *jswapfree = json_object_object_get(hostObj, "swapFree"); json_object_is_type(jswapfree, json_type_int))
				this -> swapFree = common::memToStr((double)json_object_get_uint64(jswapfree));
			else log::debug << "warning, failed to parse size of free swap" << std::endl;

		} else {
			hostMissing = true;
			this -> needsSystemUpdate = true;
			log::debug << "warning, unable to retrieve host object" << std::endl;
		}

		if ( struct json_object *versObj = json_object_object_get(response.json, "version"); json_object_is_type(versObj, json_type_object)) {

			if ( struct json_object *japi = json_object_object_get(versObj, "APIVersion"); json_object_is_type(japi, json_type_string))
				this -> api_version = std::string(json_object_get_string(japi));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, unable to retrieve API version" << std::endl;
			}

			if ( struct json_object *jvers = json_object_object_get(versObj, "Version"); json_object_is_type(jvers, json_type_string))
				this -> version = std::string(json_object_get_string(jvers));
			else {
				this -> needsSystemUpdate = true;
				log::debug << "warning, unable to retrieve Podman version" << std::endl;
			}

		} else {
			versionMissing = true;
			this -> needsSystemUpdate = true;
			log::debug << "warning, unable to retrieve version object" << std::endl;
		}

		if ( !hostMissing && !versionMissing && !this -> needsSystemUpdate )
			this -> status = RUNNING;

	} else {

		if ( struct json_object *hostObj = json_object_object_get(response.json, "host"); json_object_is_type(hostObj, json_type_object)) {

			if ( struct json_object *jmemfree = json_object_object_get(hostObj, "memFree"); json_object_is_type(jmemfree, json_type_double))
				this -> memFree = common::memToStr(json_object_get_double(jmemfree));
			else log::debug << "warning, update failed to parse size of free memory" << std::endl;

			if ( struct json_object *jswapfree = json_object_object_get(hostObj, "swapFree"); json_object_is_type(jswapfree, json_type_double))
				this -> swapFree = common::memToStr(json_object_get_double(jswapfree));
			else log::debug << "warning, update failed to parse size of free swap" << std::endl;


		} else {
			hostMissing = true;
			this -> needsSystemUpdate = true;
			log::debug << "warning, unable to retrieve host object" << std::endl;
		}

	}

	if ( !hostMissing || !versionMissing )
		this -> status = RUNNING;

	bool needsSystemUpdate = this -> needsSystemUpdate; // access after unlocking

	mutex.podman.unlock();

	return ( hostMissing || versionMissing || needsSystemUpdate ) ? false : true;
}

const std::string Podman::podman_t::containerNameForID(const std::string id) {

	if ( id.empty())
		return "";

	bool done = false;
	std::string name = "";

	mutex.podman.lock();

	for ( const auto& pod : this -> pods ) {

		for ( const auto& cntr : pod.containers ) {

			if ( common::to_lower(cntr.id) == common::to_lower(id)) {
				name = cntr.name;
				done = true;
				break;
			}
		}

		if ( done )
			break;
	}

	mutex.podman.unlock();
	return name;
}

const bool Podman::podman_t::setContainerBusyState(const std::string name, Podman::BusyStat::Value state) {

	if ( name.empty())
		return false;

	bool done = false;

	mutex.podman.lock();
	for ( int podI = 0; podI < this -> pods.size(); podI++ ) {

		for ( int idx = 0; idx < this -> pods[podI].containers.size(); idx++ ) {

			if ( this -> pods[podI].containers[idx].name == name ) {
				this -> pods[podI].containers[idx].busyState = state;
				// log::debug << APP_NAME << ": container " << this -> pods[podI].containers[idx].name << " state is now: ";
				// log::debug << this -> pods[podI].containers[idx].busyState.description() << std::endl;
				done = true;
				break;
			}
		}

		if ( done )
			break;
	}

	mutex.podman.unlock();
	return done;
}

const bool Podman::podman_t::resetContainerBusyState(const std::string name) {

	if ( name.empty())
		return false;

	bool done = false;

	mutex.podman.lock();
	for ( int podI = 0; podI < this -> pods.size(); podI++ ) {

		for ( int idx = 0; idx < this -> pods[podI].containers.size(); idx++ ) {

			if ( this -> pods[podI].containers[idx].name == name ) {
				this -> pods[podI].containers[idx].busyState.reset();
				done = true;
				break;
			}
		}

		if ( done )
			break;

	}

	mutex.podman.unlock();
	return done;
}

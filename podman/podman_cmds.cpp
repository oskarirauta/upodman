#include <string>

#include "log.hpp"
#include "podman_busystat.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"
#include "podman_t.hpp"

const bool Podman::podman_t::container_stop(std::string name) {

	if ( !log_trace )
		log::vverbose << "stopping container " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "stop",
		.body = "ignore=true&timeout=2",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}

const bool Podman::podman_t::container_start(std::string name) {

	if ( !log_trace )
		log::vverbose << "starting container " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "start",
		.body = "dummyPostData=0",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}

const bool Podman::podman_t::container_restart(std::string name) {

	if ( !log_trace )
		log::vverbose << "restarting container " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "containers",
		.id = name,
		.action = "restart",
		.body = "t=2",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 204 ? true : false;
}


const bool Podman::podman_t::pod_stop(std::string name) {

	if ( !log_trace )
		log::vverbose << "stopping pod " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "stop",
		.body = "t=2",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 200 ? true : false;
}

const bool Podman::podman_t::pod_start(std::string name) {

	if ( !log_trace )
		log::vverbose << "starting pod " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "start",
		.body = "dummyPostData=0",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;

	return response.code == 200 ? true : false;
}

const bool Podman::podman_t::pod_restart(std::string name) {

	if ( !log_trace )
		log::vverbose << "restarting pod " << name << std::endl;

	Podman::Query::Response response;
	Podman::Query query = {
		.method = "POST",
		.group = "pods",
		.id = name,
		.action = "restart",
		.body = "dummyPostData=0",
		.parseJson = false
	};

	if ( !socket.execute(query, response))
		return false;


	return response.code == 200 ? true : false;
}

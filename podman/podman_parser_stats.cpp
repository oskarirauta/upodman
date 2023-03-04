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

const bool Podman::podman_t::parse_stats(struct json_object *json, Podman::Container &cntr) {

	if ( !cntr.isRunning ) {

		cntr.cpu.percent = 0;
		cntr.cpu.text = "";
		cntr.ram.used = 0;
		cntr.ram.max = 0;
		cntr.ram.free = 0;
		cntr.ram.percent = 0;

		cntr.pids = { -1 };
		cntr.pid = -1;

		return true;
        }

	double cpu, memLimit, memPerc, memUsage;

	if ( struct json_object *jval = json_object_object_get(json, "CPU"); json_object_is_type(jval, json_type_double))
		cpu = json_object_get_double(jval);
	else if ( struct json_object *jval = json_object_object_get(json, "CPU"); json_object_is_type(jval, json_type_int))
		cpu = (double)json_object_get_int(jval);
	else {
		log::debug << "stats format error, no CPU member" << std::endl;
		return false;
	}

	if ( struct json_object *jval = json_object_object_get(json, "MemLimit"); json_object_is_type(jval, json_type_double))
		memLimit = json_object_get_double(jval);
	else if ( struct json_object *jval = json_object_object_get(json, "MemLimit"); json_object_is_type(jval, json_type_int))
		memLimit = (double)json_object_get_int(jval);
	else {
		log::debug << "stats format error, no MemLimit member" << std::endl;
		return false;
	}

	if ( struct json_object *jval = json_object_object_get(json, "MemPerc"); json_object_is_type(jval, json_type_double))
		memPerc = json_object_get_double(jval);
	else if ( struct json_object *jval = json_object_object_get(json, "MemPerc"); json_object_is_type(jval, json_type_int))
		memPerc = (double)json_object_get_int(jval);
	else {
		log::debug << "stats format error, no MemPerc member" << std::endl;
		return false;
	}

	if ( struct json_object *jval = json_object_object_get(json, "MemUsage"); json_object_is_type(jval, json_type_double))
		memUsage = json_object_get_double(jval);
	else if ( struct json_object *jval = json_object_object_get(json, "MemUsage"); json_object_is_type(jval, json_type_int))
		memUsage = (double)json_object_get_int(jval);
	else {
		log::debug << "stats format error, no MemUsage member" << std::endl;
		return false;
	}

	cpu *= 10.0;
	int i_cpu = (int)cpu;
	double f_cpu = (double)i_cpu * 0.1;

	cntr.cpu.percent = f_cpu;
	cntr.cpu.text = std::to_string((int)f_cpu) + "%";

	cntr.ram.used = memUsage;
	cntr.ram.max = memLimit;
	cntr.ram.free = memLimit - memUsage;

	memPerc *= 10.0;
	int i_memPerc = (int)memPerc;
	cntr.ram.percent = (double)i_memPerc * 0.1;

	pid_t pid = -1;

	if ( struct json_object *jval = json_object_object_get(json, "PIDs"); json_object_is_type(jval, json_type_int)) {

		pid = (pid_t)json_object_get_int64(jval);

		if ( pid < 1 ) {
			log::debug << "stats format error, PIDs value is invalid" << std::endl;
			return false;
		}

		cntr.pids = { pid };
		cntr.pid = pid;

	} else if ( struct json_object *jval = json_object_object_get(json, "PIDs"); json_object_is_type(jval, json_type_array)) {

		pid_t npid;
		std::vector<pid_t> _pids;
		int pidsSize = json_object_array_length(jval);

		for ( int pidI = 0; pidI < pidsSize; pidI++ ) {

			npid = -1;
			struct json_object *jpid = json_object_array_get_idx(jval, pidI);

			if ( json_object_is_type(jpid, json_type_int))
				npid = (pid_t)json_object_get_int64(jval);

			if ( npid == 0 ) npid = -1;
			if ( npid < 1 ) continue;
			if ( pid < 1 ) pid = npid;
			_pids.push_back(npid);
		}

		if ( pid < 1 ) {
			log::debug << "stats format error, PIDs array is invalid" << std::endl;
			return false;
		}

		cntr.pid = pid;
		cntr.pids = _pids;

	} else {
		log::debug << "stats format error, PIDs missing or object is invalid" << std::endl;
		return false;
	}

	return true;
}

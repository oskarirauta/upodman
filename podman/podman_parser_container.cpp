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

const bool Podman::podman_t::parse_cntr(struct json_object *json, Podman::Container &cntr) {

	if ( struct json_object *jname = json_object_object_get(json, "Names"); json_object_is_type(jname, json_type_array))
		if ( struct json_object *jfirst = json_object_array_get_idx(jname, 0); json_object_is_type(jfirst, json_type_string))
			cntr.name = std::string(json_object_get_string(jfirst));

	if ( cntr.name.empty()) {
		log::debug << "Failed to parse container name" << std::endl;
		return false;
	}

	if ( struct json_object *jimage = json_object_object_get(json, "Image"); json_object_is_type(jimage, json_type_string)) {
		std::string img(json_object_get_string(jimage));
		cntr.image = img.empty() ? "unknown" : img;
	} else cntr.image = "unknown";

	if ( struct json_object *jcmd_arr = json_object_object_get(json, "Command"); json_object_is_type(jcmd_arr, json_type_array)) {

		cntr.command = "";
		int arr_size = json_object_array_length(jcmd_arr);

		for ( int i = 0; i < arr_size; i++ )
			if ( struct json_object *arg = json_object_array_get_idx(jcmd_arr, i); json_object_is_type(arg, json_type_string)) {
				std::string s(json_object_get_string(arg));
				if ( !s.empty())
					cntr.command += ( !cntr.command.empty() ? " " : "" ) + s;
			}
	}

	if ( struct json_object *jvalue = json_object_object_get(json, "Id"); json_object_is_type(jvalue, json_type_string))
		cntr.id = std::string(json_object_get_string(jvalue));

	if ( cntr.id.empty()) {
		log::debug << "Failed to parse container id" << std::endl;
		return false;
	}

	if ( struct json_object *jvalue = json_object_object_get(json, "Pod"); json_object_is_type(jvalue, json_type_string))
		cntr.pod = std::string(json_object_get_string(jvalue));

	if ( struct json_object *jvalue = json_object_object_get(json, "PodName"); json_object_is_type(jvalue, json_type_string))
		cntr.podName = std::string(json_object_get_string(jvalue));

	if ( struct json_object *jvalue = json_object_object_get(json, "IsInfra"); json_object_is_type(jvalue, json_type_boolean))
		cntr.isInfra = json_object_get_boolean(jvalue);

	if ( struct json_object *jvalue = json_object_object_get(json, "Pid"); json_object_is_type(jvalue, json_type_int))
		cntr.pid = json_object_get_int(jvalue);

	if ( struct json_object *jvalue = json_object_object_get(json, "State"); json_object_is_type(jvalue, json_type_string))
		cntr.state = common::to_lower(std::string(json_object_get_string(jvalue)));
	else cntr.state = "unknown";

	cntr.isRunning = common::to_lower(cntr.state) == "running" ? true : false;

	if ( struct json_object *jvalue = json_object_object_get(json, "Paused"); json_object_is_type(jvalue, json_type_boolean))
		if ( json_object_get_boolean(jvalue)) {
			cntr.isRunning = false;
			cntr.state = "paused";
		}

	if ( struct json_object *jvalue = json_object_object_get(json, "Restarting"); json_object_is_type(jvalue, json_type_boolean))
		cntr.isRestarting = json_object_get_boolean(jvalue);

	if ( struct json_object *jvalue = json_object_object_get(json, "StartedAt"); json_object_is_type(jvalue, json_type_int)) {
		cntr.startedAt = json_object_get_uint64(jvalue);

		time_t now = std::chrono::duration_cast<std::chrono::seconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();
		cntr.uptime = cntr.startedAt > 0 ? now - cntr.startedAt : 0;
	}

	return true;
}

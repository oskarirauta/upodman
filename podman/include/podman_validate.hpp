#pragma once

#include <string>
#include <vector>
#include <json-c/json.h>

#include "podman_query.hpp"

namespace Podman {

	bool verifyJsonElements(struct json_object *jsondata, std::vector<std::string> names, Podman::Query *query = NULL);

}

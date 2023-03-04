#include <iostream>

#include <string>
#include <vector>
#include <json-c/json.h>

#include "common.hpp"
#include "podman_constants.hpp"
#include "podman_query.hpp"

bool Podman::Query::Response::parseJson(void) {

	this -> error_msg = "";

	json_tokener_error err;

	if ( this -> json != NULL ) {
		free(this -> json);
		this -> json = NULL;
	}

	this -> json = json_tokener_parse_verbose(this -> body.c_str(), &err);
	if ( err != json_tokener_success && err != json_tokener_continue ) {
		if ( this -> json != NULL ) {
			free(this -> json);
			this -> json = NULL;
		}
		this -> error_msg = std::string(json_tokener_error_desc(err));
		return false;
	}

	return true;
}

bool Podman::Query::Response::parseJson(std::string json) {

	this -> error_msg = "";

	json_tokener_error err;

	if ( this -> json != NULL ) {
		free(this -> json);
		this -> json = NULL;
	}

	this -> json = json_tokener_parse_verbose(json.c_str(), &err);
	if ( err != json_tokener_success && err != json_tokener_continue ) {
		if ( this -> json != NULL ) {
			free(this -> json);
			this -> json = NULL;
		}
		this -> error_msg = std::string(json_tokener_error_desc(err));
		return false;
	}

	return true;
}

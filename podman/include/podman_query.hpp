#pragma once

#include <string>
#include <map>

#include <json-c/json.h>

#include "app.hpp"
#include "common.hpp"
#include "podman_constants.hpp"

namespace Podman {

	struct Query {

			std::string method = "";
			std::string group = "";
			std::string id = "";
			std::string action = "";
			std::string query = "";
			std::string mime = "";
			std::string body = "";

			int chunks_allowed = 0;
			bool chunks_to_array = false;
			bool parseJson = true;

			inline const std::string path(void) {

				return (
					( this -> group.empty() ? "" : ( "/" + this -> group )) +
					( this -> id.empty() ? "" : ( "/" + this -> id )) +
					( this -> action.empty() ? "" : ( "/" + this -> action )) +
					( this -> query.empty() ? "" : ( "?" + this -> query ))
				);
			}

			inline const std::string url(void) {

				std::string path = this -> path();
				if ( path.empty()) return "";

				std::string d = std::string("d");

				if ( !libpod_version_override.empty())
					d += "/" + libpod_version_override;
				else d += Podman::API_VERSION.empty() ? "" : ( "/" + Podman::API_VERSION );
				d += Podman::API_PATH.empty() ? "" : ( "/" + Podman::API_PATH );
				d += path;

				return d;
			}

		struct Response {

			std::string error_msg = "";
			int code = -1;

			int chunks = 0;
			int chunks_allowed = 0;
			bool chunks_to_array = false;
			std::string body = "";
			std::string raw = "";
			struct json_object *json = NULL;

			~Response() {

				if ( this -> json != NULL ) {
					free(this -> json);
					this -> json = NULL;
				}
			}

			bool parseJson(void);
			bool parseJson(std::string json);

			inline uint64_t hash(void) {
				uint64_t val = 0;
				for ( const char &c: this -> body)
					val += c;
				return val;
			}

			inline std::string jsonDump(void) {
				if ( this -> body.empty())
					return "";
				return std::string(json_object_to_json_string(this -> json));
			}

			inline void reset(void) {

				this -> error_msg = "";
				this -> code = -1;

				this -> chunks = 0;
				this -> chunks_allowed = 0;
				this -> chunks_to_array = false;
				this -> body = "";
				this -> raw = "";
			}

		};
	};

}

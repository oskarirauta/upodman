#include "common.hpp"
#include "log.hpp"
#include "podman_validate.hpp"

bool Podman::verifyJsonElements(struct json_object *jsondata, std::vector<std::string> names, Podman::Query *query) {

	for ( int i = 0; i < names.size(); i++ )

		if ( json_object_object_get(jsondata, names[i].c_str()) == NULL ) {

			if ( query != NULL )
				log::verbose << "failed to call: " << common::trim_leading(query -> path()) << std::endl;
			log::vverbose << "error: json element mismatch for element \"" << names[i] << "\"" << std::endl;
			return false;
		}

	return true;
}

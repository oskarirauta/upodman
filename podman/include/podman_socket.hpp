#pragma once

#include <unistd.h>
#include "curl/curl.h"
#include "podman_error.hpp"
#include "podman_constants.hpp"
#include "podman_query.hpp"

namespace Podman {

	struct Socket {

		public:

			int timeout = 10;
			Podman::Error error = Podman::Error::NO_ERROR;
			std::string path = Podman::API_SOCKET;

			inline Socket() {
				curl_global_init(CURL_GLOBAL_ALL);
			}

			inline ~Socket() {
				curl_global_cleanup();
			}

			bool execute(Podman::Query query, Podman::Query::Response& response);

	};

}

#pragma once

#include <string>
#include "podman_t.hpp"

namespace Podman {

	namespace dump {

		std::string system(Podman::podman_t *podman);
		std::string networks(Podman::podman_t *podman);
		std::string pods(Podman::podman_t *podman);

	}
}

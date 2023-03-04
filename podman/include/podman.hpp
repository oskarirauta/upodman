#pragma once

#include "podman_t.hpp"
#include "podman_scheduler.hpp"

namespace Podman {

	void init(Podman::podman_t *podman);

}

extern Podman::podman_t *podman_data;
extern Podman::Scheduler *podman_scheduler;

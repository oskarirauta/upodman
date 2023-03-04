#pragma once

namespace Podman {

	namespace Node {

		enum State : uint8_t {
			INCOMPLETE = 0,
			NEEDS_UPDATE,
			UNKNOWN,
			FAILED,
			OK
		};

	}
}

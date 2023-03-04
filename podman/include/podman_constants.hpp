#pragma once

namespace Podman {

	inline static const std::string API_SOCKET = "/run/podman/podman.sock";
	inline static const std::string API_HOST = "localhost";
	inline static const std::string API_VERSION = "v4.0.0";
	inline static const std::string API_PATH = "libpod";
	inline static const std::string API_USERAGENT = "SystemBus";
	inline static const std::string API_PROTOCOL = "HTTP/1.1";

	inline static const int API_TIMEOUT = 10;

}

extern std::string libpod_version_override;

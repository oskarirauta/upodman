#pragma once

#include <string>
#include <map>
#include <cstring>

namespace Podman {

	class Error {

		public:

			enum Value : int {
				NO_ERROR = 0,
				SOCKET_UNAVAILABLE,
				SOCKET_NOT_ACCEPTING,
				SOCKET_READONLY,
				SOCKET_NOT_RECEIVING,
				SOCKET_STREAM_UNAVAILABLE,
				SOCKET_CLOSED_EARLY,
				SOCKET_SEND_TIMEOUT,
				SOCKET_READ_TIMEOUT,
				SOCKET_UNKNOWN_ERROR,
				PROTOCOL_INVALID,
				PROTOCOL_MISMATCH,
				CONTENT_SIZE_INVALID,
				CHUNK_SIZE_INVALID,
				CHUNK_SIZE_MISMATCH,
				CONTENT_SIZE_MISMATCH,
				JSON_PARSE_FAILED,
			};

			Error() = default;
			constexpr Error(Value aError) : value(aError) { }

			constexpr operator bool() const { return this -> value != Podman::Error::NO_ERROR; }
			constexpr bool operator==(Podman::Error a) const { return this -> value == a.value; }
			constexpr bool operator==(Podman::Error::Value a) const { return this -> value == a; }
			constexpr bool operator!=(Podman::Error a) const { return this -> value != a.value; }
			constexpr bool operator!=(Podman::Error::Value a) const { return this -> value != a; }

			inline std::string description() const {
				if ( Podman::Error::Description.count(this -> value))
					return Podman::Error::Description.at(this -> value) + ( this -> value != NO_ERROR && this -> _extended[0] == 0 ? "" : ( ": " + std::string(this -> _extended)));
				else return "Unknown error";
			}

			inline void setExtended(const std::string description) {
				bzero(this -> _extended, sizeof(this -> _extended));
				if ( !description.empty()) {
					std::string _description = description;
					if ( _description.length() > 255 )
						_description.resize(255);
					 strcpy(this -> _extended, _description.c_str());
				}
			}

			inline void clearExtended(void) {
				bzero(this -> _extended, sizeof(this -> _extended));
			}

			inline std::string extended() const {
				return std::string(this -> _extended);
			}

			inline void reset(void) {
				this -> value = Podman::Error::NO_ERROR;
				bzero(this -> _extended, sizeof(this -> _extended));
			}

		private:

			Value value;
			char _extended[256] = "";

			static inline std::map<Podman::Error::Value, std::string> Description = {
				{ NO_ERROR, "No error" },
				{ SOCKET_UNAVAILABLE, "socket is not available" },
				{ SOCKET_NOT_ACCEPTING, "socket did not accept connection" },
				{ SOCKET_READONLY, "socket is read-only" },
				{ SOCKET_NOT_RECEIVING, "socket does not receive" },
				{ SOCKET_STREAM_UNAVAILABLE, "socket is not sending data" },
				{ SOCKET_CLOSED_EARLY, "socket stream closed too early" },
				{ SOCKET_SEND_TIMEOUT, "socket send timed out" },
				{ SOCKET_READ_TIMEOUT, "socket read timed out" },
				{ SOCKET_UNKNOWN_ERROR, "undescribed error with socket" },
				{ PROTOCOL_INVALID, "invalid protocol" },
				{ PROTOCOL_MISMATCH, "protocol did not validate" },
				{ CONTENT_SIZE_INVALID, "content size is invalid" },
				{ CHUNK_SIZE_INVALID, "chunk size is invalid" },
				{ CHUNK_SIZE_MISMATCH, "chunk size did not match" },
				{ CONTENT_SIZE_MISMATCH, "content size did not match" },
				{ JSON_PARSE_FAILED, "json parse failed" },
			};

	};

}

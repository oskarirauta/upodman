#include <iostream>

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <map>

#include "curl/curl.h"

#include <sys/un.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "common.hpp"
#include "mutex.hpp"
#include "log.hpp"
#include "app.hpp"
#include "podman_error.hpp"
#include "podman_query.hpp"
#include "podman_socket.hpp"

/*
TODO:

 - check if some of headers are unnecessary

*/

static size_t receiveCallback(void *contents, size_t size, size_t nmemb, void *response) {

	size_t realsize = size * nmemb;
	std::string data(static_cast<const char *>(contents), realsize);
	Podman::Query::Response *ptr = (Podman::Query::Response*)response;

	ptr -> raw += data;
	ptr -> chunks++;

	if ( log_trace && ptr -> chunks_allowed != 0 )
		log::debug << "received chunk #" << ptr -> chunks << std::endl;

	if ( ptr -> chunks_to_array )
		ptr -> body += std::string( ptr -> chunks == 1 ? "[" : "," ) + data;
	else ptr -> body += data;

	if ( ptr -> chunks_allowed > 0 && ptr -> chunks >= ptr -> chunks_allowed ) {
		ptr -> body += "]";
		return 0;
	}

	return realsize;
}


bool Podman::Socket::execute(Podman::Query query, Podman::Query::Response &response) {

	response.reset();
	response.chunks_allowed = query.chunks_allowed;
	response.chunks_to_array = query.chunks_to_array;

	CURL *curl = curl_easy_init();

	if ( !curl ) {
		this -> error = Podman::Error::SOCKET_UNAVAILABLE;
		log::vverbose << "error with socket when querying " << common::trim_leading(query.path()) << ": socket not available" << std::endl;
		curl_easy_cleanup(curl);
		return false;
	}

	if ( log_trace )
		log::debug << "socket: " << this -> path << " query: " << query.url() << std::endl;

	bool methodPost = common::to_lower(query.method) == "post" ? true : false;

	curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, this -> path.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, query.url().c_str());
	curl_easy_setopt(curl, CURLOPT_POST, methodPost ? 1 : 0);

	if ( methodPost )
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query.body.c_str());

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

	curl_easy_setopt(curl, CURLOPT_TIMEOUT, common::trim_leading(query.path()) == "containers/stats" ? 25 : this -> timeout);

	CURLcode res = curl_easy_perform(curl);

	if ( !methodPost && response.chunks_allowed > 0 && response.chunks >= response.chunks_allowed &&
		res == CURLE_WRITE_ERROR ) res = CURLE_OK;

	if ( res != CURLE_OK ) {

		switch ( res ) {
			CURLE_UNSUPPORTED_PROTOCOL:
				this -> error = Podman::Error::PROTOCOL_INVALID;
				break;
			CURLE_FAILED_INIT:
				this -> error = Podman::Error::SOCKET_UNAVAILABLE;
				break;
			CURLE_COULDNT_CONNECT:
				this -> error = Podman::Error::SOCKET_NOT_ACCEPTING;
				break;
			CURLE_REMOTE_ACCESS_DENIED:
				this -> error = Podman::Error::SOCKET_READONLY;
				break;
			CURLE_PARTIAL_FILE:
				this -> error = Podman::Error::CONTENT_SIZE_INVALID;
				break;
			CURLE_HTTP_RETURNED_ERROR:
				this -> error = Podman::Error::PROTOCOL_MISMATCH;
				break;
			CURLE_OPERATION_TIMEDOUT:
				this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
				break;
			CURLE_GOT_NOTHING:
				this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
				break;
			CURLE_SEND_ERROR:
				this -> error = Podman::Error::SOCKET_SEND_TIMEOUT;
				break;
			CURLE_RECV_ERROR:
				this -> error = Podman::Error::SOCKET_READ_TIMEOUT;
				break;
			CURLE_BAD_CONTENT_ENCODING:
				this -> error = Podman::Error::CONTENT_SIZE_MISMATCH;
				break;
			CURLE_AGAIN:
				this -> error = Podman::Error::SOCKET_STREAM_UNAVAILABLE;
				break;
			CURLE_CHUNK_FAILED:
				this -> error = Podman::Error::CHUNK_SIZE_MISMATCH;
				break;

			default:
				this -> error = Podman::Error::SOCKET_UNKNOWN_ERROR;
				this -> error.setExtended("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
		}

		log::vverbose << "socket failed with error while querying for " << common::trim_leading(query.path()) << ": " << this -> error.description() << std::endl;
		curl_easy_cleanup(curl);
		return false;
	}

	long response_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
	response.code = (int)response_code;

	curl_easy_cleanup(curl);

	if ( log_trace )
		std::cout << "data from socket, code: " << response.code << "\n" << response.body << "\n" << std::endl;

	if ( response.body.length() != 0 ) {

		if ( query.parseJson && !response.parseJson()) {
			this -> error = Podman::Error::JSON_PARSE_FAILED;
			this -> error.setExtended(response.error_msg);
			response.error_msg = "";
			log::verbose << "failed to call: " << common::trim_leading(query.path()) << std::endl;
			log::vverbose << "error: " << this -> error.description() << std::endl;
			return false;
		}

		return true;
	} else if ( log_trace ) log::vverbose << "received response from socket had size of 0, received nothing for query " << common::trim_leading(query.path()) << std::endl;

	return false;
}

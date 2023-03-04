#pragma once

#include <string>

extern "C" {
#include <libubox/blobmsg_json.h>
#include <libubus.h>
}

enum {
	UBUS_STOPPED = 0,
	UBUS_RUNNING,
	__UBUS_STATE_MAX
};

extern std::string ubus_socket;
extern struct ubus_context *ctx;
extern struct blob_buf b;

int ubus_create(void);

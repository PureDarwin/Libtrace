#include <stdio.h>
#include <xpc/xpc.h>
#include "logd_file_format.h"

extern void logd_handle_submission(xpc_object_t submission);

static void logd_handle_general(xpc_object_t message, xpc_object_t reply) {
	// FIXME: Implement
}

int main(int argc, const char * argv[]) {
	xpc_connection_t submissionConnection = xpc_connection_create_mach_service("com.apple.log.events", dispatch_get_main_queue(), XPC_CONNECTION_MACH_SERVICE_LISTENER);
	xpc_connection_set_event_handler(submissionConnection, ^(xpc_object_t _Nonnull object) {
		logd_handle_submission(object);
	});
	xpc_connection_resume(submissionConnection);

	xpc_connection_t generalConnection = xpc_connection_create_mach_service("com.apple.log", dispatch_get_main_queue(), XPC_CONNECTION_MACH_SERVICE_LISTENER);
	xpc_connection_set_event_handler(generalConnection, ^(xpc_object_t _Nonnull object) {
		xpc_object_t reply = xpc_dictionary_create_reply(object);
		logd_handle_general(object, reply);
		xpc_connection_send_message(xpc_dictionary_get_remote_connection(object), reply);
		xpc_release(reply);
	});
	xpc_connection_resume(generalConnection);

	dispatch_main();
	return EXIT_FAILURE;
}

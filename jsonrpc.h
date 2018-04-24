#ifndef JSONRPC_H_
#define JSONRPC_H_

#include <string.h>
#include <jansson.h>

// Predefined error codes
#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS -32602
#define JSONRPC_INTERNAL_ERROR -32603

typedef int (*jsonrpc_method_prototype)(json_t *json_params, json_t **result, void *userdata);

typedef struct jsonrpc_method_entry_t {
	const char *name;
	jsonrpc_method_prototype funcptr;
	const char *params_spec;
} jsonrpc_method_entry_t;

json_t *jsonrpc_error_object_predefined(int code, json_t *data);
json_t* jsonrpc_error_response(json_t *json_id, json_t *json_error);
char* jsonrpc_handler(const char* json_data, size_t json_data_sz, jsonrpc_method_entry_t method_table[], void *userdata);

#endif /* JSONRPC_H_ */

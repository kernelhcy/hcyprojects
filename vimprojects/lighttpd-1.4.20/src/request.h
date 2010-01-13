#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "server.h"

/**
 * 如果有POST的数据需要读取，则返回1.否则返回0.
 */
int http_request_parse(server * srv, connection * con);

int http_request_header_finished(server * srv, connection * con);

#endif

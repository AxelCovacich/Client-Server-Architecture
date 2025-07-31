#ifndef SESSION_HANDLER_H
#define SESSION_HANDLER_H

#include "transport.h"

int start_communication(int sockfd, recv_fn rx, send_fn tx);
void *keepalive_thread_func(void *arg);

#endif // SESSION_HANDLER_H
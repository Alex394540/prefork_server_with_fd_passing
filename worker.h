#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <errno.h>
#include <string.h>
#include "http.h"

extern const char * SERVE_DIRECTORY;

void start_worker_loop(int passed_fd);

#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "http.h"

void run_server(const char * ip, int port, const char * dir_path);

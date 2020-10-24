#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>


struct master_connection_info {
    struct sockaddr_in address;
    int fd;
};

struct process_ipc_info {
    pid_t pid;
    int fd;
};

void spawn_worker(struct process_ipc_info * process_info);

void prepare_for_listening(const char * ip, int port, struct master_connection_info * conn_info);

void start_master_loop(int master_socket, struct sockaddr_in * address, int * worker_pids, int * worker_fds, int workers_amount);

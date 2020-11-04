#include "string.h"
#include "master.h"
#include "worker.h"


ssize_t sock_fd_write(int sock, void * buf, ssize_t buflen, int fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union { struct cmsghdr cmsghdr; char control[CMSG_SPACE(sizeof(int))]; } cmsgu;
    struct cmsghdr * cmsg;

    if (buflen < 0) {
        fprintf(stderr, "Buffer length should be at least 1 byte!\n");
        exit(EXIT_FAILURE);
    }

    iov.iov_base = buf; 
    iov.iov_len = buflen;
    msg.msg_name = NULL; 
    msg.msg_namelen = 0; 
    msg.msg_iov = &iov; 
    msg.msg_iovlen = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        *((int *) CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
    }

    size = sendmsg(sock, &msg, 0); 
    if (size < 0) {
        perror("sendmsg");
    }
    return size;
}


void spawn_worker(struct process_ipc_info * process_info) {    
    pid_t pid;
    int fd[2];
    const int parent_socket = 0;
    const int child_socket = 1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    process_info->pid = pid;
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child
        close(fd[parent_socket]);
        process_info->fd = fd[child_socket];
    } else {
        // parent
        printf("Worker was created! Pid - %d\n", pid);
        close(fd[child_socket]);
        process_info->fd = fd[parent_socket];
    }
}


void prepare_for_listening(const char * ip, int port, struct master_connection_info * connection_struct) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int max_queued = 1000;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, max_queued) < 0) 
    {
        perror("listen"); 
        exit(EXIT_FAILURE); 
    }
    
    connection_struct->address = address;
    connection_struct->fd = server_fd;
}


void start_master_loop(int master_socket, struct sockaddr_in * address, int * worker_pids, int * worker_fds, int workers_amount) {
    printf("Master: Starting master looping.\n");
    int slave_socket, addrlen;
    addrlen = sizeof(*address);
    int requests = 0;
    int sockets_to_close[1000];
    memset(sockets_to_close, -1, 1000);
    while (1) {
        if ((slave_socket = accept(master_socket, (struct sockaddr *)address,  
                        (socklen_t*)&addrlen))<0) 
        { 
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Plain round robbin algorthm
        int worker_num = (requests++) % workers_amount;

        // Some data should be sent, otherwise socket wont be passed
        char buf[1];
        ssize_t sendmsg_result = sock_fd_write(worker_fds[worker_num], buf, 1, slave_socket);
        
        if (requests > 500) {
            shutdown(sockets_to_close[(requests - 500) % 1000], SHUT_WR);
            close(sockets_to_close[(requests - 500) % 1000]);
        }
        
        sockets_to_close[requests % 1000] = slave_socket;
    }
}
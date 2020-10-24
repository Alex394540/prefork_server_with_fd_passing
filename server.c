#include "server.h"
#include "master.h"
#include "worker.h"


const char * SERVE_DIRECTORY;


void run_server(const char * ip, int port, const char * dir_path) 
{
    pid_t process_id = 0;
    pid_t sid = 0;
    process_id = fork();
    if (process_id < 0)
    {
        printf("Daemonizing failed on fork!\n");
        exit(1);
    }
    if (process_id > 0)
    {
        printf("Process id = %d, daemonizing...\n", process_id);
        exit(0);
    }

    umask(0);
    sid = setsid();
    if(sid < 0)
    {
        printf("Error while creation new session! Sid < 0\n");
        exit(1);
    }
    // chdir("/");
    // close(STDIN_FILENO);
    // // close(STDOUT_FILENO);
    // // close(STDERR_FILENO);

    int slave_socket;
    SERVE_DIRECTORY = dir_path;

    const int WORKERS_AMOUNT = get_nprocs_conf() - 1;
    // const int WORKERS_AMOUNT = 1;
    int worker_pids[WORKERS_AMOUNT];
    int worker_fd[WORKERS_AMOUNT];
    int current_worker_fd;
    int is_master = 1;
    int worker_number = 0;
    while (worker_number < WORKERS_AMOUNT) {
        struct process_ipc_info process_info;
        spawn_worker(&process_info);
        if (process_info.pid != 0) {
            worker_pids[worker_number] = process_info.pid;
            worker_fd[worker_number] = process_info.fd;
            ++worker_number;
        } else {
            current_worker_fd = process_info.fd;
            is_master = 0;
            memset(worker_pids, -1, WORKERS_AMOUNT);
            memset(worker_fd, -1, WORKERS_AMOUNT);
            break;
        }
    }

    if (is_master) {
        struct master_connection_info connection_info;
        prepare_for_listening(ip, port, &connection_info);
        printf("Workers amount = %d\n", WORKERS_AMOUNT);
        start_master_loop(connection_info.fd, &connection_info.address, worker_pids, worker_fd, WORKERS_AMOUNT);
    } else {
        start_worker_loop(current_worker_fd);
    }
}

# prefork_server_with_fd_passing
Restricted version of http server.
Prefork model with file descriptor passing from master to worker was used.

Usage example:
./webserver -h 127.0.0.1 -p 5000 -d web/html/

where:
-h - ip address
-p - port
-d - served directory

Tested only for Linux.

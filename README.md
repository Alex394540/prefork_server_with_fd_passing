# prefork_server_with_fd_passing
Restricted version of http server. </br>
Prefork model with file descriptor passing from master to worker was used. </br>

Usage example: </br>
./webserver -h 127.0.0.1 -p 5000 -d web/html/

where: </br>
-h - ip address </br>
-p - port </br>
-d - served directory </br>

Tested only for Linux.

#include "worker.h"

const int MAX_RESPONSE_SIZE = 65536;


char * trim_end_slash(const char * dir_path)
{
    int path_length = strlen(dir_path);
    int new_path_length;
    if (dir_path[path_length - 1] == '/') {
        new_path_length = path_length - 1;
    } else {
        new_path_length = path_length;
    }
    char * new_dir_path = malloc(sizeof(char) * new_path_length + 1);
    for (int i = 0; i < new_path_length; ++i)
    {
        new_dir_path[i] = dir_path[i];
    }
    new_dir_path[new_path_length] = '\0';
    return new_dir_path;
}


char * trim_get_params(const char * path)
{
    int i = 0;
    for (; i < strlen(path); i++) {
        if (path[i] == '?') {
            break;
        }
    }
    int trimmed_path_length = i;
    char * trimmed_path = malloc(sizeof(char) * i + 1);
    for (int i = 0; i < trimmed_path_length; ++i) {
        trimmed_path[i] = path[i];
    }
    trimmed_path[trimmed_path_length] = '\0';
    return trimmed_path;
}


char * get_file_path(const char * base_dir, const char * request_path) {
    // printf("Base dir: '%s', request_path: '%s'\n", base_dir, request_path);
    char * trimmed_relative_path = trim_get_params(request_path);
    // printf("Trimmed relative path %s\n", trimmed_relative_path);
    const char * relative_path;
    if (strcmp(trimmed_relative_path, "/")) {
        relative_path = trimmed_relative_path;
    } else {
        relative_path = "/index.html";
    }

    char * base_dir_trimmed_path = trim_end_slash(base_dir);
    // printf("Base dir trimmed path: '%s', relative_file: '%s'\n", base_dir_trimmed_path, relative_path);
    char * path_to_file = malloc(sizeof(char) * 128);
    strcpy(path_to_file, base_dir_trimmed_path);
    strcat(path_to_file, relative_path);
    
    free(base_dir_trimmed_path);
    free(trimmed_relative_path);
    return path_to_file;
}


void parse_http_request(const char * request_string, struct http_request * request)
{
    // printf("Request string with length = %ld\n", strlen(request_string));

    char http_params[5][64];
    int token_num = 0;
    int position = 0;
    int buf_index = 0;
    char buf[64];
    memset(buf, 0, 64);
    while (position < strlen(request_string) && token_num < 5) {
        if (request_string[position] == ' ' || request_string[position] == '\n' || request_string[position] == '\r') {
            if (buf_index) {
                buf[buf_index + 1] = '\0';
                strcpy(http_params[token_num++], buf);
                memset(buf, 0, 64);
                buf_index = 0;
            }
        } else {
            buf[buf_index++] = request_string[position];
        }
        ++position;
    }
    
    char * method = http_params[0];
    char * path = http_params[1];
    char * version = http_params[2];
    char * host_header = http_params[3];
    char * host_value = http_params[4];

    if (method == NULL || strcmp(method, "GET")) 
    {
        fprintf(stderr, "Wrong HTTP request! Only HTTP method <<GET>> is supported! Current method = '%s'\n", method);
    } else if (path == NULL) {
        fprintf(stderr, "HTTP path cant be empty!\n");
    } else if (version == NULL) {
        fprintf(stderr, "HTTP version should not be empty!\n");
    } else if (host_header == NULL) {
        fprintf(stderr, "HTTP host should not be empty!\n");
    } else if (host_value == NULL) {
        fprintf(stderr, "HTTP host value should not be empty!\n");
    } else {
        // printf("Host=%s, path=%s, version=%s\n", host_value, path, version);
        strcpy(request->host, host_value);
        strcpy(request->path, path);
        strcpy(request->version, version);
    }
}


void process_parsed_http_request(const struct http_request * request, struct http_response * response)
{   
    int status_code = 404;
    char * status_message = "Not found";
    char * response_body = NULL;
    char * path_to_file = get_file_path(SERVE_DIRECTORY, request->path);
    FILE * fptr = fopen(path_to_file, "rb");

    // TODO refactor checkings
    if (fptr != NULL)
    {
        // Check if directory was not requested
        if ((fseek(fptr, 0, SEEK_END) >= 0)) {
            int length = ftell(fptr);
            if (length >= 0) {
                status_code = 200;
                status_message = "Ok";
                fseek(fptr, 0, SEEK_SET);
                response_body = malloc(sizeof(char) * length);
                memset(response_body, 0, length);
                if (length)
                {
                    fread(response_body, 1, length, fptr);
                }
            } else {
                perror("ftell");
            }
        } else {
            perror("fseek");
        }
        fclose(fptr);
    }
    
    // printf("Setting status %d, message %s for file '%s'!\n\n", status_code, status_message, path_to_file);
    free(path_to_file);

    strcpy(response->version, request->version);
    response->status_code = status_code;
    strcpy(response->status_message, status_message);
    // TODO - make variables constant, dont hardcode
    strcpy(response->content_type, "text/html; charset=utf-8");
    if (response_body != NULL) {
        response->content_length = strlen(response_body);
        strcpy(response->response_body, response_body);
        free(response_body);
    } else {
        response->content_length = 0;
        strcpy(response->response_body, "");
    }
}


void build_http_response_string(const struct http_response * response, char * response_string)
{
    char startline[256];
    char content_type[128];
    char content_length[64];
    const char * server;
    const char * date;
    const char * last_modified;
    const char * connection;
    const char * accept_ranges;
    const char * response_body;

    sprintf(startline, "%s %d %s", response->version, response->status_code, response->status_message);

    // TODO - get from variables/special functions
    server = "Server: Custom server/1.0";
    date = "Date: Sat, 08 Mar 2014 22:52:46 GMT";
    sprintf(content_type, "Content-Type: %s", response->content_type);
    sprintf(content_length, "Content-Length: %d", response->content_length);

    last_modified = "Last-Modified: Sat, 08 Mar 2014 22:53:30 GMT";
    connection = "Connection: keep-alive";
    accept_ranges = "Accept-Ranges: bytes";
    response_body = response->response_body;

    sprintf(response_string, 
            "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\n%s", 
            startline, 
            server, 
            date, 
            content_type, 
            content_length, 
            last_modified, 
            connection,
            accept_ranges, 
            response_body);
}


ssize_t sock_fd_read(int sock, void *buf, ssize_t buflen, int *fd){
    ssize_t size;
    if (fd){
        struct msghdr msg;
        struct iovec iov;
        union { struct cmsghdr cmsghdr; char control[CMSG_SPACE(sizeof(int))];} cmsgu;
        struct cmsghdr *cmsg;

        iov.iov_base = buf;
        iov.iov_len = buflen;
        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        
        // printf("Worker %d: Prepare to recvmsg from sock %d\n", getpid(), sock);
        size = recvmsg(sock, &msg, 0);
        // printf("Worker %d: Successfully recvmsg from sock %d!\n", getpid(), sock);
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))){
            // printf("Worker %d: Ok, cmsg seems ok\n", getpid());
            if (cmsg->cmsg_level != SOL_SOCKET){
                fprintf(stderr,"invalid cmsg_level %d\n", cmsg->cmsg_level);
                exit(EXIT_FAILURE);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS){
                fprintf(stderr, "invalid cmsg_type %d\n", cmsg->cmsg_type);
                exit(EXIT_FAILURE);
            }
            *fd = *((int *) CMSG_DATA(cmsg));
            // printf("Worker %d: received fd %d\n", getpid(), *fd);
        } else {
            // printf("Worker %d: fd=-1, cmsg error!\n", getpid());
            if (cmsg) {
                // printf("Worker %d: cmsg->cmsg_len=%ld, CMSG_LEN(sizeof(int))=%ld\n", getpid(), cmsg->cmsg_len, CMSG_LEN(sizeof(int)));
            } else {
                // printf("Worker %d: cmsg pointer is NULL!\n", getpid());
            }
            // printf("Worker %d: something failed, setting fd to -1\n", getpid());
            *fd=-1;
        }
    } else {
        // printf("Worker: %d. No fd was passed! Simple read from sock %d\n", getpid(), sock);
        size = read(sock, buf, buflen);
        if (size < 0){
           perror("read");
           exit(EXIT_FAILURE);
        }
    }
    return size;
};


void handle_http_request(int slave_socket) {
    // printf("Handling http request with socket %d\n", slave_socket);
    char buffer[65536];
    memset(buffer, 0, 65536);
    if (read(slave_socket, buffer, 65536) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    
    // printf("\nRead: \n%s\n", buffer);
    if (strlen(buffer) != 0) {
        struct http_request * parsed_request = malloc(sizeof(struct http_request));
        struct http_response * built_response = malloc(sizeof(struct http_response));
        char built_response_string[MAX_RESPONSE_SIZE];
        
        parse_http_request(buffer, parsed_request);
        // printf("Worker %d: Request was parsed, path: %s, host: %s, version: %s\n", getpid(), parsed_request->path, parsed_request->host, parsed_request->version);
        process_parsed_http_request(parsed_request, built_response);
        // printf("Worker %d: Request processed! Status code: %d, status message: %s\n", getpid(), built_response->status_code, built_response->status_message);
        build_http_response_string(built_response, built_response_string);
        // printf("%s\n", built_response_string);
        // printf("Worker %d: Response string with length=%ld\n", getpid(), strlen(built_response_string));

        int bytes_sent = 0;
        if ((bytes_sent = send(slave_socket, built_response_string, strlen(built_response_string), MSG_NOSIGNAL)) < 0) {
            perror("send");
            exit(EXIT_FAILURE);
        } else {
            // printf("Sent %d bytes\n", bytes_sent);
        }
        
        free(built_response);
        free(parsed_request);
    } else {
        fprintf(stderr,"0 bytes were read from socket!\n");
    }
}


void start_worker_loop(int worker_fd) {
    int pid = getpid();
    while (1) {
        // printf("Worker %d: Waiting for new request\n", pid);
        char buf[5];
        int received_socket_fd, fd_read_result;
        fd_read_result = sock_fd_read(worker_fd, buf, 1, &received_socket_fd);
        // printf("Worker %d: read %d bytes, received file descriptor %d\n", pid, fd_read_result, received_socket_fd);
        handle_http_request(received_socket_fd);
        // printf("Worker %d: Request was handled by worker\n", pid);
        
        shutdown(received_socket_fd, SHUT_WR);
        close(received_socket_fd);
    }
}


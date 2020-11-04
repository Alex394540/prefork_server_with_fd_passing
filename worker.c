#include "worker.h"
#include "http.h"


void trim_end_slash(const char * dir_path, char * new_dir_path)
{
    int new_path_length;
    int path_length = strlen(dir_path);
    if (dir_path[path_length - 1] == '/') {
        new_path_length = path_length - 1;
    } else {
        new_path_length = path_length;
    }
    strncpy(new_dir_path, dir_path, new_path_length + 1);
    new_dir_path[new_path_length] = '\0';
}


void trim_get_params(const char * path, char * trimmed_path)
{
    int trimmed_path_length;
    char * symbol_ptr = strchr(path, '?');
    if (symbol_ptr != NULL)
        trimmed_path_length = (int) (symbol_ptr - path);
    else
        trimmed_path_length = strlen(path);
    
    strncpy(trimmed_path, path, trimmed_path_length + 1);
    trimmed_path[trimmed_path_length] = '\0';
}


void build_file_path(const char * base_dir, const char * request_path, char * path_to_file) {
    char trimmed_relative_path[strlen(request_path)];
    memset(trimmed_relative_path, 0, strlen(request_path));
    trim_get_params(request_path, trimmed_relative_path);
    const char * relative_path;
    if (strcmp(trimmed_relative_path, "/")) {
        relative_path = trimmed_relative_path;
    } else {
        relative_path = "/index.html";
    }
    
    char base_dir_trimmed_path[strlen(base_dir)];
    memset(base_dir_trimmed_path, 0, strlen(base_dir));
    trim_end_slash(base_dir, base_dir_trimmed_path);

    strcpy(path_to_file, base_dir_trimmed_path);
    strcat(path_to_file, relative_path);
}


void parse_http_request(const char * request_string, struct http_request * request)
{
    // Path is the longest variable among others
    char buf[MAX_PATH_LENGTH];
    char http_params[5][MAX_PATH_LENGTH];
    int token_num, position, buf_index;

    token_num = position = buf_index = 0;
    memset(buf, 0, MAX_PATH_LENGTH);
    while (position < strlen(request_string) && token_num < 5) {
        if (request_string[position] == ' ' || request_string[position] == '\n' || request_string[position] == '\r') {
            if (buf_index) {
                buf[buf_index + 1] = '\0';
                strcpy(http_params[token_num++], buf);
                memset(buf, 0, MAX_PATH_LENGTH);
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
    char path_to_file[MAX_PATH_LENGTH];
    char response_body[MAX_RESPONSE_BODY_LENGTH];

    memset(path_to_file, 0, MAX_PATH_LENGTH);
    int bytes_read = 0;
    int status_code = NOT_FOUND_HTTP_CODE;
    char * status_message = NOT_FOUND_HTTP_MESSAGE;
    build_file_path(SERVE_DIRECTORY, request->path, path_to_file);
    FILE * fptr = fopen(path_to_file, "rb");

    if (fptr != NULL)
    {
        // Check if directory was not requested
        if ((fseek(fptr, 0, SEEK_END) >= 0)) {
            bytes_read = ftell(fptr);
            if (bytes_read >= 0) {
                status_code = OK_HTTP_CODE;
                status_message = OK_HTTP_MESSAGE;
                fseek(fptr, 0, SEEK_SET);
                if (bytes_read)
                {
                    fread(response_body, 1, bytes_read, fptr);
                    response_body[bytes_read + 1] = '\0';
                }
            } else {
                perror("ftell");
            }
        } else {
            perror("fseek");
        }
        fclose(fptr);
    }
    
    strcpy(response->version, request->version);
    response->status_code = status_code;
    strcpy(response->status_message, status_message);
    strcpy(response->content_type, CONTENT_TYPE_HEADER_VALUE);
    if (bytes_read) {
        response->content_length = bytes_read;
        strcpy(response->response_body, response_body);
    } else {
        response->content_length = 0;
        strcpy(response->response_body, "");
    }
}


void build_http_response_string(const struct http_response * response, char * response_string)
{
    size_t max_starline_length = MAX_VERSION_LENGTH + 2 * MAX_STATUS_MESSAGE_LENGTH;
    size_t max_content_length = 64;
    size_t max_connection_line_length = 128;

    char startline[max_starline_length];
    char content_type[MAX_CONTENT_TYPE_LENGTH];
    char content_length[max_content_length];
    char connection[max_connection_line_length];

    memset(startline, 0, max_starline_length);
    memset(content_type, 0, MAX_CONTENT_TYPE_LENGTH);
    memset(content_length, 0, max_content_length);
    memset(connection, 0, max_connection_line_length);

    sprintf(startline, "%s %d %s", response->version, response->status_code, response->status_message);
    sprintf(content_type, "Content-Type: %s", response->content_type);
    sprintf(content_length, "Content-Length: %d", response->content_length);
    sprintf(connection, "Connection: %s", CONNECTION_HEADER_VALUE);

    sprintf(response_string, 
            "%s\n%s\n%s\n%s\n\n%s", 
            startline, 
            content_type, 
            content_length, 
            connection,
            response->response_body);
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
        
        size = recvmsg(sock, &msg, 0);
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))){
            if (cmsg->cmsg_level != SOL_SOCKET){
                fprintf(stderr,"invalid cmsg_level %d\n", cmsg->cmsg_level);
                exit(EXIT_FAILURE);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS){
                fprintf(stderr, "invalid cmsg_type %d\n", cmsg->cmsg_type);
                exit(EXIT_FAILURE);
            }
            *fd = *((int *) CMSG_DATA(cmsg));
        } else {
            *fd=-1;
        }
    } else {
        size = read(sock, buf, buflen);
        if (size < 0){
           perror("read");
           exit(EXIT_FAILURE);
        }
    }
    return size;
};


void handle_http_request(int slave_socket) {
    char buffer[MAX_REQUEST_BODY_LENGTH];
    memset(buffer, 0, MAX_REQUEST_BODY_LENGTH);
    if (read(slave_socket, buffer, MAX_REQUEST_BODY_LENGTH) < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    
    if (strlen(buffer) != 0) {
        struct http_request parsed_request;
        struct http_response built_response;
        char built_response_string[MAX_RESPONSE_BODY_LENGTH];
        
        parse_http_request(buffer, &parsed_request);
        process_parsed_http_request(&parsed_request, &built_response);
        build_http_response_string(&built_response, built_response_string);

        int bytes_sent = 0;
        if ((bytes_sent = send(slave_socket, built_response_string, strlen(built_response_string), MSG_NOSIGNAL)) < 0) {
            perror("send");
            exit(EXIT_FAILURE);
        }

    } else {
        fprintf(stderr, "0 bytes were read from socket!\n");
    }
}


void start_worker_loop(int worker_fd) {
    int pid = getpid();
    while (1) {
        char buf[5];
        int received_socket_fd, fd_read_result;
        fd_read_result = sock_fd_read(worker_fd, buf, 1, &received_socket_fd);
        handle_http_request(received_socket_fd);
        
        shutdown(received_socket_fd, SHUT_WR);
        close(received_socket_fd);
    }
}


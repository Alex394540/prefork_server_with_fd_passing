#ifndef H_HTTP
#define H_HTTP

#define MAX_PATH_LENGTH 256
#define MAX_HOST_LENGTH 128
#define MAX_VERSION_LENGTH 16
#define MAX_STATUS_MESSAGE_LENGTH 32
#define MAX_CONTENT_TYPE_LENGTH 64
#define MAX_REQUEST_BODY_LENGTH 65536
#define MAX_RESPONSE_BODY_LENGTH 65536
#define OK_HTTP_CODE 200
#define OK_HTTP_MESSAGE "Ok"
#define NOT_FOUND_HTTP_CODE 404
#define NOT_FOUND_HTTP_MESSAGE "Not Found"
#define CONTENT_TYPE_HEADER_VALUE "text/html; charset=utf-8"
#define CONNECTION_HEADER_VALUE "keep-alive"


struct http_request { 
    char path[MAX_PATH_LENGTH]; 
    char host[MAX_HOST_LENGTH]; 
    char version[MAX_VERSION_LENGTH]; 
};


struct http_response { 
    char version[MAX_VERSION_LENGTH]; 
    int status_code; 
    char status_message[MAX_STATUS_MESSAGE_LENGTH];
    char content_type[MAX_CONTENT_TYPE_LENGTH];
    int content_length;
    char response_body[MAX_RESPONSE_BODY_LENGTH];
};

#endif
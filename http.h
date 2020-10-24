#ifndef H_HTTP
#define H_HTTP

// TODO - replace numbers with neat constants according to HTTP 1.0
struct http_request { 
    char path[256]; 
    char host[128]; 
    char version[16]; 
};


struct http_response { 
    char version[16]; 
    int status_code; 
    char status_message[32];
    char content_type[64];
    int content_length;
    char response_body[65536];
};

#endif
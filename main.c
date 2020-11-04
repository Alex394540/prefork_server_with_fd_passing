#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>
#include "server.h"

const unsigned short MIN_VALID_PORT = 80;
const unsigned short MAX_VALID_PORT = 65535;
const char SYMBOLS_IN_IP_ADDRESS[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f',':','.'};


int parseInt(char * s, char * end, int system)
{
    errno = 0;
    int result = strtol(s,&end,10);
    if (errno == ERANGE) {
        fprintf(stderr, "Error: short integer overflow\n");
        return -1;
    } else if ((!isdigit(*s) || *s=='+') || *end) {
        fprintf(stderr, "Error: not a valid short integer\n");
        return -1;
    } else {
        return result;
    }
}

int ip_is_valid(const char * ip)
{
    int i = 0;
    while (ip[i] != 0)
    {
        if (!strchr(SYMBOLS_IN_IP_ADDRESS, ip[i++])) return 0;
    }
    return 1;
}

int port_is_valid(int port)
{
    return (port >= MIN_VALID_PORT && port <= MAX_VALID_PORT);
}

int dir_is_valid(const char * directory_path)
{
    DIR * dir = opendir(directory_path);
    if (dir)
    {
        closedir(dir);
        return 1;
    }
    return 0;
}

int save_params(char * ip, int port, char * dir)
{
    if (!ip_is_valid(ip)) {
        fprintf(stderr, "IP address '%s' is not valid!", ip);
        return 1;
    }
    if (!port_is_valid(port)) {
        fprintf(stderr, "Port '%d' is not valid!", port);
        return 1;
    }
    if (!dir_is_valid(dir)) {
        fprintf(stderr, "Directory '%s' is not valid!", dir);
        return 1;
    }
    return 0;
}


int main(int argc, char ** argv)
{
    char * ip, dir;
    int port, opt;
    // char * dir;
    // int opt;
    
    while ((opt = getopt(argc, argv, ":h:p:d:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("ip: %s\n", optarg);
                ip = optarg;
                break;
            case 'p':
                printf("port: %s\n", optarg);
                char * end = optarg;
                port = parseInt(optarg, end, 10);
                if (port == -1) return 1;
                break;
            case 'd':
                printf("dir: %s\n", optarg);
                dir = optarg;
                break;
            case ':':
                fprintf(stderr, "Error! Option needs a value!\n");
                return 1;
            case '?':
                fprintf(stderr, "Error! Unknown option!\n");
                return 1;
        }
    }

    save_params(ip, port, dir);
    run_server(ip, port, dir);

    return 0;
}

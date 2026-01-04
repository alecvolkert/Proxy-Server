#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/md5.h>

#define QUE_LIMIT 1024

int port, addr_status, connect_status;
int listen_socket, client_socket, target_socket; 
char client_recv_buffer[1024], target_recv_buffer[1024], client_parse_buffer[1024], target_parse_buffer[1024], host[128], read_to_file[1024];
ssize_t target_sent_status, client_sent_status, client_bytes_read, target_bytes_read, file_bytes_read;
char *response, *target_port;
int timeout = 0;`
char *cache[1024];
char *blocklist[1024];
char read_in_cache[1024];
char read_in_blocklist[1024];
int blocklist_count = 0;
int cache_count = 0;
socklen_t client_len;


void create_hash(const char *url, char *hashed){
    unsigned char inter_hash[16]
    MD5((unsigned char*)url, lenof(url), inter_hash);
    for( int i = 0; i<16; i++){
        sprintf(hashed + (i * 2), "%02x", hash[i]);
    }
    hashed[32] = '\0';

}

void remove_user_agent(){
        char *user_agent = strcasestr(client_recv_buffer, "User-Agent:");  
        if(user_agent == NULL) return;
        char *end = strstr(user_agent, "\r\n");
        if(end ==NULL) return;
        end +=2;
        memmove(user_agent, end, strlen(end)+1);

        char *blank_line = strstr(client_recv_buffer, "\r\n\r\n");
        if(blank_line == NULL) return;
        *blank_line = '\0';
        strcat(client_recv_buffer, "\r\nUser-Agent: Proxy/1.0\r\n");
}

void remove_cookie(){
        char *cookie_start = strcasestr(client_recv_buffer, "Cookie:");   

        if(cookie_start == NULL) return;
        char *end = strstr(cookie_start, "\r\n");
        if(end == NULL) return;
        end +=2;
        memmove(cookie_start, end, strlen(end)+1);
}


int is_it_expired(int timout, const char *file){
    return 1;
}
int main(int argc, char *argv[]){
    if (argc < 2){
        perror("Please Enter Format: <PORT> and/or <TIMEOUTVAL>");
        return 1;
    }

    port = (atoi(argv[1]));

    if (port <= 1){
        perror("Invalid Port");
    }

    if(argc ==3){
        timeout = atoi(argv[2]);
    }
    FILE *fp_cache = fopen("cache", "r");
    FILE *fp_blocklist = fopen("blocklist", "r");

    while(fgets(read_in_cache, sizeof(read_in_cache), fp_cache)){
        read_in_cache[strcspn(read_in_cache, "\n")] = '\0';
        cache[cache_count] = strdup(read_in_cache);
        cache_count++;
    }
    fclose(fp_cache);

    while(fgets(read_in_blocklist, sizeof(read_in_blocklist), fp_blocklist)){
        read_in_blocklist[strcspn(read_in_blocklist, "\n")] = '\0';
        blocklist[blocklist_count] = strdup(read_in_blocklist);
        blocklist_count++;
    }
    fclose(fp_blocklist);
    

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in client_info, listen_info, target_info;
    listen_info.sin_family = AF_INET;
    listen_info.sin_port = htons(port);
    listen_info.sin_addr.s_addr = INADDR_ANY;

    bind(listen_socket, (struct sockaddr*)&listen_info, sizeof(listen_info));
    listen(listen_socket, QUE_LIMIT);

    

    while (1){
        client_len = sizeof(client_info);
        client_socket = accept(listen_socket, (struct sockaddr*)&client_info, &client_len);

        int pid = fork();

        if (pid != 0){
            close(client_socket);
            continue;
        } else{
            close(listen_socket);
        }

        while(1){
             client_bytes_read = recv(client_socket, client_recv_buffer, sizeof(client_recv_buffer), 0);
            if (client_bytes_read < 0){
                perror("Failed To Fetch Request");
                break;
            }

            strncpy(client_parse_buffer, client_recv_buffer, sizeof(client_parse_buffer)-1);

            char *reqline = strtok(client_parse_buffer, "\r\n");
            char *method = strtok(reqline, " ");
            char *url = strtok(NULL, " ");
            char *version = strtok(NULL, " ");

            char *host_header = strcasestr(client_recv_buffer, "Host:");
            host_header += 6;

            char *end_host = strcasestr(host_header, "\r\n");
            int len = end_host - host_header;
            memcpy(host, host_header, len);
            host[len] = '\0';

            char *col = strchr(host, ':');

            if (col != NULL){
                target_port = col + 1;
                *col = '\0';
            }else target_port = "80";


            if(method == NULL || url == NULL || version == NULL){
                response = "HTTP/1.1 400 bad request";
                send(client_socket, response, strlen(response), 0);
            }
            if(strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0){
                response = "HTTP/1.1 400 bad request: Invalid HTTP";
                send(client_socket, response, strlen(response), 0);
            }
            if(strlen(host) == 0){
                response = "HTTP/1.1 400 bad request: No hostname";
                send(client_socket, response, strlen(response), 0);
            }
            if(strcmp(method, "GET")!=0){
                perror("HTTP/1.1 Invalid Method");
                continue;
            }

            for(int i = 0; i < blocklist_count; i++){ 
                if(strcasecmp(blocklist[i], host) == 0){
                response = "HTTP/1.1 403 Forbidden\r\n\r\n";  
                send(client_socket, response, strlen(response), 0);
                close(client_socket);
                exit(0);  
                }
            }
            char hashed[33];

            create_hash(url, hashed); 

            char fullpath[128];

            sprintf(fullpath, "cache/%s", hashed);

            FILE *h = fopen(fullpath, "r+");

            if(h){
                while((file_bytes_read = fread(target_recv_buffer, 1, sizeof(target_recv_buffer), h)) >0){
                    send(client_socket, target_recv_buffer, file_bytes_read, 0);
                }
                fclose(h)
                continue;
            }



            remove_cookie();
            remove_user_agent();


            struct addrinfo hints, *res;
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = 0;
            hints.ai_protocol = 0;

            addr_status = getaddrinfo(host, target_port, &hints, &res);

            if (addr_status !=0){
                char *response = "HTTP/1.1 400 bad request";
                client_sent_status = send(client_socket, response, strlen(response), 0);
                continue;
            }

            struct addrinfo *ps;
            for(ps = res; ps !=NULL; ps = ps->ai_next){
                if((target_socket = socket(ps->ai_family, ps->ai_socktype, ps->ai_protocol)) == -1){
                    continue;
                }
                break;
            }

            if (target_socket < 0){
                response = "HTTP/1.1 404 not found";
                client_sent_status = send(client_socket, response, strlen(response), 0);
                continue;
            }            

            connect_status = connect(target_socket,ps ->ai_addr, ps -> ai_addrlen);
            
            if (connect_status < 0){
                response = "HTTP/1.1 502 Bad Gateway";
                client_sent_status = send(client_socket, response, strlen(response), 0);
                continue;
            }

            target_sent_status = send(target_socket, client_recv_buffer, strlen(client_recv_buffer), 0);

            if (target_sent_status< 0){
                response = "HTTP/1.1 502 Bad Gateway";
            }

            while((target_bytes_read = recv(target_socket, target_recv_buffer, sizeof(target_recv_buffer), 0))> 0){
                send(client_socket, target_recv_buffer, target_bytes_read, 0);
                fwrite(target_recv_buffer, 1, target_bytes_read, h);
            }
            fclose(h);

            if(target_bytes_read < 0){
                response = "HTTP/1.1 502 Bad Gateway";
                client_sent_status = send(client_socket, response, strlen(response), 0);
                continue;
            }
            close(target_socket);
            close(client_socket);
            exit(0);
        }   
    }
}
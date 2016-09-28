#include <stdio.h>
#include <string>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <boost/algorithm/string.hpp>

using namespace std;
using namespace boost;

const int PORT = 80;
const int MAXRECV = 140 * 1024;

string make_request(string hostname, string path) {
    string request = "GET ";
    request.append(path);
    request.append(" HTTP/1.0");
    request.append("\r\n");
    request.append("Host: ");
    request.append(hostname);
    request.append("\r\n");
    request.append("Connection: close");
    request.append("\r\n\r\n");
    return request;
}

int send_request(string request, int sock) {
    int status = send(sock, request.c_str(), request.size(), MSG_NOSIGNAL);
    
    cout << "-------Sent Request-------" << endl;
    cout << request << endl;
    cout << "-------Request End--------" << endl;
    
    return status;
}

string receive_response(int sock) {
    char response_buffer[MAXRECV];
    memset(response_buffer, 0, MAXRECV);
    
    int status = recv(sock, response_buffer, MAXRECV, 0);
    string response(response_buffer);
    while (status != 0) {
        memset(response_buffer, 0, MAXRECV);
        status = recv(sock, response_buffer, MAXRECV, 0);
        response.append(response_buffer);
    }
    
    cout << "-----Request Response-----" << endl;
    cout << response << endl;
    cout << "------Response End--------" << endl;
    
    return response;
}

int crawl(string hostname, string path) {
    cout << "Hostname: " << hostname << endl;
    cout << "Path: " << path << endl;
    
    // set up socket sock
    int sock;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // connect to host
    struct hostent* host_ip = gethostbyname(hostname.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = *((in_addr*)host_ip->h_addr);
    connect(sock, (sockaddr*)&server_addr, sizeof(sockaddr));
    
    cout << "Connected to " << inet_ntoa(server_addr.sin_addr) << ":" << ntohs(server_addr.sin_port) << endl;
    
    if (send_request(make_request(hostname, path), sock) != 0) {
        receive_response(sock);
    }
}

int main() {
    // read the file
    // for each url in file, run connect
    crawl("nusmaids.tk", "/login");
    return 1;
}
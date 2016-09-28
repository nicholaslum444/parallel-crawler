#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;

typedef std::chrono::high_resolution_clock Clock;

const int PORT = 80;
const int MAXRECV = 140 * 1024;
const int DELAY = 2;

class Url {
public:
    string host;
    string path;
    double response_time;
    
    Url(string h, string p, double rt) {
        host = h;
        path = p;
        response_time = rt;
    }
    
    string to_string() {
        stringstream ss;
        ss << "{ " 
            << "Host: " << host 
            << ", Path: " << path 
            << ", Response Time: " << response_time 
            << " }" << endl;
        return ss.str();
    }
    
    string to_file_output() {
        stringstream ss;
        ss << host << endl 
            << path << endl 
            << response_time << endl;
        return ss.str();
    }
};

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
    int status = send(sock, 
        request.c_str(), 
        request.size(), 
        MSG_NOSIGNAL);
    
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
    
    return response;
}

double crawl(string hostname, string path) {
    chrono::duration<double> rt_nanosec;
    
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
    
    string request = make_request(hostname, path);
    auto clock_start = Clock::now();
    int status = send_request(request, sock);
    if (status != 0) {
        string response = receive_response(sock);
        auto clock_end = Clock::now();
        rt_nanosec = (clock_end - clock_start);
        
        cout << "-----Request Response-----" << endl;
        //cout << response << endl;
        cout << "Ommitted" << endl;
        cout << "------Response End--------" << endl;
        
        cout << "Response Time in Seconds: " 
            << rt_nanosec.count() << endl;
         
        return rt_nanosec.count();
    } else {
        return -1.0;
    }
}

vector<Url> readUrls(string filename) {
    vector<Url> urls;
    // read the file
    string line;
    ifstream urlsfile(filename);
    if (urlsfile.is_open()) {
        while (getline(urlsfile, line)) {
            string host = line;
            string path;
            getline(urlsfile, path);
            string rt_str;
            getline(urlsfile, rt_str);
            double response_time = atof(rt_str.c_str());
            Url url = Url(host, path, response_time);
            urls.push_back(url);
        }
        urlsfile.close();
    } else {
        cout << filename << " spoil" << endl;
    }
    return urls;
}

int writeUrls(vector<Url*> urls, string filename) {
    ofstream urlsfile(filename);
    if (urlsfile.is_open()) {
        BOOST_FOREACH(Url* url, urls) {
            urlsfile << url->to_file_output();
        }
        urlsfile.close();
    }
}

int main() {
    cout << "Start" << endl;
    cout << "Reading urls" << endl;
    vector<Url> urls = readUrls("urls.txt");
    cout << "Done reading urls" << endl;

    // for each url in file, run connect
    vector<Url*> updated_urls;
    BOOST_FOREACH(Url url, urls) {
        cout << "Crawling " << url.host << url.path << endl;
        double rt = crawl(url.host, url.path);
        
        Url* updated_url = new Url(url.host, url.path, rt);
        updated_urls.push_back(updated_url);
        
        cout << "Sleeping for " << DELAY << " seconds" << endl;
        sleep(DELAY);
    }
    
    cout << "Writing urls" << endl;
    writeUrls(updated_urls, "urls-updated.txt");
    cout << "Done writing urls" << endl;
}
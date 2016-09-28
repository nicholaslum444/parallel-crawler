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

using namespace std;
using namespace boost;
using namespace std::chrono;

const int PORT = 80;
const int MAXRECV = 140 * 1024;
const int DELAY = 2;
const int MAX_CRAWLS = 20;

class Url {
public:
    string host;
    string path;
    double response_time;
    bool crawled;
    
    Url(string h, string p, double rt) {
        host = h;
        path = p;
        response_time = rt;
        bool crawled = false;
    }
    
    void set_rt(double rt) {
        response_time = rt;
    }
    
    void set_crawled(bool c) {
        crawled = c;
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

Url* crawl(Url* url) {
    url->set_crawled(true);
    
    cout << "Hostname: " << url->host << endl;
    cout << "Path: " << url->path << endl;
    
    // set up socket sock
    int sock;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // connect to host
    struct hostent* host_ip = gethostbyname(url->host.c_str());
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = *((in_addr*)host_ip->h_addr);
    connect(sock, (sockaddr*)&server_addr, sizeof(sockaddr));
    
    cout << "Connected to " << inet_ntoa(server_addr.sin_addr) 
    << ":" << ntohs(server_addr.sin_port) << endl;
    
    string request = make_request(url->host, url->path);
    auto clock_start = high_resolution_clock::now();
    int status = send_request(request, sock);
    if (status != 0) {
        string response = receive_response(sock);
        auto clock_end = high_resolution_clock::now();
        double rt = duration_cast<nanoseconds>(clock_end - clock_start).count();
        url->set_rt((double)rt/(double)1000000000);
        
        cout << "-----Request Response-----" << endl;
        //cout << response << endl;
        cout << "Ommitted" << endl;
        cout << "------Response End--------" << endl;
        
        cout << "Response Time in Seconds: " 
            << (double)url->response_time << endl;
        
    } else {
        url->set_rt(-1.0);
    }
    return url;
}

vector<Url*> readUrls(string filename) {
    vector<Url*> urls;
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
            Url* url = new Url(host, path, response_time);
            urls.push_back(url);
        }
        urlsfile.close();
    } else {
        cout << filename << " spoil" << endl;
    }
    return urls;
}

int writeUrls(vector<Url*> &urls, string filename) {
    ofstream urlsfile(filename);
    if (urlsfile.is_open()) {
        BOOST_FOREACH(Url* url, urls) {
            urlsfile << url->to_file_output();
        }
        urlsfile.close();
    }
}

int test(Url* url) {
    url->set_rt(321);
    cout << url->to_string() << endl;
}

int main() {
    cout << "Start" << endl;
    
    cout << "Reading urls" << endl;
    vector<Url*> urls = readUrls("urls.txt");
    cout << "Done reading urls, there are " << urls.size() 
        << " urls in the file." << endl;

    int crawled = 0;
    for (int i = 0; 
        (crawled < MAX_CRAWLS) && 
            (crawled < urls.size()) && 
            (i < urls.size());
        i++) {
        Url* url = urls.at(crawled);
        if (url->crawled) {
            break;
        }
        cout << "Crawling " << url->host << url->path << endl;
        urls.push_back(crawl(url));
        crawled++;
        
        cout << "Sleeping for " << DELAY << " seconds" << endl;
        sleep(DELAY);
    }
    
    cout << "Writing urls" << endl;
    writeUrls(urls, "urls-updated.txt");
    cout << "Done writing urls" << endl;
}
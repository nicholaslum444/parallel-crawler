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
#include <boost/regex.hpp>

using namespace std;
using namespace boost;
using namespace std::chrono;

const int PORT = 80;
const int MAXRECV = 140 * 1024;
const int DELAY = 2;
const int MAX_CRAWLS = 50;

class Url {
public:
    string host;
    string path;
    double response_time;
    bool accessible;
    
    Url(string h, string p, double rt) {
        host = h;
        path = p;
        response_time = rt;
        accessible = false;
    }
    
    void set_rt(double rt) {
        response_time = rt;
    }
    
    string get() {
        string full_url = "";
        full_url.append(host);
        full_url.append(path);
        return full_url;
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

Url* parseHref(string orig_host, string str) {
    const regex re("(?i)http://(.*)/(.*)");
    smatch what;
    if (regex_match(str, what, re)) {
        // We found a full URL, parse out the 'hostname'
        // Then parse out the page
        string hostname = what[1];
        algorithm::to_lower(hostname);
        string page = what[2];
        return new Url(hostname, "/" + page, -1);
    } else {
        // We could not find the 'page' 
        // but we can still build the hostname
        return new Url(orig_host, "/", -1);
    }
}

std::string parseHttp(string str) {
    // first check if it's a https link.
    // if it is then dont parse it,
    // just return / so that it's a dead link.
    const regex re_https("(?i)https://(.*)/?(.*)");
    smatch what_https;
    if (regex_match(str, what_https, re_https)) {
        return "/";
    }
    
    // check if the link has a http
    // if have http, then likely its another website
    // get the website hostname, 
    // or return empty string if dont have
    const regex re("(?i)http://(.*)/?(.*)");
    smatch what;
    if (regex_match(str, what, re)) {
        string hst = what[1];
        algorithm::to_lower(hst);
        return hst;
    }
    return "";
}

Url* parse(string orig_host, string href) {
    string host = parseHttp(href);
    string page;
    Url* new_url;
    if (!host.empty()) {
        // If we have a HTTP prefix
        // We could end up with a 'hostname' and page
        new_url = parseHref(host, href);
    } else {
        // the hostname part is empty or no http prefix
        // so the page is actually link to the same hostname
        // the entire href is the page
        if (href[0] != '/') {
            href = "/" + href;
        }
        new_url = new Url(orig_host, href, -1);
    }
    
    // hostname and page are constructed,
    // perform post analysis
    if (new_url->path.length() == 0) {
        new_url->path = "/";
    }
    
    return new_url;
}

// returns all urls in the page
vector<Url*> parse_response(Url* url, string &response) {
    vector<Url*> new_urls;
    
    // remove enters
    const regex rmv_all("[\\r|\\n]");
    const string s2 = regex_replace(response, rmv_all, "");
    const string s = s2;
    
    // get all the <a> link tags
    const regex re("<a\\s+href\\s*=\\s*(\"([^\"]*)\")|('([^']*)')\\s*>");
    cmatch matches;
    
    // iterate through the tokens
    const int subs[] = {2, 4};
    sregex_token_iterator i(s.begin(), s.end(), re, subs);
    sregex_token_iterator j;
    for (; i != j; i++) {
        const string href = *i;
        if (href.length() != 0) {
            Url* new_url = parse(url->host, href);
            new_urls.push_back(new_url);
        }
    }
    
    return new_urls;
}

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

vector<Url*> crawl(Url* url) {
    vector<Url*> new_urls;
    
    cout << "Hostname: " << url->host << endl;
    cout << "Path: " << url->path << endl;
    
    // set up socket sock
    int sock;
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    sock = socket(AF_INET, SOCK_STREAM, 0);
    
    // connect to host
    struct hostent* host_ip = gethostbyname(url->host.c_str());
    cout << "got host ip -----------------------" << endl;
    if (host_ip == NULL) {
        cout << "no ip" << endl;
        url->accessible = false;
        return vector<Url*>();
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    cout << "gonna connect" << endl;
    server_addr.sin_addr = *((in_addr*)host_ip->h_addr);
    if (connect(sock, (sockaddr*)&server_addr, sizeof(sockaddr)) != 0) {
        cout << "cannot connect" << endl;
        url->accessible = false;
        return vector<Url*>();
    }
    
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
            
        // deal with response
        // get all the urls in the response
        new_urls = parse_response(url, response);
        cout << new_urls.size() << " new urls" << endl;
    } else {
        url->set_rt(-2.0);
    }

    url->accessible = true;
    cout << "Sleeping for " << DELAY << " seconds" << endl;
    sleep(DELAY);
    return new_urls;
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
        cout << filename << " spoil." << endl;
    }
    return urls;
}

int writeUrls(vector<Url*> &urls, string filename) {
    ofstream urlsfile(filename);
    if (urlsfile.is_open()) {
        for(Url* url : urls) {
            if (url->accessible) {
                urlsfile << url->to_file_output();
            }
        }
        urlsfile.close();
    }
}

bool exists(vector<Url*> &urls, Url* &new_url) {
    for (Url* url : urls) {
        if (url->get() == new_url->get()) {
            cout << url->get() 
                << " already exists! Not adding..." << endl
            return true;
        }
    }
    return false;
}

int add_to(vector<Url*> &urls, vector<Url*> &new_urls) {
    BOOST_FOREACH(Url* new_url, new_urls) {
        if (!exists(urls, new_url)) {
            urls.push_back(new_url);
        }
    }
}

int main() {
    cout << "Start" << endl;
    
    cout << "Reading urls" << endl;
    vector<Url*> urls = readUrls("urls.txt");
    cout << "Done reading urls, there are " << urls.size() 
        << " urls in the file." << endl;

    int crawled = 0;
    for (int i = 0;
            (crawled < MAX_CRAWLS) && (i < urls.size());
            i++) {
        Url* url = urls.at(i);
        cout << "Crawling " << url->host << url->path << endl;
        
        vector<Url*> new_urls = crawl(url);
        if (!url->accessible) {
            cout << "Cannot connect to: " << url->get() << endl;
        } else {
            add_to(urls, new_urls);
            crawled++;
        }
        
        cout << urls.size() << " urls in the queue" << endl;
        cout << crawled << " / " << MAX_CRAWLS << endl;
    }
    
    cout << "Writing urls" << endl;
    writeUrls(urls, "urls-updated.txt");
    cout << "Done writing urls" << endl;
}
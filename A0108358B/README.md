# Compile
To compile, run the following command: `g++ -std=c++11 -o crawler crawler.cpp -lboost_regex`. This program requires the Boost library. It has only been tested on Windows.

# Execution
To execute the program, simply run `crawler.exe`.

# Resource files
The file `urls.txt` contains the starting URLs for the crawler to visit. It can contain any number of starting URLs.

The file `urls-updated.txt` contains the list of URLs that the crawler has successfully crawled. It contains at most the number of URLs that the crawler has been instructed to crawl.

The files have the same layout, as follows:
```
<Hostname of URL 1>
<Path of URL 1>
<Response time of URL 1>
<Hostname of URL 2>
<Path of URL 2>
<Response time of URL 2>
...
<Hostname of URL n>
<Path of URL n>
<Response time of URL n>
```

# Features
- File `urls-updated.txt` generated, containing base URL and response time
- HTTP message constructed in code
- 2-second delay between calls
- Crawler automatically stops after successfulling crawling `100` URLs
- Crawler spawns multiple threads to simultaneously crawl multiple URLs
- Discovered URLs are checked against the current pool of URLs for uniqueness
- Exception handling for I/O operations
- Comments in code where useful

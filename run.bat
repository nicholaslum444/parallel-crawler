@ECHO OFF
g++ -std=c++11 -o crawler crawler.cpp -lboost_regex && crawler.exe

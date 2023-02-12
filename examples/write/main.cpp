#include <iostream>

#define RSP_IMPLEMENTATION
#include "RSP.hpp"

int main(){
    RSP::data d;

    d.next = {
        {"Bikes", "2"},
        {"Bike 1", .next = {{"color", "\"red\""}}, .args = {{"test1","\"1\""}, {"test2","\"2\""}, {"test3","\"3\""}}},
        {"Bike 2", .next = {{"color", "\"blue\""}}}
    }; 

    RSP::writeF("data.json", d, RSP::JSON);
    RSP::writeF("data.xml", d, RSP::XML);
}
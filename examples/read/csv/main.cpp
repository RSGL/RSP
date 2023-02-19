#include <iostream>

#define RSP_IMPLEMENTATION
#include "RSP.hpp"

int main(){
    RSP::data d = RSP::loadF("file.csv");
    
    system("clear");
    std::cout << std::endl;

    for (int i = 0; i < d.list.size(); i++)
        std::cout << d[i].key << " : " << d[i].value << std::endl;
}
#include <iostream>

#define RSP_IMPLEMENTATION
#include "RSP.hpp"

int main(){
    RSP::data d = RSP::loadF("index.html");
    
    for (auto& k : d.next){
        std::cout << "html/index layer" << std::endl;
        std::cout << k.key << std::endl;

        std::cout << std::endl << "body layer" << std::endl;
        for (auto& k : k.next){
            std::cout << k.key << std::endl;
        }
    }

    std::cout << std::endl << "body > img's args :" << std::endl << std::endl;

    for (auto& arg : d["body"]["img"].args)
        std::cout << arg.first << " : " << arg.second << std::endl;
}
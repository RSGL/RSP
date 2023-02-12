#include <iostream>

#define RSP_IMPLEMENTATION
#include "RSP.hpp"

int main(){
    RSP::data d = RSP::loadF("file.json");

    std::cout << "member 1 :" << std::endl;

    for (auto& k : d["member 1"].next){
        std::cout << k.key;
        
        if (k.empty())
            std::cout << " : " << k.value; 
        
        std::cout << std::endl; 
    }

    d["member 2"].next = {{"test", "1"}};
    
    std::cout << d["member 2"]["name"].value << std::endl;
    std::cout << d["member 2"]["test"].value << std::endl;

    d["member 1"]["age"].value = "19";
    std::cout << "new member 1 age : " << d["member 1"]["age"].value << std::endl;
}
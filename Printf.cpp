#include "Printf.hpp"

#include <string>
#include <iostream>

namespace mojo
{

void printfHelper(const std::string& string) {
    std::cout << string << std::endl;
}

}

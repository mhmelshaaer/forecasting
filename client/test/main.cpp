#include <iostream>
#include "test.h"
int main() {
    std::cout << "Starting test_client_server_communication: \n\n";
    test_client_server_communication();
    std::cout << "\n\n\n";

    std::cout << "Starting test_net_message: \n\n";
    test_net_message();
    std::cout << "\n\n\n";

    std::cout << "All tests passed\n";
    return 0;
}
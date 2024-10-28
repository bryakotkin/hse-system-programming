#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Program is sleeping for 60 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(60));
    std::cout << "Program has finished sleeping and will now exit." << std::endl;
    return 0;
}
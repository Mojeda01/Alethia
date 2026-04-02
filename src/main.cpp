#include "VulkanApp.h"
#include "SwapchainBundle.h"
#include <iostream>

int main() {
    int version = 0;
    std::cout << "Select engine version:\n";
    std::cout << "  1 - Alethia v1\n";
    std::cout << "  2 - Alethia v2\n";
    std::cin >> version;

    switch (version) {
        case 1: {
            try {
                VulkanApp app(2560, 1440, "Alethia Game Engine");
                app.run();
            } catch (const std::exception& e) {
                std::cerr << e.what() << "\n";
                return 1;
            }
            break;
        }
        case 2:
            std::cout << "Alethia v2 — coming soon\n";
            break;
        default:
            std::cerr << "Invalid selection\n";
            return 1;
    }
    return 0;
}

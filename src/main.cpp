#include "VulkanApp.h"
#include "SwapchainBundle.h"
#include <iostream>

int main() {
    try {
        VulkanApp app(2560, 1440, "VulkanLab");
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}


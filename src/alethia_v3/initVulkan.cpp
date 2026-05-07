#include "AlethiaV3.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>

void AlethiaV3::initVulkan()
{
    std::cout << "[AlethiaV3::initVulkan()]:: Started \n";

    std::cout << "→ createInstance()\n";
    createInstance();
}

#include "AlethiaV3.h"
#include <iostream>

// Constructor
AlethiaV3::AlethiaV3(GLFWwindow* window) 
    : m_window(window)
{
    std::cout << "[AlethiaV3] Constructor called\n";
}

// Destructor
AlethiaV3::~AlethiaV3()
{
    std::cout << "[AlethiaV3] Destructor called\n";

}


void AlethiaV3::drawFrame()
{

}

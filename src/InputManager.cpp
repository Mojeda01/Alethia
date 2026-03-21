#include "InputManager.h"
#include <imgui.h>
#include <cstring>
#include <iostream>

InputManager::InputManager(GLFWwindow* win) : window(win)
{
    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, InputManager::cursorPosCallback);
    glfwSetMouseButtonCallback(window, InputManager::mouseButtonCallback);
    glfwSetFramebufferSizeCallback(window, InputManager::framebufferResizeCallback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (!glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwGetCursorPos(window, &cursorX, &cursorY);
    lastCursorX = cursorX;
    lastCursorY = cursorY;

    std::memset(keysCurrent, 0, sizeof(keysCurrent));
    std::memset(keysPrevious, 0, sizeof(keysPrevious));
    std::memset(buttonsCurrent, 0, sizeof(buttonsCurrent));
    std::memset(buttonsPrevious, 0, sizeof(buttonsPrevious));
}

void InputManager::update() 
{

    void* ptr = glfwGetWindowUserPointer(window);
    std::cout << "UserPointer: " << ptr << " this: " << this << "\n";
    std::cout << "buttonsCurrent[0]=" << buttonsCurrent[0] << " buttonsPrevious[0]=" << buttonsPrevious[0] << "\n";
    std::memset(buttonsJustPressed, 0, sizeof(buttonsJustPressed)); 
    std::memcpy(keysPrevious, keysCurrent, sizeof(keysCurrent));
    std::memcpy(buttonsPrevious, buttonsCurrent, sizeof(buttonsCurrent));

    for (int i = 0; i < MAX_KEYS; ++i)
    {
        keysCurrent[i] = (glfwGetKey(window, i) == GLFW_PRESS);
    }

    for (int i = 0; i < MAX_BUTTONS; ++i)
    {
        buttonsCurrent[i] = (glfwGetMouseButton(window, i) == GLFW_PRESS);
    }

    if (wasKeyJustPressed(GLFW_KEY_TAB)) {
        setUIMode(!uiMode);
    }

    if (!uiMode) {
        deltaX = cursorX - lastCursorX;
        deltaY = cursorY - lastCursorY;
    } else {
        deltaX = 0.0;
        deltaY = 0.0;
    }

    lastCursorX = cursorX;
    lastCursorY = cursorY;
    firstMouse = false; 
}

bool InputManager::isKeyPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return keysCurrent[key];
}

bool InputManager::wasKeyJustPressed(int key) const {
    if (key < 0 || key >= MAX_KEYS) return false;
    return keysCurrent[key] && !keysPrevious[key];
}

bool InputManager::isMouseButtonPressed(int button) const {
    if (button < 0 || button >= MAX_BUTTONS) return false;
    return buttonsCurrent[button];
}

bool InputManager::wasMouseButtonJustPressed(int button) const {
    if (button < 0 || button >= MAX_BUTTONS) return false;
    return buttonsJustPressed[button]; 
}

void InputManager::setUIMode(bool mode) {
    uiMode = mode;
    if (uiMode) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true;
        glfwGetCursorPos(window, &lastCursorX, &lastCursorY);
        cursorX = lastCursorX;
        cursorY = lastCursorY;
        deltaX = 0.0;
        deltaY = 0.0;
    }
}

bool InputManager::imguiWantsMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool InputManager::imguiWantsKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

void InputManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) 
{
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;
    input->cursorX = xpos;
    input->cursorY = ypos;
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    std::cout << "CALLBACK: button=" << button << " action=" << action << "\n";
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;
    if (button >= 0 && button < MAX_BUTTONS) {
        input->buttonsCurrent[button] = (action == GLFW_PRESS);
        if (action == GLFW_PRESS) {
            input->buttonsJustPressed[button] = true;
        }
    }
}

void InputManager::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    (void)width;
    (void)height;
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (input) {
        input->fbResized = true;
    }
}

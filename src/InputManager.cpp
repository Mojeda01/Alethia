#include "InputManager.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <cstring>
#include <iostream>

InputManager::InputManager(GLFWwindow* win) : window(win)
{
    glfwSetWindowUserPointer(window, this);
    installCallbacks();
   
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported()) {
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
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;
    if (button >= 0 && button < MAX_BUTTONS) {
        input->buttonsCurrent[button] = (action == GLFW_PRESS);
        if (action == GLFW_PRESS) {
            input->buttonsJustPressed[button] = true;
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT) {
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    std::cout << "MY scroll callback fired: " << yoffset << "\n";
    (void)xoffset;
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;
    input->scrollY += yoffset;
    std::cout << "scroll: " << yoffset << "\n";
    if (input->uiMode) {
        ImGui::GetIO().AddMouseWheelEvent(0.0f, static_cast<float>(yoffset));
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

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;
    
    // in FPS mode, block Space and Enter from reaching ImGui
    // prevents gameplay keys from activating focused UI buttons
    if (!input->uiMode && (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER)) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (mods & GLFW_MOD_CONTROL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (mods & GLFW_MOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (mods & GLFW_MOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (mods & GLFW_MOD_SUPER) != 0);

    ImGuiKey imKey = ImGuiKey_None;
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) imKey = (ImGuiKey)(ImGuiKey_A + (key - GLFW_KEY_A));
    else if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) imKey = (ImGuiKey)(ImGuiKey_0 + (key - GLFW_KEY_0));
    else if (key == GLFW_KEY_SPACE) imKey = ImGuiKey_Space;
    else if (key == GLFW_KEY_ENTER) imKey = ImGuiKey_Enter;
    else if (key == GLFW_KEY_BACKSPACE) imKey = ImGuiKey_Backspace;
    else if (key == GLFW_KEY_DELETE) imKey = ImGuiKey_Delete;
    else if (key == GLFW_KEY_LEFT) imKey = ImGuiKey_LeftArrow;
    else if (key == GLFW_KEY_RIGHT) imKey = ImGuiKey_RightArrow;
    else if (key == GLFW_KEY_UP) imKey = ImGuiKey_UpArrow;
    else if (key == GLFW_KEY_DOWN) imKey = ImGuiKey_DownArrow;
    else if (key == GLFW_KEY_HOME) imKey = ImGuiKey_Home;
    else if (key == GLFW_KEY_END) imKey = ImGuiKey_End;
    else if (key == GLFW_KEY_ESCAPE) imKey = ImGuiKey_Escape;
    else if (key == GLFW_KEY_TAB) imKey = ImGuiKey_Tab;
    else if (key == GLFW_KEY_PERIOD) imKey = ImGuiKey_Period;
    else if (key == GLFW_KEY_MINUS) imKey = ImGuiKey_Minus;

    if (imKey != ImGuiKey_None) {
        io.AddKeyEvent(imKey, action != GLFW_RELEASE);
    }
}

void InputManager::charCallback(GLFWwindow* window, unsigned int codepoint) {
    auto* input = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(codepoint);
}

void InputManager::installCallbacks() {
    glfwSetCursorPosCallback(window, InputManager::cursorPosCallback);
    glfwSetMouseButtonCallback(window, InputManager::mouseButtonCallback);
    glfwSetScrollCallback(window, InputManager::scrollCallback);
    glfwSetKeyCallback(window, InputManager::keyCallback);
    std::cout << "scroll callback installed: " << (void*)InputManager::scrollCallback << "\n";
    glfwSetCharCallback(window, InputManager::charCallback);
    glfwSetFramebufferSizeCallback(window, InputManager::framebufferResizeCallback);
}


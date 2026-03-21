#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class InputManager{
public:
    InputManager(GLFWwindow* window);

    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager) = delete;

    void update();
    bool isKeyPressed(int key) const;
    bool wasKeyJustPressed(int key) const;
    bool isMouseButtonPressed(int button) const;
    bool wasMouseButtonJustPressed(int button) const;
    double mouseX() const { return cursorX; }
    double mouseY() const { return cursorY; }
    double mouseDeltaX() const { return deltaX; }
    double mouseDeltaY() const { return deltaY; }
    bool inUIMode() const { return uiMode; }
    void setUIMode(bool mode);
    bool imguiWantsMouse() const;
    bool imguiWantsKeyboard() const;
    bool framebufferWasResized() const { return fbResized; }
    void clearFramebufferResized() { fbResized = false; }

private:
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* window = nullptr;
    bool uiMode = false;

    double cursorX = 0.0;
    double cursorY = 0.0;
    double lastCursorX = 0.0;
    double lastCursorY = 0.0;
    double deltaX = 0.0;
    double deltaY = 0.0;
    bool firstMouse = true;

    bool fbResized = false;

    static constexpr int MAX_KEYS = 512;
    static constexpr int MAX_BUTTONS = 8;
    bool keysCurrent[MAX_KEYS] = {};
    bool keysPrevious[MAX_KEYS] = {};
    bool buttonsCurrent[MAX_BUTTONS] = {};
    bool buttonsPrevious[MAX_BUTTONS] = {};
    bool buttonsJustPressed[MAX_BUTTONS] = {};
};

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "renderer.h"
#include "model_loader.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Camera class for controlling view
class Camera {
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float distance;
    float focusHeight;

    float movementSpeed;
    float mouseSensitivity;
    float zoom;

    Camera(glm::vec3 pos = glm::vec3(0.0f, 2.0f, 15.0f)) {
        position = pos;
        worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = -20.0f;
        distance = 15.0f;
        focusHeight = 0.0f;
        movementSpeed = 2.5f;
        mouseSensitivity = 0.1f;
        zoom = 45.0f;

        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void processKeyboard(int direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        if (direction == 0) // FORWARD
            position += front * velocity;
        if (direction == 1) // BACKWARD
            position -= front * velocity;
        if (direction == 2) // LEFT
            position -= right * velocity;
        if (direction == 3) // RIGHT
            position += right * velocity;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        zoom -= yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 120.0f)
            zoom = 120.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 frontVec;
        frontVec.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        frontVec.y = sin(glm::radians(pitch));
        frontVec.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(frontVec);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

// Camera control variables
    Camera camera;
bool cameraMouseControl = false;
float lastX = 400, lastY = 300;
bool firstMouse = true;

// FPS-style camera settings
float cameraSensitivity = 0.002f; // Increased sensitivity for better control
float baseCameraSpeed = 8.0f; // Base speed for gunfight movement
float sprintMultiplier = 2.0f; // Sprint speed multiplier

// Input callback functions
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (cameraMouseControl) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = (xpos - lastX) * cameraSensitivity;
        float yoffset = (lastY - ypos) * cameraSensitivity; // reversed since y-coordinates go from bottom to top

        lastX = xpos;
        lastY = ypos;

        camera.processMouseMovement(xoffset * 100.0f, yoffset * 100.0f); // Scale back to expected range
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.processMouseScroll(yoffset);
}

void processInput(GLFWwindow* window, float deltaTime, Renderer& renderer) {
    // Calculate current speed (with sprint)
    float currentSpeed = baseCameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentSpeed *= sprintMultiplier; // Sprint speed
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(0, deltaTime * currentSpeed); // FORWARD
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(1, deltaTime * currentSpeed); // BACKWARD
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(2, deltaTime * currentSpeed); // LEFT
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(3, deltaTime * currentSpeed); // RIGHT

    // Toggle camera mouse control with C key
    static bool cKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !cKeyPressed) {
        cameraMouseControl = !cameraMouseControl;
        if (cameraMouseControl) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        cKeyPressed = true;
        std::cout << "Camera mouse control: " << (cameraMouseControl ? "ON" : "OFF") << std::endl;
    } else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE) {
        cKeyPressed = false;
    }

    // Debug mode switching with F keys
    static bool f1Pressed = false, f2Pressed = false, f3Pressed = false, f4Pressed = false, f5Pressed = false;

    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS && !f1Pressed) {
        renderer.showDebugMode(0); // Albedo
        f1Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_RELEASE) {
        f1Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS && !f2Pressed) {
        renderer.showDebugMode(1); // Normal
        f2Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_RELEASE) {
        f2Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS && !f3Pressed) {
        renderer.showDebugMode(2); // Depth
        f3Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_RELEASE) {
        f3Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS && !f4Pressed) {
        renderer.showDebugMode(3); // Lighting
        f4Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_RELEASE) {
        f4Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS && !f5Pressed) {
        renderer.showDebugMode(4); // SSAO
        f5Pressed = true;
    } else if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_RELEASE) {
        f5Pressed = false;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "SSAO with Camera Control", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Set up input callbacks
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Setup ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    Renderer renderer;
    if (!renderer.init()) {
        std::cerr << "Renderer init failed\n";
        return -1;
    }

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        // Process input
        processInput(window, dt, renderer);

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Gunfight Scene Renderer");
        ImGui::Text("Frame time: %.3f ms (%.1f FPS)", dt * 1000.0f, 1.0f / dt);
        ImGui::Text("Scene: Gunfight arena with walls and cover objects");
        ImGui::Text("Rendering: G-buffer + SSAO + Deferred Lighting");
        ImGui::Text("Lights: 1 directional + %d point lights", 7);
        ImGui::Separator();
        ImGui::Text("Camera Controls:");
        ImGui::Text("C: Toggle mouse control (%s)", cameraMouseControl ? "ON" : "OFF");
        ImGui::Text("Mouse: Rotate camera (when ON)");
        ImGui::Text("Scroll: Zoom in/out");
        ImGui::Text("WASD: Move camera");
        ImGui::Text("Shift+WASD: Sprint");
        ImGui::Text("ESC: Close application");
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("Zoom: %.1f", camera.zoom);
        ImGui::Text("Sensitivity: %.4f", cameraSensitivity);

        ImGui::Separator();
        ImGui::Text("Rendering Options:");
        ImGui::Text("F1-F5: Switch debug modes");
        ImGui::Text("F1: Albedo | F2: Normal | F3: Depth | F4: Lighting | F5: SSAO");

        // Add some camera sensitivity control
        if (ImGui::SliderFloat("Mouse Sensitivity", &cameraSensitivity, 0.001f, 0.01f)) {
            // Sensitivity updated
        }

        ImGui::End();

        // Show renderer debug UI
        renderer.showDebugUI();

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create projection matrix with camera zoom
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom),
                                              (float)display_w / (float)display_h,
                                              0.1f, 1000.0f);

        renderer.render((float)now, camera.getViewMatrix(), projection);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, 1);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

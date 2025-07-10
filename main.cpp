#define GLM_COMPILER 0
#include <iostream>
#include <iomanip>
#include <string>
#include <glad/glad.h>                  // library for loading OpenGL functions (like glClear or glViewport)
#include <GLFW/glfw3.h>                 // library for creating windows and handling input – mouse clicks, keyboard input, or window resizes
#include <glm/glm.hpp>                  // for basic vector and matrix mathematics functions
#include <glm/gtc/matrix_transform.hpp> // for matrix transformation functions
#include <glm/gtc/type_ptr.hpp>         // for matrix conversion to raw pointers (OpenGL compatibility with GLM)

#define STB_IMAGE_IMPLEMENTATION // define a STB_IMAGE_IMPLEMENTATION macro (to tell the compiler to include function implementations)
#include "stb_image.h"           // library for image loading
#include "shader.h"              // implementation of the graphics pipeline
#include "camera.h"              // implementation of the camera system
#include "light.h"               // definition of the light settings and light cube

#include "io/PackPatchReader.h"
#include "resFile.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

// window settings
bool isFullscreen = false;
const unsigned int DEFAULT_SCR_WIDTH = 800;
const unsigned int DEFAULT_SCR_HEIGHT = 600;
const unsigned int DEFAULT_WINDOW_POS_X = 100;
const unsigned int DEFAULT_WINDOW_POS_Y = 100;

unsigned int currentScreenWidth = DEFAULT_SCR_WIDTH;
unsigned int currentScreenHeight = DEFAULT_SCR_HEIGHT;

// create a Camera class instance with a specified position and default values for other parameters, to access its functionality
Camera ourCamera;

float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time of last frame

bool firstMouse = true;                 // flag to check if the mouse movement is being processed for the first time
float lastX = DEFAULT_SCR_WIDTH / 2.0;  // starting cursor position (x-axis)
float lastY = DEFAULT_SCR_HEIGHT / 2.0; // starting cursor position (y-axis)

int main()
{
    // initialize and configure (use core profile mode and OpenGL v3.3)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // GLFW window creation
    GLFWwindow *window = glfwCreateWindow(DEFAULT_SCR_WIDTH, DEFAULT_SCR_HEIGHT, "BDAE Viewer by fata1error404", NULL, NULL);

    // set OpenGL context and callback
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);   // register scroll callback
    glfwSetCursorPosCallback(window, mouse_callback); // register mouse callback
    glfwSetKeyCallback(window, key_callback);         // register key callback

    // hide and lock the mouse cursor to the window
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // load all OpenGL function pointers
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // enable depth testing to ensure correct pixel rendering order in 3D space (depth buffer prevents incorrect overlaying and redrawing of objects)
    glEnable(GL_DEPTH_TEST);

    Shader ourShader("shader model.vs", "shader model.fs");

    // BDAE File Loading
    // _________________

    //
    struct Vertex
    {
        float x, y, z;    // 12 bytes
        float nx, ny, nz; // 12 bytes
        float u, v;       // 8 bytes
        float padding;    // 4 bytes (maybe) or alignment
    };

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int vertexOffset = 0;

    IReadResFile *archiveFile = createReadFile("example.bdae"); // city_sky.bdae

    if (archiveFile)
    {
        CPackPatchReader *archiveReader = new CPackPatchReader(archiveFile, true, false);
        archiveFile->drop();

        IReadResFile *bdaeFile = archiveReader->openFile("little_endian_not_quantized.bdae");

        if (bdaeFile)
        {
            File myFile;
            int result = myFile.Init(bdaeFile);

            std::cout << "\n"
                      << (result != 1 ? "PARSING SUCCESS" : "PARSING ERROR") << std::endl;

            if (result != 1)
            {
                int submeshCount = 0;

                for (int i = 0, n = myFile.StringStorage.size(); i < n; i++)
                {
                    std::string &s = myFile.StringStorage[i];

                    // must be at least 5 chars long to hold "-mesh"
                    if (!s.empty() && s[0] != '#' && s.length() >= 5 && s.rfind("-mesh") == s.length() - 5)
                        ++submeshCount;
                }

                std::cout << "\nRetrieving model vertices, combining " << submeshCount << " submeshes.." << std::endl;

                for (int i = 0; i < submeshCount; i++)
                {
                    unsigned char *submeshVertexDataPtr = (unsigned char *)myFile.RemovableBuffers[i * 2] + 4;
                    unsigned int submeshVertexDataSize = myFile.RemovableBuffersInfo[i * 4] - 4;
                    unsigned int submeshVertexCount = submeshVertexDataSize / sizeof(Vertex);

                    for (int j = 0; j < submeshVertexCount; j++)
                    {
                        float *v = reinterpret_cast<float *>(submeshVertexDataPtr + j * sizeof(Vertex));
                        vertices.push_back(v[0]); // x
                        vertices.push_back(v[1]); // y
                        vertices.push_back(v[2]); // z

                        vertices.push_back(v[3]); //
                        vertices.push_back(v[4]); //
                        vertices.push_back(v[5]); //

                        vertices.push_back(v[6]); // u
                        vertices.push_back(v[7]); // v
                    }

                    unsigned char *submeshIndexDataPtr = (unsigned char *)myFile.RemovableBuffers[i * 2 + 1] + 4;
                    unsigned int submeshIndexDataSize = myFile.RemovableBuffersInfo[i * 4 + 2] - 4;
                    unsigned int submeshIndexCount = submeshIndexDataSize / sizeof(unsigned short);

                    unsigned short *rawIndices = reinterpret_cast<unsigned short *>(submeshIndexDataPtr);

                    for (int k = 0; k < submeshIndexCount; k++)
                        indices.push_back((unsigned int)rawIndices[k] + i * vertexOffset);

                    vertexOffset += submeshVertexCount;
                }
            }
        }

        delete bdaeFile;
        delete archiveReader;
    }

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    std::cout << "Total vertices: " << vertices.size() / 8 << std::endl;
    std::cout << "Total faces: " << indices.size() / 3 << std::endl;

    // load the model's texture
    // ________________________

    unsigned int texture;
    glGenTextures(1, &texture);            // generate a texture object to store texture data
    glBindTexture(GL_TEXTURE_2D, texture); // bind the texture object so that all upcoming texture operations affect this texture

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for s (x) axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // for t (y) axis

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load("texture/sky_city.png", &width, &height, &nrChannels, 0); // load the image and its parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); // create and store texture image inside the texture object
    glGenerateMipmap(GL_TEXTURE_2D);                                                          // generate a mipmap
    stbi_image_free(data);

    ourShader.use();
    ourShader.setVec3("lightPos", lightPos);
    ourShader.setVec3("lightColor", lightColor);
    ourShader.setFloat("ambientStrength", ambientStrength);
    ourShader.setFloat("diffuseStrength", diffuseStrength);
    ourShader.setFloat("specularStrength", specularStrength);

    Light lightSource(ourCamera);

    // game loop
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // handle keyboard input
        processInput(window);

        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the color buffer (fill the screen with a clear color) and the depth buffer; otherwise the information of the previous frame stays in these buffers

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = ourCamera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(ourCamera.Zoom), (float)currentScreenWidth / (float)currentScreenHeight, 0.1f, 1000.0f);

        ourShader.use();
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        ourShader.setBool("lighting", showLighting);
        ourShader.setVec3("cameraPos", ourCamera.Position);

        glBindVertexArray(VAO);
        ourShader.setInt("renderMode", 2);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        ourShader.setInt("renderMode", 1);
        ourShader.setInt("modelTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // render light cube
        lightSource.draw(view, projection);

        glfwSwapBuffers(window); // make the contents of the back buffer (stores the completed frames) visible on the screen
        glfwPollEvents();        // if any events are triggered (like keyboard input or mouse movement events), updates the window state, and calls the corresponding functions (which we can register via callback methods)
    }

    // terminate, clearing all previously allocated GLFW resources
    glfwTerminate();
    return 0;
}

// whenever the window size changed (by OS or user resize), this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// whenever the mouse uses scroll wheel, this callback function executes
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    // handle mouse wheel scroll input using the Camera class function
    ourCamera.ProcessMouseScroll(yoffset);
}

// whenever the mouse moves, this callback function executes
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    // handle the first mouse movement to prevent a sudden jump
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // calculate the mouse offset since the last frame
    // (xpos and ypos are the current cursor coordinates in screen space)
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    // handle mouse movement input using the Camera class function
    ourCamera.ProcessMouseMovement(xoffset, yoffset);
}

// whenever a key is pressed, this callback function executes and only 1 time, preventing continuous toggling when a key is held down (which would occur in processInput)
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_L:
            showLighting = !showLighting;
            break;
        case GLFW_KEY_F:
        {
            isFullscreen = !isFullscreen;
            if (isFullscreen)
            {
                // switch to fullscreen mode on primary monitor
                GLFWmonitor *monitor = glfwGetPrimaryMonitor();      // main display in the system
                const GLFWvidmode *mode = glfwGetVideoMode(monitor); // video mode (info like resolution, color depth, refresh rate)
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);

                currentScreenWidth = mode->width;
                currentScreenHeight = mode->height;
            }
            else
            {
                // restore to windowed mode with default position + size
                glfwSetWindowMonitor(window, NULL, DEFAULT_WINDOW_POS_X, DEFAULT_WINDOW_POS_Y, DEFAULT_SCR_WIDTH, DEFAULT_SCR_HEIGHT, 0);

                // reset mouse to avoid camera jump
                double xpos, ypos;
                glfwGetCursorPos(window, &xpos, &ypos);
                lastX = xpos;
                lastY = ypos;
                firstMouse = true;

                currentScreenWidth = DEFAULT_SCR_WIDTH;
                currentScreenHeight = DEFAULT_SCR_HEIGHT;
            }
        }
        break;
        }
    }
}

// process all input: query GLFW whether relevant keys are pressed / released this frame and react accordingly
void processInput(GLFWwindow *window)
{
    // Escape key to close the program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    /*
       WASD keys for camera movement:
       W – move forward (along the camera's viewing direction vector, i.e. z-axis)
       S – move backward
       A – move left (along the right vector, i.e. x-axis; computed using the cross product)
       D – move right
    */
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        ourCamera.ProcessKeyboard(FORWARD);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        ourCamera.ProcessKeyboard(BACKWARD);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        ourCamera.ProcessKeyboard(LEFT);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        ourCamera.ProcessKeyboard(RIGHT);

    ourCamera.UpdatePosition(deltaTime);
}

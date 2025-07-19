#include <iostream>
#include <iomanip>
#include <string>
#include "libs/glad/glad.h"             // library for loading OpenGL functions (like glClear or glViewport)
#include <GLFW/glfw3.h>                 // library for creating windows and handling input – mouse clicks, keyboard input, or window resizes
#include <glm/glm.hpp>                  // for basic vector and matrix mathematics functions
#include <glm/gtc/matrix_transform.hpp> // for matrix transformation functions
#include <glm/gtc/type_ptr.hpp>         // for matrix conversion to raw pointers (OpenGL compatibility with GLM)

#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_opengl3.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/ImGuiFileDialog.h"

#define STB_IMAGE_IMPLEMENTATION // define a STB_IMAGE_IMPLEMENTATION macro (to tell the compiler to include function implementations)
#include "libs/stb_image.h"      // library for image loading
#include "shader.h"              // implementation of the graphics pipeline
#include "camera.h"              // implementation of the camera system
#include "light.h"               // definition of the light settings and light cube

#include "libs/io/PackPatchReader.h"
#include "resFile.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
void loadBDAEModel(const char *fpath);

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

// viewer variables
bool displayBaseMesh = false; // flag that indicates base / textured mesh display mode
bool modelLoaded = false;     // flag that indicates whether to display model info and settings

std::string fileName;
int fileSize, vertexCount, faceCount, textureCount;
unsigned int VAO, VBO, EBO;
std::vector<float> vertices;
std::vector<unsigned int> indices;
std::vector<unsigned int> textures;

int main()
{
    // initialize and configure (use core profile mode and OpenGL v3.3)
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // GLFW window creation
    GLFWwindow *window = glfwCreateWindow(DEFAULT_SCR_WIDTH, DEFAULT_SCR_HEIGHT, "BDAE 3D Model Viewer", NULL, NULL);

    // set window icon
    int width, height, nrChannels;
    unsigned char *data = stbi_load("aux_docs/icon.png", &width, &height, &nrChannels, 0);
    GLFWimage icon;
    icon.width = width;
    icon.height = height;
    icon.pixels = data;
    glfwSetWindowIcon(window, 1, &icon);
    stbi_image_free(data);

    // set OpenGL context and callback
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);   // register scroll callback
    glfwSetCursorPosCallback(window, mouse_callback); // register mouse callback
    glfwSetKeyCallback(window, key_callback);         // register key callback

    // load all OpenGL function pointers
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // setup settings panel (Dear ImGui library)
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    ImGui::GetIO().IniFilename = NULL; // disable saving UI states to .ini file

    // apply styles to have a grayscale theme
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;                                               // border radius
    style.WindowBorderSize = 0.0f;                                             // border width
    style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);              // text color
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);      // background color of the panel's main content area
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // background color of the panel's title bar
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);  // .. (when panel is hidden)
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);       // .. (when panel is overlayed and inactive)
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);           // background color of input fields, checkboxes
    style.Colors[ImGuiCol_Button] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);            // background color of buttons
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);         // mark color in checkboxes
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f); // background color of table headers (for file browsing dialog)

    // configure file browsing dialog
    IGFD::FileDialogConfig cfg;
    cfg.path = "./model/creature/pet";                                                     // default path
    cfg.fileName = "";                                                                     // default file name (none)
    cfg.filePathName = "";                                                                 // default file path name (none)
    cfg.countSelectionMax = 1;                                                             // only allow to select one file
    cfg.flags = ImGuiFileDialogFlags_HideColumnType | ImGuiFileDialogFlags_HideColumnDate; // flags: hide file type and date columns
    cfg.userFileAttributes = NULL;                                                         // no custom columns
    cfg.userDatas = NULL;                                                                  // no custom user data passed to the dialog
    cfg.sidePane = NULL;                                                                   // no side panel
    cfg.sidePaneWidth = 0.0f;                                                              // side panel width (unused)

    // enable depth testing to ensure correct pixel rendering order in 3D space (depth buffer prevents incorrect overlaying and redrawing of objects)
    glEnable(GL_DEPTH_TEST);

    Shader ourShader("shader model.vs", "shader model.fs");

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

        // prepare ImGui for a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // define settings panel with fixed size and position
        ImGui::SetNextWindowSize(ImVec2(200.0f, 200.0f), ImGuiCond_None);
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_None);
        ImGui::Begin("Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // add a button that opens file browsing dialog
        if (ImGui::Button("Load Model"))
        {
            IGFD::FileDialog::Instance()->OpenDialog(
                "File_Browsing_Dialog", // dialog ID (used to reference this dialog instance)
                "Load .bdae Model",     // dialog title
                ".bdae",                // file extension filter
                cfg                     // config
            );
        }

        // define file browsing dialog with fixed size and position in the center
        ImVec2 dialogSize(600.0f, 400.0f);
        ImVec2 dialogPos((currentScreenWidth - dialogSize.x) * 0.5f, (currentScreenHeight - dialogSize.y) * 0.5f);
        ImGui::SetNextWindowSize(dialogSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(dialogPos, ImGuiCond_Always);

        // if the dialog is open
        if (IGFD::FileDialog::Instance()->Display("File_Browsing_Dialog", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            // if selection is confirmed (by OK or double-click), process it
            if (IGFD::FileDialog::Instance()->IsOk())
            {
                std::map<std::string, std::string> selection = IGFD::FileDialog::Instance()->GetSelection(); // returns pairs (file name, full path)
                loadBDAEModel(selection.begin()->second.c_str());
            }

            IGFD::FileDialog::Instance()->Close(); // close the dialog after handling OK or Cancel
        }

        // if a model is loaded, show its stats + checkboxes
        if (modelLoaded)
        {
            ImGui::Spacing();
            ImGui::Text("File: %s", fileName.c_str());
            ImGui::Text("Size: %d Bytes", fileSize);
            ImGui::Text("Vertices: %d", vertexCount);
            ImGui::Text("Faces: %d", faceCount);
            ImGui::NewLine();
            ImGui::Checkbox("Base Mesh On/Off", &displayBaseMesh);
            ImGui::Checkbox("Lighting On/Off", &showLighting);
        }

        ImGui::End();

        glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the color buffer (fill the screen with a clear color) and the depth buffer; otherwise the information of the previous frame stays in these buffers

        // update dynamic shader uniforms on GPU
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = ourCamera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(ourCamera.Zoom), (float)currentScreenWidth / (float)currentScreenHeight, 0.1f, 1000.0f);

        ourShader.use();
        ourShader.setMat4("model", model);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);
        ourShader.setBool("lighting", showLighting);
        ourShader.setVec3("cameraPos", ourCamera.Position);

        // render model
        glBindVertexArray(VAO);

        if (!displayBaseMesh)
        {
            /*
            [TODO] multiple texture models doesn't blend them correctly

            ourShader.setInt("textureCount", textureCount);

            for (int i = 0; i < textureCount; i++)
            {
                glActiveTexture(GL_TEXTURE0 + i);                   // activate texture unit i
                glBindTexture(GL_TEXTURE_2D, textures[i]);          // bind texture
                ourShader.setInt("texture" + std::to_string(i), i); // assign texture unit to sampler
            }
            */

            ourShader.setInt("renderMode", 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }
        else
        {
            ourShader.setInt("renderMode", 2);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            ourShader.setInt("renderMode", 3);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        // render light cube
        lightSource.draw(view, projection);

        // render settings panel (and file browsing dialog, if open)
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
    // [TODO] EXPLAIN
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
        firstMouse = true;

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
        case GLFW_KEY_K:
            displayBaseMesh = !displayBaseMesh;
            break;
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

// BDAE File Loading
// _________________

void loadBDAEModel(const char *fpath)
{
    // 1. clear GPU memory and reset viewer state
    if (VAO)
    {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }

    if (VBO)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (EBO)
    {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }

    if (!textures.empty())
    {
        glDeleteTextures(textureCount, textures.data());
        textures.clear();
    }

    vertices.clear();
    indices.clear();
    fileSize = vertexCount = faceCount = textureCount = 0;
    std::vector<std::string> textureNames;

    // 2. load and parse the .bdae file, building the mesh vertex and index data
    IReadResFile *archiveFile = createReadFile(fpath);

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
                int submeshCount, meshMetadataOffset;
                char *ptr = (char *)myFile.DataBuffer + 80 + 120;
                memcpy(&submeshCount, ptr, sizeof(int));
                memcpy(&meshMetadataOffset, ptr + 4, sizeof(int));

                int submeshVertexCount[submeshCount], submeshMetadataOffset[submeshCount];

                for (int i = 0; i < submeshCount; i++)
                {
                    memcpy(&submeshMetadataOffset[i], ptr + 4 + meshMetadataOffset + 20 + i * 24, sizeof(int));
                    memcpy(&submeshVertexCount[i], ptr + 4 + meshMetadataOffset + 20 + i * 24 + submeshMetadataOffset[i] + 4, sizeof(int));
                }

                std::cout << "\nRetrieving model vertices, combining " << submeshCount << " submeshes.." << std::endl;
                int vertexOffset = 0;

                for (int i = 0; i < submeshCount; i++)
                {
                    unsigned char *submeshVertexDataPtr = (unsigned char *)myFile.RemovableBuffers[i * 2] + 4;
                    unsigned int submeshVertexDataSize = myFile.RemovableBuffersInfo[i * 4] - 4;
                    unsigned int bytesPerVertex = submeshVertexDataSize / submeshVertexCount[i];

                    for (int j = 0; j < submeshVertexCount[i]; j++)
                    {
                        float *v = reinterpret_cast<float *>(submeshVertexDataPtr + j * bytesPerVertex);
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

                    vertexOffset += submeshVertexCount[i];
                }

                // retrieve textures
                int textureMetadataOffset;
                ptr = (char *)myFile.DataBuffer + 80 + 96;
                memcpy(&textureCount, ptr, sizeof(int));
                memcpy(&textureMetadataOffset, ptr + 4, sizeof(int));

                std::cout << "TEXTURES: " << textureCount << std::endl;
                textures.resize(textureCount);

                for (int i = 0, n = myFile.StringStorage.size(); i < n; i++)
                {
                    std::string &s = myFile.StringStorage[i];

                    for (char &c : s)
                        c = std::tolower(static_cast<unsigned char>(c));

                    if (s.length() >= 4 && s.compare(s.length() - 4, 4, ".tga") == 0 && s[0] != '_')
                    {
                        // build the png name
                        std::string tex = s;
                        tex.replace(tex.length() - 4, 4, ".png");

                        // ensure it starts with "texture/"
                        const std::string prefix = "texture/";
                        if (tex.rfind(prefix, 0) != 0)
                            tex = prefix + tex;

                        if (std::find(textureNames.begin(),
                                      textureNames.end(),
                                      tex) == textureNames.end())
                        {
                            textureNames.push_back(tex);
                        }
                    }
                }

                for (int i = 0; i < (int)textureNames.size(); ++i)
                    std::cout << textureNames[i] << std::endl;

                std::string pathStr(fpath); // convert to std::string
                size_t lastSlash = pathStr.find_last_of("/\\");
                fileName = (lastSlash == std::string::npos)
                               ? pathStr
                               : pathStr.substr(lastSlash + 1);

                vertexCount = vertices.size() / 8;
                faceCount = indices.size() / 3;
                fileSize = myFile.getSize();
            }
        }

        delete bdaeFile;
        delete archiveReader;
    }

    // 3. setup buffers
    glGenVertexArrays(1, &VAO); // generate a Vertex Attribute Object to store vertex attribute configurations
    glGenBuffers(1, &VBO);      // generate a Vertex Buffer Object to store vertex data
    glGenBuffers(1, &EBO);      // generate an Element Buffer Object to store index data

    glBindVertexArray(VAO); // bind the VAO first so that subsequent VBO bindings and vertex attribute configurations are stored in it correctly

    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                                              // bind the VBO
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW); // copy vertex data into the GPU buffer's memory

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0); // define the layout of the vertex data (vertex attribute configuration): index 0, 3 components per vertex, type float, not normalized, with a stride of 8 * sizeof(float) (next vertex starts after 8 floats), and an offset of 0 in the buffer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // 4. load texture(s)
    glGenTextures(textureCount, textures.data()); // generate and store texture ID(s)

    for (int i = 0; i < textureCount; i++)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]); // bind the texture ID so that all upcoming texture operations affect this texture

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // for s (x) axis
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // for t (y) axis

        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels, format;
        unsigned char *data = stbi_load(textureNames[i].c_str(), &width, &height, &nrChannels, 0); // load the image and its parameters

        if (!data)
        {
            std::cerr << "Failed to load texture: " << textureNames[i] << "\n";
            continue;
        }

        format = (nrChannels == 4) ? GL_RGBA : GL_RGB;                                            // image format
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); // create and store texture image inside the texture object (upload to GPU)
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }

    modelLoaded = true;
}

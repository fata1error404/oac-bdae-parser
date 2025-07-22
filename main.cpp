#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
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
bool fileDialogOpen = false;  // flag that indicates whether to block background inputs

std::string fileName;
int fileSize, vertexCount, faceCount, textureCount, totalSubmeshCount;
unsigned int VAO, VBO;
std::vector<unsigned int> EBOs;
std::vector<float> vertices;
std::vector<std::vector<unsigned short>> indices;
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
    style.WindowRounding = 4.0f;                                              // border radius
    style.WindowBorderSize = 0.0f;                                            // border width
    style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);             // text color
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);         // background color of the panel's main content area
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);    // background color of the panel's title bar
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // .. (when panel is hidden)
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);          // .. (when panel is overlayed and inactive)
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);          // background color of input fields, checkboxes
    style.Colors[ImGuiCol_Button] = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);           // background color of buttons
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);        // mark color in checkboxes
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f); // background color of table headers (for file browsing dialog)
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);   // background color of scrollbar track (for file browsing dialog)
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f); // background color of scrollbar thumb (for file browsing dialog)
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);       // background color of tooltips (for file browsing dialog)

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
        if (!fileDialogOpen)
            processInput(window);

        // prepare ImGui for a new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // define settings panel with fixed size and position
        ImGui::SetNextWindowSize(ImVec2(200.0f, 210.0f), ImGuiCond_None);
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

        fileDialogOpen = IGFD::FileDialog::Instance()->IsOpened("File_Browsing_Dialog");

        // if the dialog is opened with the load button, show it
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
            ImGui::TextWrapped("File:\xC2\xA0%s", fileName.c_str());
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
            ourShader.setInt("renderMode", 1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            for (int i = 0; i < totalSubmeshCount; i++)
            {
                if (textureCount == totalSubmeshCount)
                {
                    glActiveTexture(GL_TEXTURE0); // [TODO] textures are assigned to the wrong submeshes
                    glBindTexture(GL_TEXTURE_2D, textures[i]);
                }
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
                glDrawElements(GL_TRIANGLES, indices[i].size(), GL_UNSIGNED_SHORT, 0);
            }
        }
        else
        {
            // first pass: render mesh edges (wireframe mode)
            ourShader.setInt("renderMode", 2);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            for (int i = 0; i < totalSubmeshCount; i++)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
                glDrawElements(GL_TRIANGLES, indices[i].size(), GL_UNSIGNED_SHORT, 0);
            }

            // second pass: render mesh faces
            ourShader.setInt("renderMode", 3);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            for (int i = 0; i < totalSubmeshCount; i++)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
                glDrawElements(GL_TRIANGLES, indices[i].size(), GL_UNSIGNED_SHORT, 0);
            }
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
    if (fileDialogOpen)
        return;

    // handle mouse wheel scroll input using the Camera class function
    ourCamera.ProcessMouseScroll(yoffset);
}

// whenever the mouse moves, this callback function executes
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (fileDialogOpen)
        return;

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

    if (!EBOs.empty())
    {
        glDeleteBuffers(totalSubmeshCount, EBOs.data());
        EBOs.clear();
    }

    if (!textures.empty())
    {
        glDeleteTextures(textureCount, textures.data());
        textures.clear();
    }

    vertices.clear();
    indices.clear();
    fileSize = vertexCount = faceCount = textureCount = totalSubmeshCount = 0;
    std::vector<std::string> textureNames;

    // 2. load and parse the .bdae file, building the mesh vertex and index data
    CPackPatchReader *bdaeArchive = new CPackPatchReader(fpath, true, false);           // open outer .bdae archive file
    IReadResFile *bdaeFile = bdaeArchive->openFile("little_endian_not_quantized.bdae"); // open inner .bdae file

    if (bdaeFile)
    {
        File myFile;
        int result = myFile.Init(bdaeFile); // run the parser

        std::cout << "\n"
                  << (result != 1 ? "INITIALIZATION SUCCESS" : "INITIALIZATION ERROR") << std::endl;

        if (result != 1)
        {
            std::cout << "\nRetrieving model vertex and index data, loading textures.." << std::endl;

            // retrieve the number of meshes, submeshes, and vertex count for each one
            int meshCount, meshInfoOffset;
            char *ptr = (char *)myFile.DataBuffer + 80 + 120; // points to mesh info in the Data section
            memcpy(&meshCount, ptr, sizeof(int));
            memcpy(&meshInfoOffset, ptr + 4, sizeof(int));

            std::cout << "MESHES: " << meshCount << std::endl;

            int meshVertexCount[meshCount], submeshCount[meshCount], meshMetadataOffset[meshCount];

            for (int i = 0; i < meshCount; i++)
            {
                memcpy(&meshMetadataOffset[i], ptr + 4 + meshInfoOffset + 20 + i * 24, sizeof(int));
                memcpy(&meshVertexCount[i], ptr + 4 + meshInfoOffset + 20 + i * 24 + meshMetadataOffset[i] + 4, sizeof(int));
                memcpy(&submeshCount[i], ptr + 4 + meshInfoOffset + 20 + i * 24 + meshMetadataOffset[i] + 12, sizeof(int));

                std::cout << "[" << i + 1 << "]  " << meshVertexCount[i] << " vertices, " << submeshCount[i] << " submeshes" << std::endl;
                totalSubmeshCount += submeshCount[i];
            }

            indices.resize(totalSubmeshCount);
            int currentSubmeshIndex = 0;

            // loop through each mesh, retrieve its vertex and index data; all vertex data is stored in a single flat vector, while index data is stored in separate vectors for each submesh
            for (int i = 0; i < meshCount; i++)
            {
                unsigned char *meshVertexDataPtr = (unsigned char *)myFile.RemovableBuffers[i + currentSubmeshIndex] + 4;
                unsigned int meshVertexDataSize = myFile.RemovableBuffersInfo[(i + currentSubmeshIndex) * 2] - 4;
                unsigned int bytesPerVertex = meshVertexDataSize / meshVertexCount[i];

                for (int j = 0; j < meshVertexCount[i]; j++)
                {
                    float vertex[8]; // each vertex has 3 position, 3 normal, and 2 texture coordinates (total of 8 float components; in fact, in the .bdae file there are more than 8 variables per vertex, that's why bytesPerVertex is more than 8 * sizeof(float))
                    memcpy(vertex, meshVertexDataPtr + j * bytesPerVertex, sizeof(vertex));

                    vertices.push_back(vertex[0]); // X
                    vertices.push_back(vertex[1]); // Y
                    vertices.push_back(vertex[2]); // Z

                    vertices.push_back(vertex[3]); // Nx
                    vertices.push_back(vertex[4]); // Ny
                    vertices.push_back(vertex[5]); // Nz

                    vertices.push_back(vertex[6]); // S
                    vertices.push_back(vertex[7]); // T
                }

                for (int k = 0; k < submeshCount[i]; k++)
                {
                    unsigned char *submeshIndexDataPtr = (unsigned char *)myFile.RemovableBuffers[i + currentSubmeshIndex + 1] + 4;
                    unsigned int submeshIndexDataSize = myFile.RemovableBuffersInfo[(i + currentSubmeshIndex) * 2 + 2] - 4;
                    unsigned int submeshTriangleCount = submeshIndexDataSize / (3 * sizeof(unsigned short));

                    for (int l = 0; l < submeshTriangleCount; l++)
                    {
                        unsigned short triangle[3];
                        memcpy(triangle, submeshIndexDataPtr + l * sizeof(triangle), sizeof(triangle));

                        indices[currentSubmeshIndex].push_back(triangle[0]);
                        indices[currentSubmeshIndex].push_back(triangle[1]);
                        indices[currentSubmeshIndex].push_back(triangle[2]);
                        faceCount++;
                    }

                    currentSubmeshIndex++;
                }
            }

            // search for texture names
            ptr = (char *)myFile.DataBuffer + 80 + 96;
            memcpy(&textureCount, ptr, sizeof(int));

            std::cout << "\nTEXTURES: " << ((textureCount != 0) ? std::to_string(textureCount) : "0, file name will be used as a texture name") << std::endl;

            // [TODO] implement a more robust approach
            // loop through each retrieved string and find those that are texture names
            for (int i = 0, n = myFile.StringStorage.size(); i < n; i++)
            {
                std::string s = myFile.StringStorage[i];

                // convert to lowercase
                for (char &c : s)
                    c = std::tolower(c);

                // remove '/avatar' if it exists
                int avatarPos = s.find("/avatar");
                if (avatarPos != std::string::npos)
                    s.erase(avatarPos, 7);

                // a string is a texture file name if it ends with '.tga' and doesn't start with '_'
                if (s.length() >= 4 && s.compare(s.length() - 4, 4, ".tga") == 0 && s[0] != '_')
                {
                    // ensure it starts with "texture/"
                    if (s.rfind("texture/", 0) != 0)
                        s = "texture/" + s;

                    // replace the ending with '.png'
                    s.replace(s.length() - 4, 4, ".png");

                    // ensure it is a unique texture name
                    if (std::find(textureNames.begin(), textureNames.end(), s) == textureNames.end())
                        textureNames.push_back(s);
                }
            }

            // set file info to be displayed in the settings panel
            std::string pathStr(fpath);
            fileName = pathStr.substr(pathStr.find_last_of("/\\") + 1); // file name is after the last path separator in the full path
            fileSize = myFile.Size;
            vertexCount = vertices.size() / 8;

            /*
            if (textureNames.size() == 1)
            {
                std::string baseName = textureNames[0];
                std::string textureDir = "texture/";

                std::cout << "Searching for alternative color textures..." << std::endl;

                // Extract just the base file name (without path and extension)
                std::string stem = std::filesystem::path(baseName).stem().string(); // "pet_ents"
                std::vector<std::string> alternatives;

                for (const auto &entry : std::filesystem::directory_iterator(textureDir))
                {
                    if (!entry.is_regular_file())
                        continue;

                    std::string filename = entry.path().filename().string(); // "pet_ents_red.png"

                    // Convert to lowercase (optional, depending on your platform)
                    std::string lower = filename;
                    for (char &c : lower)
                        c = std::tolower(c);

                    // Must be a .png file, start with stem + '_', and not be exactly baseName
                    if (lower.size() > stem.size() + 1 &&
                        lower.compare(0, stem.size(), stem) == 0 &&
                        lower[stem.size()] == '_' &&
                        lower.compare(lower.size() - 4, 4, ".png") == 0 &&
                        (textureDir + lower != baseName))
                    {
                        std::string fullPath = textureDir + lower;

                        // Avoid duplicates
                        if (std::find(textureNames.begin(), textureNames.end(), fullPath) == textureNames.end())
                        {
                            std::cout << "Found alternative: " << fullPath << std::endl;
                            textureNames.push_back(fullPath);
                        }
                    }
                }
            }
            */

            // if the texture name is missing in the .bdae file, use this file's name instead (assuming the texture file was manually found and named).
            if (textureNames.empty())
            {
                std::string s = "texture/" + fileName;
                textureNames.push_back(s.replace(s.length() - 5, 5, ".png"));
                textureCount++;
            }

            for (int i = 0; i < (int)textureNames.size(); i++)
                std::cout << "[" << i + 1 << "]  " << textureNames[i] << std::endl;
        }

        free(myFile.DataBuffer);
        delete[] static_cast<char *>(myFile.RemovableBuffers[0]);
        delete[] myFile.RemovableBuffers;
        delete[] myFile.RemovableBuffersInfo;
    }

    delete bdaeFile;
    delete bdaeArchive;

    // 3. setup buffers
    EBOs.resize(totalSubmeshCount);
    glGenVertexArrays(1, &VAO);                   // generate a Vertex Attribute Object to store vertex attribute configurations
    glGenBuffers(1, &VBO);                        // generate a Vertex Buffer Object to store vertex data
    glGenBuffers(totalSubmeshCount, EBOs.data()); // generate an Element Buffer Object for each submesh to store index data

    glBindVertexArray(VAO); // bind the VAO first so that subsequent VBO bindings and vertex attribute configurations are stored in it correctly

    glBindBuffer(GL_ARRAY_BUFFER, VBO);                                                              // bind the VBO
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW); // copy vertex data into the GPU buffer's memory

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0); // define the layout of the vertex data (vertex attribute configuration): index 0, 3 components per vertex, type float, not normalized, with a stride of 8 * sizeof(float) (next vertex starts after 8 floats), and an offset of 0 in the buffer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    for (int i = 0; i < totalSubmeshCount; i++)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices[i].size() * sizeof(unsigned short), indices[i].data(), GL_STATIC_DRAW);
    }

    // 4. load texture(s)
    textures.resize(textureCount);
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

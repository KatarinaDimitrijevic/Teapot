#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

unsigned int loadTexture(char const * path);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct DirLight {
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 roomPosition = glm::vec3(0.0,0.0,0.0);
    float roomScale = 1.0f;
    PointLight pointLight;
    DirLight dirLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 0)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    //programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;



    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader roomShader("resources/shaders/roomShader.vs", "resources/shaders/roomShader.fs");
    Shader modelsShader("resources/shaders/modelsShader.vs", "resources/shaders/modelsShader.fs");


    // load models
    // -----------
    Model room("resources/objects/soba_zavrsena/soba_zavrsena.obj");
    room.SetShaderTextureNamePrefix("material.");

    Model table("resources/objects/sto_iz_blendera/table.obj");
    table.SetShaderTextureNamePrefix("material.");

    Model chair("resources/objects/stolica/Lucien_Lilippe_Chaise_Louis_XVI/Chaise_louisXVI_deco2.obj");
    chair.SetShaderTextureNamePrefix("material.");

    Model teapot("resources/objects/teapot/teapot_n_glass.obj");
    teapot.SetShaderTextureNamePrefix("material.");

    Model cup("resources/objects/soljica/cup.obj");
    cup.SetShaderTextureNamePrefix("material.");

//    Model plant("resources/objects/POKUSAJ_BILJKE/eb_house_plant_02/eb_house_plant_02.obj");
//    plant.SetShaderTextureNamePrefix("material.");


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(0.5f, 2.0f, 0.0f);
    pointLight.ambient = glm::vec3(0.4, 0.4, 0.4);
    pointLight.diffuse = glm::vec3(0.4, 0.4, 0.4);
    pointLight.specular = glm::vec3(0.45, 0.45, 0.45);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    DirLight& dirLight = programState->dirLight;
    dirLight.ambient = glm::vec3(0.3 ,0.3 ,0.3);
    dirLight.diffuse = glm::vec3(0.01, 0.01, 0.01);
    dirLight.specular = glm::vec3(0.05, 0.05, 0.05);
    dirLight.direction = glm::vec3(-5.0, 1.0, -1.5);


//    unsigned int map = loadTexture("resources/textures/awesomeface.png");
//    tableShader.use();
//    tableShader.setInt("texture1", 0);

    // shader configuration
    // --------------------


    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        roomShader.use();

        roomShader.setVec3("pointLight.position", pointLight.position);
        roomShader.setVec3("pointLight.ambient", pointLight.ambient);
        roomShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        roomShader.setVec3("pointLight.specular", pointLight.specular);
        roomShader.setFloat("pointLight.constant", pointLight.constant);
        roomShader.setFloat("pointLight.linear", pointLight.linear);
        roomShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        roomShader.setVec3("viewPosition", programState->camera.Position);
        roomShader.setFloat("material.shininess", 2.0f);

        roomShader.setVec3("dirLight.ambient", dirLight.ambient);
        roomShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        roomShader.setVec3("dirLight.specular", dirLight.specular);
        roomShader.setVec3("dirLight.direction", dirLight.direction);

        modelsShader.use();
        modelsShader.setVec3("viewPosition", programState->camera.Position);
        modelsShader.setFloat("material.shininess", 16.0f);

        modelsShader.setVec3("dirLight.ambient", dirLight.ambient);
        modelsShader.setVec3("dirLight.diffuse", dirLight.diffuse);
        modelsShader.setVec3("dirLight.specular", dirLight.specular);
        modelsShader.setVec3("dirLight.direction", dirLight.direction);

        modelsShader.setVec3("pointLight.position", pointLight.position);
        modelsShader.setVec3("pointLight.ambient", pointLight.ambient);
        modelsShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        modelsShader.setVec3("pointLight.specular", pointLight.specular);
        modelsShader.setFloat("pointLight.constant", pointLight.constant);
        modelsShader.setFloat("pointLight.linear", pointLight.linear);
        modelsShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        
        // view/projection transformations
        roomShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        roomShader.setMat4("projection", projection);
        roomShader.setMat4("view", view);


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->roomPosition); // translate it down so it's at the center of the scene
        //model = glm::rotate(model, glm::radians(40.0f), glm::vec3(1.0,1.0 ,0.0));
        model = glm::scale(model, glm::vec3(programState->roomScale));    // it's a bit too big for our scene, so scale it down
        roomShader.setMat4("model", model);
        room.Draw(roomShader);

        modelsShader.use();
        modelsShader.setMat4("projection", projection);  
        modelsShader.setMat4("view", view);

        model = glm::translate(model, glm::vec3(0.0, -0.55, 0.0));
        //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
        model = glm::scale(model, glm::vec3(0.2, 0.25, 0.2));    // it's a bit too big for our scene, so scale it down
        modelsShader.setMat4("model", model);
        table.Draw(modelsShader);

        // TODO ovde fali neka biljka koju crta isto modelsShader

//        model = glm::mat4(1.0);
//        model = glm::translate(model,
//                               programState->roomPosition + glm::vec3(3.0, 0.01, 0.0));
//        model = glm::scale(model, glm::vec3(0.025));
//        modelsShader.setMat4("model", model);
//        plant.Draw(modelsShader);


//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, map);

        modelsShader.use();

        model = glm::mat4(1.0);
        model = glm::translate(model,
                               programState->roomPosition + glm::vec3(0.5, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(-25.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(1.5));
        modelsShader.setMat4("model", model);
        chair.Draw(modelsShader);

        model = glm::mat4(1.0);
        model = glm::translate(model,
                               programState->roomPosition + glm::vec3(-0.5, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(155.0f), glm::vec3(0.0, 1.0, 0.0));
        model = glm::scale(model, glm::vec3(1.5));
        modelsShader.setMat4("model", model);
        chair.Draw(modelsShader);


        model = glm::mat4(1.0);
        model = glm::translate(model,
                               programState->roomPosition + glm::vec3(-0.65, 0.415, 0.45));
        //model = glm::scale(model, glm::vec3(0.65));
        modelsShader.setMat4("model", model);
        teapot.Draw(modelsShader);

        model = glm::mat4(1.0);
        model = glm::translate(model,
                               programState->roomPosition + glm::vec3(0.0, 1.15, 0.58));
        model = glm::scale(model, glm::vec3(0.5));
        modelsShader.setMat4("model", model);
        cup.Draw(modelsShader);

        model = glm::mat4(1.0);
        model = glm::translate(model,
                               programState->roomPosition + glm::vec3(0.0, 1.15, -0.58));
        model = glm::scale(model, glm::vec3(0.5));
        modelsShader.setMat4("model", model);
        cup.Draw(modelsShader);



        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Room position", (float*)&programState->roomPosition);
        ImGui::DragFloat("Room scale", &programState->roomScale, 0.05, 0.1, 10.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::DragFloat3("Camera position", (float*)&programState->roomPosition);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}




unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
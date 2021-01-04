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

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 roomPosition = glm::vec3(0.0,0.0,0.0);
    float roomScale = 1.0f;
    PointLight pointLight;
    SpotLight spotLight;

    float deltaY = 0;
    float deltaZ = 0;

    bool spotLightEnabled = false;
    bool blurEnabled = false;

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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");


    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader roomShader("resources/shaders/roomShader.vs", "resources/shaders/roomShader.fs");
    Shader modelsShader("resources/shaders/modelsShader.vs", "resources/shaders/modelsShader.fs");
    Shader lightShader("resources/shaders/lightShader.vs", "resources/shaders/lightShader.fs");
    Shader paintingShader("resources/shaders/paintingShader.vs", "resources/shaders/paintingShader.fs");

    Shader screenShader("resources/shaders/screenShader.vs", "resources/shaders/screenShader.fs");

    float t = (1 + sqrt(5))/2;
    float u = (5 - sqrt(5))/10;
    float verticesLamp[] = {
            u*t, u, 0,
            -u*t, u, 0,
            u*t, -u, 0,
            -u*t, -u, 0,
            u, 0, u*t,
            u, 0, -u*t,
            -u, 0, u*t,
            -u, 0, -u*t,
            0, u*t, u,
            0, -u*t, u,
            0, u*t, -u,
            0, -u*t, -u,
    };

    unsigned int indicesLamp[] = {
            0, 8, 4, //first triangle
            0, 5, 10, //second triangle
            2, 4, 9, //third triangle
            2, 11, 5, //fourth triangle
            1, 6, 8, //fifth triangle
            1, 10, 7, //sixth triangle,
            3, 9, 6, //seventh triangle
            3, 7, 11, //eighth triangle
            0, 10, 8, //...
            1, 8, 10,
            2, 9, 11,
            3, 11, 9,
            4, 2, 0,
            5, 0, 2,
            6, 1, 3,
            7, 3, 1,
            8, 6, 4,
            9, 4, 6,
            10, 5, 7,
            11, 7, 5
    };

    float verticesPainting[] = {
            //coords              normals              TexCoords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
            0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
            0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
    };

    //lamp
    unsigned int VBO1, VAO1, EBO;
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO1);

    glBindBuffer(GL_ARRAY_BUFFER, VBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesLamp), verticesLamp, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesLamp), indicesLamp, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // painting
    unsigned int VBO2, VAO2;
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glBindVertexArray(VAO2);

    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPainting), verticesPainting, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof (float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof (float)));
    glEnableVertexAttribArray(2);

    //screen
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    //MSAA framebuffer
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a multisampled color attachment texture
    unsigned int textureColorBufferMultiSampled;
    glGenTextures(1, &textureColorBufferMultiSampled);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textureColorBufferMultiSampled, 0);
    // create a (also multisampled) renderbuffer object for depth and stencil attachments
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //second post-processing framebuffer
    unsigned int intermediateFBO;
    glGenFramebuffers(1, &intermediateFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
    // create a color attachment texture
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);	// we only need a color buffer

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // shader configuration
    screenShader.use();
    screenShader.setInt("screenTexture", 0);
    screenShader.setInt("SCR_WIDTH", SCR_WIDTH);
    screenShader.setInt("SCR_HEIGHT", SCR_HEIGHT);


    //diffuse and specular textures
    unsigned int diffuseMap = loadTexture("resources/textures/difuzna.jpg");
    unsigned int specularMap = loadTexture("resources/textures/spekularna1.jpg");

    paintingShader.use();
    paintingShader.setInt("material.diffuse", 0);
    paintingShader.setInt("material.specular", 1);

    // load models
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


    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(0.0f, 3.0f, 0.0f);
    pointLight.ambient = glm::vec3(0.7, 0.7, 0.7);
    pointLight.diffuse = glm::vec3(0.5, 0.5, 0.5);
    pointLight.specular = glm::vec3(0.55, 0.55, 0.55);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.ambient = glm::vec3(0.0f, 0.0f, 0.0f);
    spotLight.diffuse = glm::vec3 (1.0f, 1.0f, 1.0f);
    spotLight.specular = glm::vec3(0.55f, 0.55f, 0.55f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;
    spotLight.cutOff = glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff = glm::cos(glm::radians(20.0f));

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        //camera inside
        if (programState->camera.Position.x < -2.9)
            programState->camera.Position.x = -2.9;
        if (programState->camera.Position.x > 3.1)
            programState->camera.Position.x = 3.1;
        if (programState->camera.Position.y > 2.91)
            programState->camera.Position.y = 2.91;
        if (programState->camera.Position.y < 0.25)
            programState->camera.Position.y = 0.25;
        if (programState->camera.Position.z < -2.8)
            programState->camera.Position.z = -2.8;
        if (programState->camera.Position.z > 2.3)
            programState->camera.Position.z = 2.3;

        //painting inside
        if(programState->deltaY < -1.23)
            programState->deltaY = -1.23;
        if(programState->deltaY > 0.77)
            programState->deltaY = 0.77;
        if(programState->deltaZ < -2.48)
            programState->deltaZ = -2.48;
        if(programState->deltaZ > 2.485)
            programState->deltaZ = 2.485;

        // render
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        //room lights
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

        roomShader.setVec3("spotLight.position", programState->camera.Position);
        roomShader.setVec3("spotLight.direction", programState->camera.Front);
        roomShader.setVec3("spotLight.ambient", spotLight.ambient);
        roomShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        roomShader.setVec3("spotLight.specular", spotLight.specular);
        roomShader.setFloat("spotLight.constant", spotLight.constant);
        roomShader.setFloat("spotLight.linear", spotLight.linear);
        roomShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        roomShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        roomShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);
        roomShader.setBool("spotLightEnabled", programState->spotLightEnabled);

        //models lights
        modelsShader.use();
        modelsShader.setVec3("viewPosition", programState->camera.Position);
        modelsShader.setFloat("material.shininess", 16.0f);

        modelsShader.setVec3("pointLight.position", pointLight.position);
        modelsShader.setVec3("pointLight.ambient", pointLight.ambient);
        modelsShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        modelsShader.setVec3("pointLight.specular", pointLight.specular);
        modelsShader.setFloat("pointLight.constant", pointLight.constant);
        modelsShader.setFloat("pointLight.linear", pointLight.linear);
        modelsShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        modelsShader.setVec3("spotLight.position", programState->camera.Position);
        modelsShader.setVec3("spotLight.direction", programState->camera.Front);
        modelsShader.setVec3("spotLight.ambient", spotLight.ambient);
        modelsShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        modelsShader.setVec3("spotLight.specular", spotLight.specular);
        modelsShader.setFloat("spotLight.constant", spotLight.constant);
        modelsShader.setFloat("spotLight.linear", spotLight.linear);
        modelsShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        modelsShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        modelsShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);
        modelsShader.setBool("spotLightEnabled", programState->spotLightEnabled);

        lightShader.use();
        lightShader.setBool("spotLightEnabled", programState->spotLightEnabled);

        paintingShader.use();
        paintingShader.use();
        paintingShader.setVec3("light.position", pointLight.position);
        paintingShader.setVec3("viewPos", programState->camera.Position);

        // light properties
        paintingShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
        paintingShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
        paintingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // material properties
        paintingShader.setFloat("material.shininess", 64.0f);
        paintingShader.setVec3("pointLight.position", pointLight.position);
        paintingShader.setVec3("pointLight.ambient", pointLight.ambient);
        paintingShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        paintingShader.setVec3("pointLight.specular", pointLight.specular);
        paintingShader.setFloat("pointLight.constant", pointLight.constant);
        paintingShader.setFloat("pointLight.linear", pointLight.linear);
        paintingShader.setFloat("pointLight.quadratic", pointLight.quadratic);

        paintingShader.setVec3("spotLight.position", programState->camera.Position);
        paintingShader.setVec3("spotLight.direction", programState->camera.Front);
        paintingShader.setVec3("spotLight.ambient", spotLight.ambient);
        paintingShader.setVec3("spotLight.diffuse", spotLight.diffuse);
        paintingShader.setVec3("spotLight.specular", spotLight.specular);
        paintingShader.setFloat("spotLight.constant", spotLight.constant);
        paintingShader.setFloat("spotLight.linear", spotLight.linear);
        paintingShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        paintingShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        paintingShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);
        paintingShader.setBool("spotLightEnabled", programState->spotLightEnabled);

        screenShader.use();
        screenShader.setBool("blurEnabled", programState->blurEnabled);

        // view/projection transformations
        roomShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        roomShader.setMat4("projection", projection);
        roomShader.setMat4("view", view);


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,
                               programState->roomPosition);
        model = glm::scale(model, glm::vec3(programState->roomScale));
        roomShader.setMat4("model", model);
        room.Draw(roomShader);

        modelsShader.use();
        modelsShader.setMat4("projection", projection);
        modelsShader.setMat4("view", view);

        model = glm::translate(model, glm::vec3(0.0, -0.55, 0.0));
        model = glm::scale(model, glm::vec3(0.2, 0.25, 0.2));
        modelsShader.setMat4("model", model);
        table.Draw(modelsShader);


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

        //draw the lamp object
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLight.position);
        model = glm::scale(model, glm::vec3(0.3f));
        lightShader.setMat4("model", model);
        glBindVertexArray(VAO1);
        glDrawElements(GL_TRIANGLES, 60, GL_UNSIGNED_INT, 0);

        //painting
        paintingShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        paintingShader.setMat4("projection", projection);
        paintingShader.setMat4("view", view);
        model = glm::mat4(1.0);
        model = glm::translate(model, programState->roomPosition + glm::vec3(3.3 , 1.8 + programState->deltaY, 0.0 + programState->deltaZ));
        model = glm::scale(model, glm::vec3(0.1,1.1, 1.0));
        paintingShader.setMat4("model", model);
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        screenShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);

    glDeleteVertexArrays(1, &VAO2);
    glDeleteBuffers(1, &VBO2);

    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    //clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        programState->spotLightEnabled = true;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        programState->spotLightEnabled = false;

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
        programState->blurEnabled = true;
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
        programState->blurEnabled = false;

    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->deltaY += 0.01;
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->deltaY -= 0.01;
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->deltaZ -= 0.01;
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->deltaZ += 0.01;

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
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
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
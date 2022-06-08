#include "app.hpp"

#define PARTICLE_NUM 5000
#define PARTICLE_RADIUS 0.005f
#define WORKGROUP_SIZE 128
#define NUM_WORKGROUP ((PARTICLE_NUM + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE)

void processInput(GLFWwindow * window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

app::app(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
}

void app::execute()
{
    initWindow();
    initGLAD();
    initOpenGL();
    // initialize the window && glad && opengl(vao, vbo, shader)
    mainLoop();
}

void app::initWindow()
{
    if(!glfwInit())
    {
        throw std::runtime_error("Failed To Initialize GLFW");
    }

    window = glfwCreateWindow(windowWidth, windowHeight, "sphSim", nullptr, nullptr);
    // create the window
    glfwMakeContextCurrent(window);

    if(!window)
    {
        glfwTerminate();
        throw std::runtime_error("Failed To Create Window");
    }
    // check window is created
}

void app::destroyWindow()
{
    if(window)
    {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void app::initGLAD()
{
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize glad");
    }
}

void app::initOpenGL()
{
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    GLuint vertexShader = compileShader("vertex.vs", GL_VERTEX_SHADER);
    checkShaderProgram(vertexShader, "SHADER");
    // vertex shader compile 
    GLuint fragmentShader = compileShader("fragment.fs", GL_FRAGMENT_SHADER);
    checkShaderProgram(fragmentShader, "SHADER");
    // fragment shader compile
    renderProgram = glCreateProgram();
    glAttachShader(renderProgram, vertexShader);
    glAttachShader(renderProgram, fragmentShader);
    glLinkProgram(renderProgram);
    checkShaderProgram(renderProgram, "PROGRAM");
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    // render program
    
    /*
        1. create shader
        2. compile shader
        3. create program
        4. attach shader to program
        5. link program
        6. check is linked
        7. delete shader
    */


    GLuint computeShader;
    computeShader = compileShader("update_density_pressure.comps", GL_COMPUTE_SHADER);
    computeProgram[0] = glCreateProgram();
    glAttachShader(computeProgram[0], computeShader);
    glLinkProgram(computeProgram[0]);
    checkShaderProgram(computeProgram[0], "PROGRAM");
    glDeleteShader(computeShader);

    computeShader = compileShader("update_force.comps", GL_COMPUTE_SHADER);
    computeProgram[1] = glCreateProgram();
    glAttachShader(computeProgram[1], computeShader);
    glLinkProgram(computeProgram[1]);
    checkShaderProgram(computeProgram[1], "PROGRAM");
    glDeleteShader(computeShader);

    computeShader = compileShader("update_integrate.comps", GL_COMPUTE_SHADER);
    computeProgram[2] = glCreateProgram();
    glAttachShader(computeProgram[2], computeShader);
    glLinkProgram(computeProgram[2]);
    checkShaderProgram(computeProgram[2], "PROGRAM");
    glDeleteShader(computeShader);
    // compute program && shader handle finished


    ptrdiff_t positionSSBOsize = sizeof(glm::vec2) * PARTICLE_NUM;
    ptrdiff_t velocitySSBOsize = sizeof(glm::vec2) * PARTICLE_NUM;
    ptrdiff_t forceSSBOsize = sizeof(glm::vec2) * PARTICLE_NUM;
    ptrdiff_t densitySSBOsize = sizeof(float) * PARTICLE_NUM;
    ptrdiff_t pressureSSBOsize = sizeof(float) * PARTICLE_NUM;

    ptrdiff_t totalSSBOsize = positionSSBOsize +
                                velocitySSBOsize +
                                forceSSBOsize +
                                densitySSBOsize +
                                pressureSSBOsize;
    // ssbo size

    ptrdiff_t positionSSBOoffset = 0;
    ptrdiff_t velocitySSBOoffset = positionSSBOoffset + positionSSBOsize;
    ptrdiff_t forceSSBOoffset = velocitySSBOoffset + velocitySSBOsize;
    ptrdiff_t densitySSBOoffset = forceSSBOoffset + forceSSBOsize;
    ptrdiff_t pressureSSBOoffset = densitySSBOoffset + densitySSBOsize;
    // ssbo offset
    // ssbo handle

    std::vector<glm::vec2> initPositionVec(PARTICLE_NUM);

    float x = 0;
    float y = 0;
    for(int i = 0; i < PARTICLE_NUM; i++)
    {
        initPositionVec[i].x = -1 + PARTICLE_RADIUS * 2 * x;
        initPositionVec[i].y = -1 + PARTICLE_RADIUS * 2 * y;
        x++;
        if(x >= 100)
        {
            x = 0;
            y++;
        }
    }
    // initialize the particle position
    void * initialData = std::malloc(totalSSBOsize);
    std::memset(initialData, 0, totalSSBOsize);
    std::memcpy(initialData, initPositionVec.data(), positionSSBOsize);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, VBO);
    glBufferStorage(GL_SHADER_STORAGE_BUFFER, totalSSBOsize, initialData, GL_DYNAMIC_STORAGE_BIT);
    std::free(initialData);
    // init buffer data vbo   

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // vao

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, VBO, positionSSBOoffset, positionSSBOsize);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, VBO, velocitySSBOoffset, velocitySSBOsize);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, VBO, forceSSBOoffset, forceSSBOsize);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 3, VBO, densitySSBOoffset, densitySSBOsize);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 4, VBO, pressureSSBOoffset, pressureSSBOsize);
    
    
    glBindVertexArray(VAO);
    // ssbo binding
    glClearColor(0.92f, 0.92f, 0.92f, 1.f);
}

void app::destroyOpenGL()
{
    glDeleteProgram(renderProgram);
    glDeleteProgram(computeProgram[0]);
    glDeleteProgram(computeProgram[1]);
    glDeleteProgram(computeProgram[2]);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

GLuint app::compileShader(const char * path, GLenum shaderType)
{
    std::string shaderCode;
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        shaderFile.open(path);
		std::stringstream fileSS;
		fileSS << shaderFile.rdbuf();
		shaderCode = fileSS.str();
		shaderFile.close();
    }catch(std::ifstream::failure & e)
	{
		std::cout << "ERROR::CANNOT OPEN THE FILE\n";
	}
    const char* shaderCodeC = shaderCode.c_str();
    unsigned int shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, &shaderCodeC, NULL);
    glCompileShader(shaderID);
    checkShaderProgram(shaderID, "SHADER");
    return shaderID;
}

void app::checkShaderProgram(GLuint id, const char * type)
{
    int success;
	char infoLog[1024];
	if(type != "PROGRAM")
	{
		glGetShaderiv(id, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(id, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: "
				<< type << "\n" << infoLog << std::endl;
		}
	}
	else
	{
		glGetProgramiv(id, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(id, 1024, NULL, infoLog);
			std::cout << "ERROR::SHADER_LINKING::ERROR of type: "
			<< type << "\n" << infoLog << std::endl;
		}
	}
}

void app::mainLoop()
{
    processInput(window);
    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        sph();
        render();
        glfwSwapBuffers(window);
    }
}

void app::sph()
{
    glUseProgram(computeProgram[0]);
    glDispatchCompute(WORKGROUP_SIZE, 1, 1);

    glUseProgram(computeProgram[1]);
    glDispatchCompute(WORKGROUP_SIZE, 1, 1);

    glUseProgram(computeProgram[2]);
    glDispatchCompute(WORKGROUP_SIZE, 1, 1);
}

void app::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(renderProgram);
    glDrawArrays(GL_POINTS, 0, PARTICLE_NUM);
}
#include <gl/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>


struct ShaderInformation {
    // Переменные с индентификаторами ID
    // ID шейдерной программы
    GLuint shaderProgram;
    // ID атрибута вершин
    GLint attribVertex;
    // ID атрибута текстурных координат
    GLint attribTexture;
    GLint attribNormal;
    // ID юниформа текстуры
    GLint unifTexture;
    // ID юниформа сдвига
    GLint unifShift;
    GLint unifBusMove;
    GLint unifLightPos;
};

struct GameObject {
    // Количество вершин в буферах
    GLfloat buffers_size;
    // ID буфера вершин
    GLuint vertexVBO;
    // ID буфера текстурных координат
    GLuint textureVBO;
    GLuint normalVBO;

    sf::Texture textureData;

    // Велечина сдвига
    GLfloat shift[2];

    GLfloat move[2];
};

std::vector <GameObject> roadObjects;
std::vector <GameObject> grassObjects;
GameObject bus;
ShaderInformation shaderInformation;
// Массив VBO что бы их потом удалить
std::vector <GLuint> VBOArray;

struct vec3
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};
struct vec2
{
    GLfloat x;
    GLfloat y;
};
std::vector<vec3> triangles;
std::vector<vec3> normals;
std::vector<vec2> tex_coordinates;

int numberOfSquares = 3;
float shift_move = 0.05;
float light_pos[3] = { 0.0f, 1.0f, 1.0f };

const char* VertexShaderSource = R"(
    #version 330 core

    uniform vec2 shift;
    uniform vec2 move;

    in vec3 vertCoord;
    in vec3 normCoord;
    in vec2 texureCoord;

    out vec2 tCoord;
    out vec3 normal;
    out vec3 position;

    void main() {
        mat3 rot = mat3(
            cos(move[0]), 0, sin(move[0]),
            0,           1,         0,
            -sin(move[0]), 0, cos(move[0]))
            * mat3(
            cos(0.25 * shift[0]), -sin(0.25 * shift[0]), 0, 
			sin(0.25 * shift[0]), cos(0.25 * shift[0]), 0,         
			0, 0, 1
        );
		
		position = vertCoord * mat3(
          cos(3.14159), 0, -sin(3.14159),
            0, 1, 0,
         -sin(3.14159), 0, cos(3.14159)  
        );
       position =  position * rot 
		* mat3(
				0.05f, 0, 0, 
				0, 0.05f, 0,
				0,  0, 0.05f
			);
		vec4 posmove = vec4(position, 1.0) * mat4(
			1, 0, 0, move[1],
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
			);
        normal = normCoord * rot;
		
        gl_Position = vec4(posmove[0], posmove[1] - 0.5 - shift[0], posmove[2] + shift[1], 1.0 + posmove[2] + shift[1] );
		tCoord = texureCoord;
    }
)";

const char* FragShaderSource = R"(
    #version 330 core

    uniform sampler2D textureData;
    uniform vec3 lightPos;

    in vec2 tCoord;
    in vec3 normal;
    in vec3 position;

    out vec4 color;

    void main() {
        vec3 diffColor = texture(textureData, tCoord).rgb;
		vec3 n = normalize(normal);
		vec3 l = normalize(lightPos);
		vec3 diff = diffColor * max (dot(n,l), 0.0);
		color = vec4(diff, 1.0);
    }
)";


void Init();
void GameTick(int tick);
void Draw(GameObject gameObject);
void Release();

void InitShiftRoad()
{
    roadObjects[0].shift[1] = 0;
    roadObjects[1].shift[1] = 10;
    roadObjects[2].shift[1] = 20;
}
void InitShiftGrass()
{
    grassObjects[0].shift[1] = 0;
    grassObjects[1].shift[1] = 10;
    grassObjects[2].shift[1] = 20;
    grassObjects[3].shift[1] = 30;
    grassObjects[0].shift[0] = 1;
    grassObjects[1].shift[0] = 1;
    grassObjects[2].shift[0] = 1;
    grassObjects[3].shift[0] = 1;
}

int main() {
    sf::Window window(sf::VideoMode(600, 600), "Bus", sf::Style::Default, sf::ContextSettings(24));
    window.setVerticalSyncEnabled(true);

    window.setActive(true);

    glewInit();

    Init();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Счётчик кадров
    int tickCounter = 0;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::Resized) {
                glViewport(0, 0, event.size.width, event.size.height);
            }

            else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case (sf::Keyboard::Left):
                    if (bus.move[1] > -0.5)
                    {
                        bus.move[0] -= 0.1;
                        bus.move[1] -= 0.5;
                    }
                    else
                        bus.move[0] = -0.1;
                    break;
                case (sf::Keyboard::Right):
                    if (bus.move[1] < 0.5)
                    {
                        bus.move[0] += 0.1;
                        bus.move[1] += 0.5;
                    }
                    else
                        bus.move[0] = 0.1;
                    break;
                case (sf::Keyboard::A):
                    light_pos[0] += 0.1;  break;
                case (sf::Keyboard::W):
                    light_pos[1] += 0.1;  break;
                case (sf::Keyboard::D):
                    light_pos[0] -= 0.1;  break;
                case (sf::Keyboard::S):
                    light_pos[1] -= 0.1;  break;
                case (sf::Keyboard::Q):
                    light_pos[2] -= 0.1;  break;
                case (sf::Keyboard::E):
                    light_pos[2] += 0.1;  break;
                default: break;
                }
            }

            else if (event.type == sf::Event::KeyReleased) {
                switch (event.key.code) {
                case (sf::Keyboard::Right):
                    bus.move[0] = 0.0f; break;
                case (sf::Keyboard::Left):
                    bus.move[0] = 0.0f; break;
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GameTick(tickCounter);
        // Отрисовываем все объекты сцены
        for (GameObject& object : grassObjects)
            Draw(object);
        glClear(GL_DEPTH_BUFFER_BIT);
        for (GameObject& object: roadObjects)
            Draw(object);
        //Draw(roadObjects[0]);
        glClear(GL_DEPTH_BUFFER_BIT);
        Draw(bus);

        tickCounter++;
        window.display();
    }

    Release();
    return 0;
}


// Проверка ошибок OpenGL, если есть то вывод в консоль тип ошибки
void checkOpenGLerror() {
    GLenum errCode;
    // Коды ошибок можно смотреть тут
    // https://www.khronos.org/opengl/wiki/OpenGL_Error
    if ((errCode = glGetError()) != GL_NO_ERROR)
        std::cout << "OpenGl error!: " << errCode << std::endl;
}

// Функция печати лога шейдера
void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    int charsWritten = 0;
    char* infoLog;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        infoLog = new char[infologLen];
        if (infoLog == NULL)
        {
            std::cout << "ERROR: Could not allocate InfoLog buffer" << std::endl;
            exit(1);
        }
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog);
        std::cout << "InfoLog: " << infoLog << "\n\n\n";
        delete[] infoLog;
    }
}

// Загрузка obj файла
void load_obj(std::string filename) {
    std::ifstream infile(filename);
    std::string instr;

    triangles.clear();
    normals.clear();
    tex_coordinates.clear();

    std::vector<vec3> vertices;
    std::vector<vec3> norm;
    std::vector<vec2> tex;

    while (infile >> instr) {
        if (instr == "v")
        {
            float x, y, z;
            infile >> x >> y >> z;
            vertices.push_back({ x, y, z });
        }
        else if (instr == "vt")
        {
            float x, y;
            infile >> x >> y;
            tex.push_back({ x, y });
        }
        else if (instr == "vn")
        {
            float x, y, z;
            infile >> x >> y >> z;
            norm.push_back({ x, y, z });
        }
        else if (instr == "f")
        {
            for (size_t i = 0; i < 3; i++)
            {
                int p_ind;
                char c;
                // read vertex index
                infile >> p_ind;
                infile >> c;
                triangles.push_back(vertices[p_ind - 1]);

                // read uv index
                int uv_ind;
                infile >> uv_ind;
                infile >> c;
                tex_coordinates.push_back(tex[uv_ind - 1]);

                // read normal index
                int n_ind;
                infile >> n_ind;
                normals.push_back(norm[n_ind - 1]);
            }
        }
    }
}

void InitObjects()
{
    // road
    GLuint vertexVBORoad;
    GLuint textureVBORoad;
    GLuint normalVBORoad;
    glGenBuffers(1, &vertexVBORoad);
    glGenBuffers(1, &textureVBORoad);
    glGenBuffers(1, &normalVBORoad);
    VBOArray.push_back(vertexVBORoad);
    VBOArray.push_back(textureVBORoad);
    VBOArray.push_back(normalVBORoad);

    load_obj("road.obj");

    glBindBuffer(GL_ARRAY_BUFFER, vertexVBORoad);
    glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(vec3), &triangles[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBORoad);
    glBufferData(GL_ARRAY_BUFFER, tex_coordinates.size() * sizeof(vec2), &tex_coordinates[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBORoad);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);

    sf::Texture texRoad;
    texRoad.loadFromFile("road.png");
    for (int i = 0; i < numberOfSquares; ++i)
        roadObjects.push_back(GameObject{
                (GLfloat)triangles.size() * 3,
                vertexVBORoad,
                textureVBORoad,
                normalVBORoad,
                texRoad, {0, 0}, {0, 0} });
    InitShiftRoad();


    // grass 
    GLuint vertexVBOGrass;
    GLuint textureVBOGrass;
    GLuint normalVBOGrass;
    glGenBuffers(1, &vertexVBOGrass);
    glGenBuffers(1, &textureVBOGrass);
    glGenBuffers(1, &normalVBOGrass);
    VBOArray.push_back(vertexVBOGrass);
    VBOArray.push_back(textureVBOGrass);
    VBOArray.push_back(normalVBOGrass);

    load_obj("grass.obj");

    glBindBuffer(GL_ARRAY_BUFFER, vertexVBOGrass);
    glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(vec3), &triangles[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBOGrass);
    glBufferData(GL_ARRAY_BUFFER, tex_coordinates.size() * sizeof(vec2), &tex_coordinates[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBOGrass);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);

    sf::Texture texGrass;
    texGrass.loadFromFile("grass.png");
    for (int i = 0; i < 4; ++i)
        grassObjects.push_back(GameObject{
                (GLfloat)triangles.size() * 3,
                vertexVBOGrass,
                textureVBOGrass,
                normalVBOGrass,
                texGrass, {0, 0}, {0, 0} });
    InitShiftGrass();


    // bus
    GLuint vertexVBOBus;
    GLuint textureVBOBus;
    GLuint normalVBOBus;
    glGenBuffers(1, &vertexVBOBus);
    glGenBuffers(1, &textureVBOBus);
    glGenBuffers(1, &normalVBOBus);
    VBOArray.push_back(vertexVBOBus);
    VBOArray.push_back(textureVBOBus);
    VBOArray.push_back(normalVBOBus);

    load_obj("bus2.obj");

    glBindBuffer(GL_ARRAY_BUFFER, vertexVBOBus);
    glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(vec3), &triangles[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBOBus);
    glBufferData(GL_ARRAY_BUFFER, tex_coordinates.size() * sizeof(vec2), &tex_coordinates[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBOBus);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), &normals[0], GL_STATIC_DRAW);

    sf::Texture texBus;
    texBus.loadFromFile("bus2.png");
    bus = GameObject{ (GLfloat)triangles.size() * 3,
                vertexVBOBus,
                textureVBOBus,
                normalVBOBus,
                texBus, {0, 0}, {0, 0} };


    checkOpenGLerror();
}


void InitShader() {
    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &VertexShaderSource, NULL);
    glCompileShader(vShader);
    std::cout << "vertex shader \n";
    ShaderLog(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &FragShaderSource, NULL);
    glCompileShader(fShader);
    std::cout << "fragment shader \n";
    ShaderLog(fShader);

    shaderInformation.shaderProgram = glCreateProgram();
    glAttachShader(shaderInformation.shaderProgram, vShader);
    glAttachShader(shaderInformation.shaderProgram, fShader);

    glLinkProgram(shaderInformation.shaderProgram);
    int link_status;
    glGetProgramiv(shaderInformation.shaderProgram, GL_LINK_STATUS, &link_status);
    if (!link_status)
    {
        std::cout << "error attach shaders \n";
        return;
    }

    shaderInformation.attribVertex =
        glGetAttribLocation(shaderInformation.shaderProgram, "vertCoord");
    if (shaderInformation.attribVertex == -1)
    {
        std::cout << "could not bind attrib vertCoord" << std::endl;
        return;
    }

    shaderInformation.attribTexture =
        glGetAttribLocation(shaderInformation.shaderProgram, "texureCoord");
    if (shaderInformation.attribTexture == -1)
    {
        std::cout << "could not bind attrib texureCoord" << std::endl;
        return;
    }

    shaderInformation.attribNormal =
        glGetAttribLocation(shaderInformation.shaderProgram, "normCoord");
    if (shaderInformation.attribNormal == -1)
    {
        std::cout << "could not bind attrib normalCoord" << std::endl;
        return;
    }

    shaderInformation.unifTexture =
        glGetUniformLocation(shaderInformation.shaderProgram, "textureData");
    if (shaderInformation.unifTexture == -1)
    {
        std::cout << "could not bind uniform textureData" << std::endl;
        return;
    }

    shaderInformation.unifShift = glGetUniformLocation(shaderInformation.shaderProgram, "shift");
    if (shaderInformation.unifShift == -1)
    {
        std::cout << "could not bind uniform shift" << std::endl;
        return;
    }

    shaderInformation.unifBusMove = glGetUniformLocation(shaderInformation.shaderProgram, "move");
    if (shaderInformation.unifBusMove == -1)
    {
        std::cout << "could not bind uniform move" << std::endl;
        return;
    }

    shaderInformation.unifLightPos = glGetUniformLocation(shaderInformation.shaderProgram, "lightPos");
    if (shaderInformation.unifLightPos == -1)
    {
        std::cout << "could not bind uniform light" << std::endl;
        return;
    }
    checkOpenGLerror();
}



void Init() {
    InitShader();
    //InitTexture();
    InitObjects();
}

int step = 0;

// Обработка шага игрового цикла
void GameTick(int tick) {
    for (int i = 0; i < 3; i++)
    {
        if (roadObjects[i].shift[1] <= -10)
            InitShiftRoad();
        else
            roadObjects[i].shift[1] -= shift_move;
    }

    for (int i = 0; i < 4; i++)
    {
        if (grassObjects[i].shift[1] <= -10)
            InitShiftGrass();
        else
            grassObjects[i].shift[1] -= shift_move;
    }
}


void Draw(GameObject gameObject) {
    glUseProgram(shaderInformation.shaderProgram);
    glUniform2fv(shaderInformation.unifShift, 1, gameObject.shift);
    glUniform2fv(shaderInformation.unifBusMove, 1, gameObject.move);
    glUniform3fv(shaderInformation.unifLightPos, 1, light_pos);

    glActiveTexture(GL_TEXTURE0);
    sf::Texture::bind(&gameObject.textureData);
    glUniform1i(shaderInformation.unifTexture, 0);

    // Подключаем VBO
    glEnableVertexAttribArray(shaderInformation.attribVertex);
    glBindBuffer(GL_ARRAY_BUFFER, gameObject.vertexVBO);
    glVertexAttribPointer(shaderInformation.attribVertex, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(shaderInformation.attribTexture);
    glBindBuffer(GL_ARRAY_BUFFER, gameObject.textureVBO);
    glVertexAttribPointer(shaderInformation.attribTexture, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(shaderInformation.attribNormal);
    glBindBuffer(GL_ARRAY_BUFFER, gameObject.normalVBO);
    glVertexAttribPointer(shaderInformation.attribNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Передаем данные на видеокарту(рисуем)
    glDrawArrays(GL_TRIANGLES, 0, gameObject.buffers_size);

    // Отключаем массив атрибутов
    glDisableVertexAttribArray(shaderInformation.attribVertex);
    // Отключаем шейдерную программу
    glUseProgram(0);
    checkOpenGLerror();
}


void ReleaseShader() {
    glUseProgram(0);
    glDeleteProgram(shaderInformation.shaderProgram);
}

void ReleaseVBO()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Чистим все выделенные VBO
    for (GLuint VBO: VBOArray)
        glDeleteBuffers(1, &VBO);
}

void Release() {
    ReleaseShader();
    ReleaseVBO();
}

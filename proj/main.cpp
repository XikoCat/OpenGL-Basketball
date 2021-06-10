#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include <vector>
#include <fstream>
#include <sstream>

class Keyframe;
class AnimationObj;
class Obj;
class Cam;

void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void load_all();
void unload_all();

GLuint LoadShaders(const char *vertex_file_path, const char *fragment_file_path);

class Keyframe
{
public:
  int frame;
  glm::vec3 pos;
  glm::vec3 rot;
};

class AnimationObj
{
public:
  bool loop;
  int keys_count = -1;
  std::vector<Keyframe> keys;

  AnimationObj()
  {
  }

  AnimationObj(const char *anm_path)
  {
    load(anm_path);
  }

  glm::vec3 getPos(int frame)
  {
    frame = frame % keys_count;

    int last, next = -1;

    int i;
    for (i = 0; i < keys.size(); i++)
    {
      if (keys[i].frame == frame)
        return keys[i].pos;

      if (keys[i].frame < frame)
      {
        last = i;
      }
      else
      {
        next = i;
        break;
      }
    }
    if (next == -1)
      next = 0;

    int key_diff = next > last ? keys[next].frame - keys[last].frame : abs(keys[next].frame - keys_count + keys[last].frame);
    int c = frame - keys[last].frame;

    glm::vec3 curr;
    curr.x = ((keys[next].pos.x - keys[last].pos.x) / key_diff * c) + keys[last].pos.x;
    curr.y = ((keys[next].pos.y - keys[last].pos.y) / key_diff * c) + keys[last].pos.y;
    curr.z = ((keys[next].pos.z - keys[last].pos.z) / key_diff * c) + keys[last].pos.z;

    return curr;
  }

  glm::vec3 getRot(int frame)
  {
    frame = frame % keys_count;

    int last, next = -1;

    int i;
    for (i = 0; i < keys.size(); i++)
    {
      if (keys[i].frame == frame)
        return keys[i].rot;

      if (keys[i].frame < frame)
      {
        last = i;
      }
      else
      {
        next = i;
        break;
      }
    }
    if (next == -1)
      next = 0;

    int key_diff = next > last ? keys[next].frame - keys[last].frame : abs(keys[next].frame - keys_count + keys[last].frame);
    int c = frame - keys[last].frame;

    glm::vec3 curr;
    curr.x = ((keys[next].rot.x - keys[last].rot.x) / key_diff * c) + keys[last].rot.x;
    curr.y = ((keys[next].rot.y - keys[last].rot.y) / key_diff * c) + keys[last].rot.y;
    curr.z = ((keys[next].rot.z - keys[last].rot.z) / key_diff * c) + keys[last].rot.z;

    return curr;
  }

  void debug()
  {
    std::cout << "AnimationObject - Debug\n loop: " << loop << "\n keys: " << keys_count << std::endl;
    for (int i = 0; i < keys.size(); i++)
      std::cout << " K " << keys[i].frame
                << "\n   " << keys[i].pos.x << ", " << keys[i].pos.y << ", " << keys[i].pos.z
                << "\n   " << keys[i].rot.x << ", " << keys[i].rot.y << ", " << keys[i].rot.z
                << std::endl;
  }

private:
  void load(const char *anm_path)
  {
    printf("Reading animation %s\n", anm_path);

    FILE *fp;

    /* try to open the file */
    fp = fopen(anm_path, "rb");
    if (fp == NULL)
    {
      printf("%s could not be opened. Are you in the right directory ?\n", anm_path);
      getchar();
      return;
    }

    if (fscanf(fp, "%d", &keys_count) == EOF)
    {
      printf("%s invalid format. Expected a INT for the total framecount of animation on line 1\n", anm_path);
      getchar();
      return;
    }

    char lineHeader[5];
    if (fscanf(fp, "%s", &lineHeader) == EOF)
    {
      printf("%s invalid format. Expected a string loop or cont for the type of animation, continuous or looping on line 2\n", anm_path);
      getchar();
      return;
    }
    if (strcmp(lineHeader, "loop") == 0)
      loop = true;
    else if (strcmp(lineHeader, "comp") == 0)
      loop = false;
    else
    {
      printf("%s invalid format. Expected a string loop or cont for the type of animation, continuous or looping on line 2\n", anm_path);
      getchar();
      return;
    }

    bool first = true;
    while (1)
    {
      Keyframe k;

      int res = fscanf(fp, "%d%f%f%f%f%f%f", &k.frame, &k.pos.x, &k.pos.y, &k.pos.z, &k.rot.x, &k.rot.y, &k.rot.z);
      if (res == EOF)
        break;
      if (first)
      {
        first = false;
        if (k.frame != 0)
        {
          printf("%s invalid format. Expected first frame to start at 0\n", anm_path);
          getchar();
          return;
        }
      }
      keys.push_back(k);
    }

    fclose(fp);
  }
};

class Obj
{
public:
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs; //texture maping
  std::vector<glm::vec3> normals;

  glm::vec3 scl = glm::vec3(1);
  glm::vec3 pos = glm::vec3(0);
  glm::vec3 rot = glm::vec3(0);

  GLuint Texture, vertexbuffer, uvbuffer, normalbuffer;

  AnimationObj animation;
  int frame = 0;

  void set(const char *obj_path, const char *tex_path)
  {
    loadOBJ(obj_path, vertices, uvs, normals);

    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

    glGenBuffers(1, &normalbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

    Texture = loadDDS(tex_path);
  }

  void update()
  {
    if (animation.keys_count != -1)
    {
      pos = animation.getPos(frame);
      rot = animation.getRot(frame);
      frame++;
      frame %= animation.keys_count;
    }
  }

  void draw(glm::mat4 mVP, GLuint MatrixID, GLuint TextureID)
  {

    glm::mat4 ModelMatrix = glm::mat4(1.0);
    ModelMatrix = glm::scale(ModelMatrix, scl);
    ModelMatrix = glm::translate(ModelMatrix, pos);

    //rotation in 3 phases to avoid gimbal lock
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(rot.x), glm::vec3(1, 0, 0));
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(rot.y), glm::vec3(0, 1, 0));
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(rot.z), glm::vec3(0, 0, 1));

    glm::mat4 MVP = mVP * ModelMatrix;

    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(TextureID, 0);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
        0,        // attribute
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );

    // 2nd attribute buffer : UVs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glVertexAttribPointer(
        1,        // attribute
        2,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );

    // 3rd attribute buffer : normals
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
    glVertexAttribPointer(
        2,        // attribute
        3,        // size
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0,        // stride
        (void *)0 // array buffer offset
    );

    // Draw the triangles !
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
  }

  void unload()
  {
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteBuffers(1, &normalbuffer);
    glDeleteTextures(1, &Texture);
  }

private:
#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

  /* This function was created by opengl-tutorial.org and can be found in their git repository.
      Was modified to use Anisotropic filtering instead of trilinear filtering with mipmaping
  */
  GLuint loadDDS(const char *imagepath)
  {

    printf("Reading image %s\n", imagepath);

    unsigned char header[124];

    FILE *fp;

    /* try to open the file */
    fp = fopen(imagepath, "rb");
    if (fp == NULL)
    {
      printf("%s could not be opened. Are you in the right directory ? Don't forget to read the FAQ !\n", imagepath);
      getchar();
      return 0;
    }

    /* verify the type of file */
    char filecode[4];
    fread(filecode, 1, 4, fp);
    if (strncmp(filecode, "DDS ", 4) != 0)
    {
      fclose(fp);
      return 0;
    }

    /* get the surface desc */
    fread(&header, 124, 1, fp);

    unsigned int height = *(unsigned int *)&(header[8]);
    unsigned int width = *(unsigned int *)&(header[12]);
    unsigned int linearSize = *(unsigned int *)&(header[16]);
    unsigned int mipMapCount = *(unsigned int *)&(header[24]);
    unsigned int fourCC = *(unsigned int *)&(header[80]);

    unsigned char *buffer;
    unsigned int bufsize;
    /* how big is it going to be including all mipmaps? */
    bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    buffer = (unsigned char *)malloc(bufsize * sizeof(unsigned char));
    fread(buffer, 1, bufsize, fp);
    /* close the file pointer */
    fclose(fp);

    unsigned int components = (fourCC == FOURCC_DXT1) ? 3 : 4;
    unsigned int format;
    switch (fourCC)
    {
    case FOURCC_DXT1:
      format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      break;
    case FOURCC_DXT3:
      format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
      break;
    case FOURCC_DXT5:
      format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      break;
    default:
      free(buffer);
      return 0;
    }

    // Create one OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);

    // "Bind" the newly created texture : all future texture functions will modify this texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
    unsigned int offset = 0;

    /* Trilinear filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_EXT_texture_filter_anisotropic);
*/
    GLfloat value = 1.f, max_anisotropy = 16.0f; /* don't exceed this value...*/
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &value);

    value = (value > max_anisotropy) ? max_anisotropy : value;
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, value);

    /* load the mipmaps */
    for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
    {
      unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
      glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
                             0, size, buffer + offset);

      offset += size;
      width /= 2;
      height /= 2;

      // Deal with Non-Power-Of-Two textures. This code is not included in the webpage to reduce clutter.
      if (width < 1)
        width = 1;
      if (height < 1)
        height = 1;
    }

    free(buffer);

    return textureID;
  }

  /* This function was created by opengl-tutorial.org and can be found in their git repository.
      Was modified to use Anisotropic filtering instead of trilinear filtering with mipmaping
  */
  bool loadOBJ(
      const char *path,
      std::vector<glm::vec3> &out_vertices,
      std::vector<glm::vec2> &out_uvs,
      std::vector<glm::vec3> &out_normals)
  {
    printf("Loading OBJ file %s...\n", path);

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
      printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
      getchar();
      return false;
    }

    while (1)
    {

      char lineHeader[128];
      // read the first word of the line
      int res = fscanf(file, "%s", lineHeader);
      if (res == EOF)
        break; // EOF = End Of File. Quit the loop.

      // else : parse lineHeader

      if (strcmp(lineHeader, "v") == 0)
      {
        glm::vec3 vertex;
        fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
        temp_vertices.push_back(vertex);
      }
      else if (strcmp(lineHeader, "vt") == 0)
      {
        glm::vec2 uv;
        fscanf(file, "%f %f\n", &uv.x, &uv.y);
        uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
        temp_uvs.push_back(uv);
      }
      else if (strcmp(lineHeader, "vn") == 0)
      {
        glm::vec3 normal;
        fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
        temp_normals.push_back(normal);
      }
      else if (strcmp(lineHeader, "f") == 0)
      {
        std::string vertex1, vertex2, vertex3;
        unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
        int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
        if (matches != 9)
        {
          printf("File can't be read by our simple parser :-( Try exporting with other options\n");
          fclose(file);
          return false;
        }
        vertexIndices.push_back(vertexIndex[0]);
        vertexIndices.push_back(vertexIndex[1]);
        vertexIndices.push_back(vertexIndex[2]);
        uvIndices.push_back(uvIndex[0]);
        uvIndices.push_back(uvIndex[1]);
        uvIndices.push_back(uvIndex[2]);
        normalIndices.push_back(normalIndex[0]);
        normalIndices.push_back(normalIndex[1]);
        normalIndices.push_back(normalIndex[2]);
      }
      else
      {
        // Probably a comment, eat up the rest of the line
        char stupidBuffer[1000];
        fgets(stupidBuffer, 1000, file);
      }
    }

    // For each vertex of each triangle
    for (unsigned int i = 0; i < vertexIndices.size(); i++)
    {

      // Get the indices of its attributes
      unsigned int vertexIndex = vertexIndices[i];
      unsigned int uvIndex = uvIndices[i];
      unsigned int normalIndex = normalIndices[i];

      // Get the attributes thanks to the index
      glm::vec3 vertex = temp_vertices[vertexIndex - 1];
      glm::vec2 uv = temp_uvs[uvIndex - 1];
      glm::vec3 normal = temp_normals[normalIndex - 1];

      // Put the attributes in buffers
      out_vertices.push_back(vertex);
      out_uvs.push_back(uv);
      out_normals.push_back(normal);
    }
    fclose(file);
    return true;
  }
};

class Cam
{
public:
  glm::vec3 pos = glm::vec3(0, 4, 0);
  glm::vec3 lookat = glm::vec3(0, 0, 0);
  glm::vec3 up = glm::vec3(0, 1, 0);

  glm::vec3 pos_relative = glm::vec3(0, 4, 0);
  bool followPos = false;
  bool followLook = false;
  int followObj = -1;

  glm::mat4 MVP;

  void update(Obj *objList)
  {
    if (followObj >= 0)
    {
      if (followPos)
        pos = objList[followObj].pos + pos_relative;
      if (followLook)
        lookat = objList[followObj].pos;
    }

    // Projection matrix: [left,right] = [0, 800] [bottom,top] = [0,600]
    // display range: 0.1 unit to 100 units (near and far field)
    glm::mat4 Projection = glm::mat4(1.0f);
    Projection = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    // View camera matrix
    glm::mat4 View = glm::lookAt(pos, lookat, up);

    MVP = Projection * View;
  }
};

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

Cam cam;
int object_count;
Obj *objList;

int main()
{
  // glfw: initialize and configure
  // ----------------------------
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // glfw window creation
  // --------------------
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Basket Animation", NULL, NULL);

  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetKeyCallback(window, key_callback);

  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glClearColor(0.2f, 0.3f, 0.3f, 1.f); // background color, useless because of skybox

  // Enable depth test
  glEnable(GL_DEPTH_TEST);
  // Accept fragment if it closer to the camera than the former one
  glDepthFunc(GL_LESS);

  // Cull triangles which normal is not towards the camera
  glEnable(GL_CULL_FACE);

  // uncomment this call to draw in wireframe polygons.
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  GLuint VertexArrayID;
  glGenVertexArrays(1, &VertexArrayID);
  glBindVertexArray(VertexArrayID);

  GLuint programID = LoadShaders(
      "proj/shaders/TransformVertexShader.vertexshader",
      "proj/shaders/TextureFragmentShader.fragmentshader");

  // Get a handle for our "MVP" uniform
  GLuint MatrixID = glGetUniformLocation(programID, "MVP");

  // Get a handle for our "myTextureSampler" uniform
  GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

  load_all();

  double lastTime = glfwGetTime();
  int nbFrames = 0;

  // render loop
  // -----------
  while (!glfwWindowShouldClose(window))
  {
    // Measure speed
    double currentTime = glfwGetTime();
    nbFrames++;
    if (currentTime - lastTime >= 1.0)
    {
      printf("%d FPS : %f ms/frame\n", nbFrames, 1000.0 / double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0;
    }

    for (int i = 0; i < object_count; i++)
      objList[i].update();

    cam.update(objList);

    // also clear the depth buffer now!
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    for (int i = 0; i < object_count; i++)
      objList[i].draw(cam.MVP, MatrixID, TextureID);

    processInput(window);

    /* glfw: swap buffers and poll IO events (keys pressed/released,
         mouse moved etc.)
         --------------------------------------------------------------*/
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  for (int i = 0; i < object_count; i++)
    objList[i].unload();

  glDeleteProgram(programID);
  glDeleteVertexArrays(1, &VertexArrayID);

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwTerminate();
  return 0;
}

/* process all input: query GLFW whether relevant keys are pressed/released
this frame and react accordingly
-----------------------------------------------------------------------*/
void processInput(GLFWwindow *window)
{
}

/* glfw: whenever the window size changed (by OS or user resize) this
   callback function executes
   -------------------------------------------------------------------*/
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  // make sure the viewport matches the new window dimensions; note that
  // width and height will be significantly larger than specified on
  // retina displays.
  glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

bool debug = false;
void load_all()
{
  //final camera
  cam.pos = glm::vec3(6.f, 1.7f, -1.f);
  cam.lookat = glm::vec3(3.f, 2.f, -7.5f);

  object_count = 10;
  objList = new Obj[object_count];

  int i = 0;
  objList[i++].set("proj/models/campo.obj", "proj/textures/campo.dds");
  objList[i++].set("proj/models/cesto_suporte.obj", "proj/textures/cesto_suporte.dds");
  objList[i++].set("proj/models/cesto_wall.obj", "proj/textures/cesto_wall.dds");
  objList[i++].set("proj/models/hoop.obj", "proj/textures/hoop.dds");
  objList[i++].set("proj/models/ball.obj", "proj/textures/ball.dds");
  objList[i - 1].pos = glm::vec3(5.58f, 1.f, -7.5f);
  objList[i - 1].animation = AnimationObj("proj/animations/ball.anm");

  objList[i++].set("proj/models/painel.obj", "proj/textures/painel.dds");
  objList[i++].set("proj/models/floor.obj", "proj/textures/campo.dds");
  objList[i++].set("proj/models/seats_back.obj", "proj/textures/seats_back.dds");
  objList[i++].set("proj/models/banco.obj", "proj/textures/banco.dds");
  objList[i++].set("proj/models/skybox.obj", "proj/textures/city_box.dds");
  objList[i - 1].pos = cam.pos;
  objList[i - 1].rot = glm::vec3(180, 270, 0);

  if (debug)
  {
    cam.pos_relative = glm::vec3(0.f, 0.f, 1.f);
    cam.up = glm::vec3(0, 1, 0);
    cam.followObj = 4;
    cam.followPos = true;
    cam.followLook = true;

    objList[4].pos = glm::vec3(0.82f, 0.12f, -7.5f);
    objList[4].animation.debug();
  }
}
/* This function was created by opengl-tutorial.org and can be found in their git repository.
      Was modified to use Anisotropic filtering instead of trilinear filtering with mipmaping
  */
void unload_all()
{
  for (int i = 0; i < object_count; i++)
    objList[i].unload();
}

// This function was created by opengl-tutorial.org and can be found in their git repository.
GLuint LoadShaders(const char *vertex_file_path, const char *fragment_file_path)
{

  // Create the shaders
  GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
  GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the Vertex Shader code from the file
  std::string VertexShaderCode;
  std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
  if (VertexShaderStream.is_open())
  {
    std::stringstream sstr;
    sstr << VertexShaderStream.rdbuf();
    VertexShaderCode = sstr.str();
    VertexShaderStream.close();
  }
  else
  {
    printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
    getchar();
    return 0;
  }

  // Read the Fragment Shader code from the file
  std::string FragmentShaderCode;
  std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
  if (FragmentShaderStream.is_open())
  {
    std::stringstream sstr;
    sstr << FragmentShaderStream.rdbuf();
    FragmentShaderCode = sstr.str();
    FragmentShaderStream.close();
  }

  GLint Result = GL_FALSE;
  int InfoLogLength;

  // Compile Vertex Shader
  printf("Compiling shader : %s\n", vertex_file_path);
  char const *VertexSourcePointer = VertexShaderCode.c_str();
  glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
  glCompileShader(VertexShaderID);

  // Check Vertex Shader
  glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0)
  {
    std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    printf("%s\n", &VertexShaderErrorMessage[0]);
  }

  // Compile Fragment Shader
  printf("Compiling shader : %s\n", fragment_file_path);
  char const *FragmentSourcePointer = FragmentShaderCode.c_str();
  glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
  glCompileShader(FragmentShaderID);

  // Check Fragment Shader
  glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
  glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0)
  {
    std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    printf("%s\n", &FragmentShaderErrorMessage[0]);
  }

  // Link the program
  printf("Linking program\n");
  GLuint ProgramID = glCreateProgram();
  glAttachShader(ProgramID, VertexShaderID);
  glAttachShader(ProgramID, FragmentShaderID);
  glLinkProgram(ProgramID);

  // Check the program
  glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
  glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0)
  {
    std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    printf("%s\n", &ProgramErrorMessage[0]);
  }

  glDetachShader(ProgramID, VertexShaderID);
  glDetachShader(ProgramID, FragmentShaderID);

  glDeleteShader(VertexShaderID);
  glDeleteShader(FragmentShaderID);

  return ProgramID;
}

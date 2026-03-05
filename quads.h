#ifndef QUADS_H
#define QUADS_H

typedef unsigned int shader_t;
typedef unsigned int texture_t;

typedef struct {
    float x, y, w, h, rotation;
    shader_t shader;
    texture_t img;
} quad_t;

quad_t Quad(float w_, float h_, shader_t shader_, texture_t img_); // shader_ and/or img_ may be 0
texture_t Texture(const char* path);
shader_t Shader(const char* path);

void set_mainloop(int (*mainloop)(double)); // mainloop :: time -> should_continue_loop
void set_key_callback(void (*key_callback__)(const char* key, int pressed)); // "a", "ArrowUp", "Escape", ...
void set_mouse_move_callback(void (*mouse_move_callback__)(int x, int y)); // (x, y) is mouse position from top-left
void set_mouse_button_callback(void (*mouse_button_callback__)(int left, int pressed));

void draw(quad_t q);

void uniform (quad_t q, const char* name, float value);
void uniform2(quad_t q, const char* name, float vx, float vy);
void uniform3(quad_t q, const char* name, float vx, float vy, float vz);

// ______________________________________________________________________________

// to link : first #define QUADS_IMPLEMENTATION (only once !), then include
// to compile : emcc -o main.js main.c --preload-file res/

/* Minimal fragment shader with texture :

precision mediump float;
uniform sampler2D img;
varying vec2 uv;

void main()
{
    gl_FragColor = texture2D(img, uv);
}

*/

// ______________________________________________________________________________

/* Minimal index.html :

<!DOCTYPE html>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Smol window</title>
<style>html,body{margin:0;height:100%;overflow:hidden}canvas{display:block;width:100%;height:100%}</style>
<canvas id="canvas"></canvas>
<script>var Module={canvas};let r=_=>{canvas.width=innerWidth;canvas.height=innerHeight};addEventListener("resize",r);r()</script>
<script src="main.js"></script>

*/

// ______________________________________________________________________________



















// ____________________________________________________________________________

// ______________________________IMPLEMENTATION________________________________
// ____________________________________________________________________________



















#ifdef QUADS_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>

#include <GLES2/gl2.h>
#include <emscripten/html5.h>

int (*mainloop_)(double);
void (*key_callback_)(const char* key, int pressed);
void (*mouse_move_callback_)(int x, int y);
void (*mouse_button_callback_)(int left, int pressed);

double window_width, window_height;
float aspectRatio;

void update_window_dimensions()
{
    EMSCRIPTEN_RESULT res = emscripten_get_element_css_size("#canvas", &window_width, &window_height);

    if (res == EMSCRIPTEN_RESULT_SUCCESS && window_height != 0) {
        aspectRatio = (float)(window_width / window_height);
    }

    glViewport(0, 0, (int)window_width, (int)window_height);
}


unsigned int compile_shader(unsigned int type, const char *source)
{
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);

    return id;
}

int create_program(const char *vertex_shader, const char *fragment_shader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glBindAttribLocation(program, 0, "local");
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

char *read_shader(const char *filename)
{
    FILE *shader_file = fopen(filename, "r");

    fseek(shader_file, 0L, SEEK_END);
    size_t sz = ftell(shader_file);
    rewind(shader_file);

    char *res = malloc((1 + sz) * sizeof(char));
    fread(res, 1, sz, shader_file);
    res[sz] = '\0';

    fclose(shader_file);

    return res;
}

EM_BOOL EM_MAINLOOP(double time, void* data)
{
    update_window_dimensions();
    return mainloop_(time);
}

EM_BOOL EM_KEY_CALLBACK(int eventType, const EmscriptenKeyboardEvent *e, void *userData)
{
    if(eventType == EMSCRIPTEN_EVENT_KEYDOWN)
    {
        key_callback_(e->key, true);
    }
    else if(eventType == EMSCRIPTEN_EVENT_KEYUP)
    {
        key_callback_(e->key, false);
    }
    return EM_TRUE;
}

EM_BOOL EM_MOUSE_MOVE_CALLBACK(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
    mouse_move_callback_(e->canvasX, e->canvasY);
    return EM_TRUE;
}

EM_BOOL EM_MOUSE_BUTTON_CALLBACK(int eventType, const EmscriptenMouseEvent *e, void *userData)
{
    if(eventType == EMSCRIPTEN_EVENT_MOUSEDOWN && (e->button == 0 || e->button == 2))
    {
        mouse_button_callback_(e->button == 0, true);
    }
    else if(eventType == EMSCRIPTEN_EVENT_MOUSEUP && (e->button == 0 || e->button == 2))
    {
        mouse_button_callback_(e->button == 0, false);
    }
    return EM_TRUE;
}

void set_mainloop(int (*mainloop)(double))
{
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    mainloop_ = mainloop;
    emscripten_request_animation_frame_loop(EM_MAINLOOP, NULL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float vertices[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    unsigned int triang_buf;
    glGenBuffers(1, &triang_buf);
    glBindBuffer(GL_ARRAY_BUFFER, triang_buf);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    unsigned short indices[] = { 0, 1, 2, 2, 1, 3 };
    unsigned int ind_buf;
    glGenBuffers(1, &ind_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ind_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned short), indices, GL_STATIC_DRAW);

    update_window_dimensions();
}


void set_key_callback(void (*key_callback__)(const char* key, int pressed))
{
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, EM_KEY_CALLBACK);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, EM_KEY_CALLBACK);
    key_callback_ = key_callback__;
}

void set_mouse_move_callback(void (*mouse_move_callback__)(int x, int y))
{
    emscripten_set_mousemove_callback("#canvas", NULL, EM_TRUE, EM_MOUSE_MOVE_CALLBACK);
    mouse_move_callback_ = mouse_move_callback__;
}

void set_mouse_button_callback(void (*mouse_button_callback__)(int left, int pressed))
{
    emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, EM_MOUSE_BUTTON_CALLBACK);
    emscripten_set_mouseup_callback("#canvas", NULL, EM_TRUE, EM_MOUSE_BUTTON_CALLBACK);
    mouse_button_callback_ = mouse_button_callback__;
}


// written in one of my previous projects 2 years ago
// I adapted it so it can also read grey-scale images (.pgm format)
unsigned char *read_ppm(int is_pgm, const char *filename_ppm, int *width, int *height)
{
    FILE *fichier = fopen(filename_ppm, "r");
    if (fichier == NULL)
    {
        printf("Can't find %s\n", filename_ppm);
        return NULL;
    }
    if(fgetc(fichier) != 'P' || fgetc(fichier) != (is_pgm ? '5' : '6'))
    {
        return NULL;
    }
    fgetc(fichier);
    if(fgetc(fichier) == '#')
    {
        while(fgetc(fichier) != '\n');
    }
    else
    {
        fseek(fichier, -1, SEEK_CUR);
    }

    int max_color = 255;
    if (fscanf(fichier, "%d %d %d", width, height, &max_color) != 3)
    {
        printf("Probleme de lecture du header ppm...");
        fclose(fichier);
        return NULL;
    }
    if (max_color != 255)
    {
        printf("Couleur ref doit etre 255, non ?\n");
        fclose(fichier);
        return NULL;
    }

    // Tout le fichier est lu d'un seul coup avec un fread approprié

    int nb_px = (*width) * (*height);
    unsigned char *res = (unsigned char*)malloc((is_pgm ? 1 : 3) * nb_px * sizeof(unsigned char));
    fread(res, sizeof(unsigned char), (is_pgm ? 1 : 3) * nb_px, fichier);

    fclose(fichier);
    return res;
}

#define VERTEX_SHADER_SOURCE \
        ("precision mediump float;\
         attribute vec2 local;\
         uniform vec2 pos;\
         uniform vec2 size;\
         uniform float rotation;\
         varying vec2 uv;\
         mat2 rot2D(float a){\
             float s=sin(a), c=cos(a);\
             return mat2(c, -s, s, c);\
         }\
         void main() {\
            gl_Position = vec4((2.0 * local - 1.0) * rot2D(rotation) * size + pos, 0.0, 1.0);\
            uv = 2.0 * local - vec2(1.0);\
         }")

shader_t default_shader()
{
    return create_program(
        VERTEX_SHADER_SOURCE,
        
        "precision mediump float;\
         varying vec2 uv;\
         void main(){\
            gl_FragColor=vec4(uv, 1.0, 1.0);\
         }"
    );
}

shader_t sprite_shader()
{
    return create_program(
        VERTEX_SHADER_SOURCE,
        
        "precision mediump float;\
         uniform sampler2D img;\
         varying vec2 uv;\
         void main(){\
            gl_FragColor=texture2D(img, uv);\
         }"
    );
}

shader_t Shader(const char* path)
{
    if(!path) return sprite_shader();

    char* fs_source = read_shader(path);
    shader_t sh = create_program(VERTEX_SHADER_SOURCE, fs_source);
    free(fs_source);

    return sh;
}

texture_t Texture(const char* path)
{
    if(!path || !*path || !path[1]) return 0;

    int width, height;
    int g = 0; for(; path[g]; g++);
    int is_pgm = path[g - 2] == 'g';
    unsigned char *data = read_ppm(is_pgm, path, &width, &height);

    // Chaque texture chargée est associée à un uint, qu'on stocke dans un
    // unsigned int[], ici comme il y en a 1 seul on envoie &texture
    unsigned int texture_id;
    // Génération du texture object
    glGenTextures(1, &texture_id);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    if(!is_pgm)
    {
        // On attache une image 2d à un texture object
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    free(data);

    return texture_id;
}

quad_t Quad(float w_, float h_, shader_t shader_, texture_t img_)
{
    quad_t res;
    res.x = 0;
    res.y = 0;
    res.w = w_;
    res.h = h_;
    res.rotation = 0.0f;
    res.shader = shader_ ? shader_ : (img_ ? sprite_shader() : default_shader());
    res.img = img_;

    return res;
}

void uniform(quad_t q, const char* name, float value)
{
    glUseProgram(q.shader);
    glUniform1f(glGetUniformLocation(q.shader, name), value);
}

void uniform2(quad_t q, const char* name, float vx, float vy)
{
    glUseProgram(q.shader);
    glUniform2f(glGetUniformLocation(q.shader, name), vx, vy);
}

void uniform3(quad_t q, const char* name, float vx, float vy, float vz)
{
    glUseProgram(q.shader);
    glUniform3f(glGetUniformLocation(q.shader, name), vx, vy, vz);
}

void draw(quad_t q)
{
    glUseProgram(q.shader);
    glUniform2f(glGetUniformLocation(q.shader, "size"), q.w / aspectRatio, q.h);
    glUniform2f(glGetUniformLocation(q.shader, "pos"), q.x / window_width, q.y / window_height);
    glUniform1f(glGetUniformLocation(q.shader, "rotation"), q.rotation);
    if(q.img != 0)
    {
        glBindTexture(GL_TEXTURE_2D, q.img);
        glUniform1i(glGetUniformLocation(q.shader, "img"), 0);
    }
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);
}

#endif
#endif
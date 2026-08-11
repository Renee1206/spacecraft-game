#define _USE_MATH_DEFINES
#include <windows.h>
#include <GL/glut.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <string>

#include "tutorial4.h"
#include "texture.h"
#include "3dsloader.h"

// Constants
constexpr int TEXT_MAX_CHAR = 128;
constexpr int NUM_OBSTACLES = 80;
constexpr float MIN_DISTANCE = 100.0f;
constexpr float OBSTACLE_SPEED = 1.3f;
constexpr float OBSTACLE_RESPAWN_Z = -1000.0f;
constexpr float OBSTACLE_DELETE_Z = 1000.0f;
constexpr float BULLET_SPEED = 5.0f;
constexpr float BULLET_LIFETIME_Z = -1000.0f;
constexpr float BULLET_FIRE_COOLDOWN = 200.0f;
constexpr float SPACESHIP_RENDER_SCALE = 5.0f;
constexpr float SPACESHIP_OFFSET_Y = -45.0f;
constexpr float SPACESHIP_OFFSET_Z = -50.0f;
constexpr float BULLET_COLLISION_RADIUS = 2.0f;
constexpr int WIN_SCORE_THRESHOLD = 50;

// Global variables
int screen_width = 640;
int screen_height = 480;
GLfloat t = 0.0f;
double posx = 0, posy = 0, posz = 0;
int filling = 1;
int lastFireTime = 0;
int countdown = 40;
int previousTime = 0;
int hearts = 8;
int score = 0;
bool gameOver = false;
bool gameWin = false;
float SPACESHIP_COLLISION_RADIUS = 0.0f;

obj_type object1, object_rock1, object_rock2, object_fighter1;
obj_type object_new_obstacle_A, object_new_obstacle_B;
GLuint skyboxTextures[6];

GLuint g_font_display_list_base = 0;
bool g_font_initialized = false;
int g_current_font_size = 0;
char g_current_font_face[LF_FACESIZE] = { 0 };

// Define ObstacleType
enum class ObstacleType {
    ROCK1,
    ROCK2,
    FIGHTER,
    NEW_A,
    NEW_B
};

// Define ExplosionType
enum class ExplosionType {
    COLLISION,
    BULLET_HIT
};

// Define Obstacle structure
struct Obstacle {
    float x, y, z;
    float scale;
    float rotation_angle_y;
    ObstacleType type;
    bool active;
    float collision_radius;
};

// Define Bullet structure
struct Bullet {
    float x, y, z;
    float velocity_z;
    bool active;
    float collision_radius;
};

// Define Particle structure
struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float life;
    float r, g, b, a;
    float max_life;
    float size;
    bool is_thruster;
    bool has_gravity;
    bool is_quad;
};

// Global vectors
std::vector<Obstacle> obstacles;
std::vector<Bullet> bullets;
std::vector<Particle> particles;

// Function declarations
void calculateObjectRadius(obj_type* obj, float scale, float& radius_out, float adjustment_factor = 1.0f);
void selectFont(int size, int charset, const char* face);
void drawString(float x, float y, const char* str, float r, float g, float b);
float getStringWidth(const char* str, int font_size);
void drawHeart(float x, float y, float size, bool filled);
void initSkybox();
void drawSkybox(float size);
void drawObject(obj_type* obj);
void drawBullet(float x, float y, float z);
void drawParticles();
void generateExplosion(float x, float y, float z, ExplosionType type, ObstacleType obs_type);
void generateThrusterParticles(float x, float y, float z, int count, float spread, float initial_speed);
void init();
void resize(int width, int height);
bool checkSphereCollision(float x1, float y1, float z1, float radius1, float x2, float y2, float z2, float radius2);
void tryRespawnObstacle(Obstacle& obs);
void display();
void idle();
void keyboard(unsigned char key, int x, int y);
void keyboard_s(int key, int x, int y);

// Function Definitions
void calculateObjectRadius(obj_type* obj, float scale, float& radius_out, float adjustment_factor) {
    if (obj->vertices_qty == 0) {
        radius_out = 0.0f;
        return;
    }

    float max_dist_sq = 0.0f;
    for (int i = 0; i < obj->vertices_qty; ++i) {
        float vx = obj->vertex[i].x * scale;
        float vy = obj->vertex[i].y * scale;
        float vz = obj->vertex[i].z * scale;
        float dist_sq = vx * vx + vy * vy + vz * vz;
        if (dist_sq > max_dist_sq) {
            max_dist_sq = dist_sq;
        }
    }

    radius_out = sqrt(max_dist_sq) * adjustment_factor;
}

void selectFont(int size, int charset, const char* face) {
    if (g_font_initialized && g_current_font_size == size && strcmp(g_current_font_face, face) == 0) {
        return;
    }

    if (g_font_display_list_base != 0) {
        glDeleteLists(g_font_display_list_base, TEXT_MAX_CHAR);
    }
    g_font_display_list_base = glGenLists(TEXT_MAX_CHAR);
    g_font_initialized = true;
    g_current_font_size = size;
    strncpy_s(g_current_font_face, sizeof(g_current_font_face), face, _TRUNCATE);
    g_current_font_face[LF_FACESIZE - 1] = '\0';

    HFONT hFont = CreateFontA(size, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
        charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, face);
    HDC hdc = wglGetCurrentDC();
    if (!hdc) {
        MessageBox(NULL, "Failed to get current device context for font.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    wglUseFontBitmaps(hdc, 0, TEXT_MAX_CHAR, g_font_display_list_base);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void drawString(float x, float y, const char* str, float r, float g, float b) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screen_width, 0, screen_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Glow effect
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    for (float offset = 1.0f; offset <= 3.0f; offset += 1.0f) {
        glRasterPos2f(x + offset, y + offset);
        glListBase(g_font_display_list_base);
        glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
        glRasterPos2f(x - offset, y - offset);
        glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
        glRasterPos2f(x + offset, y - offset);
        glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
        glRasterPos2f(x - offset, y + offset);
        glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
    }

    // Shadow
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glRasterPos2f(x + 2.0f, y - 2.0f);
    glListBase(g_font_display_list_base);
    glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);

    // Main text
    glColor4f(r, g, b, 1.0f);
    glRasterPos2f(x, y);
    glListBase(g_font_display_list_base);
    glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);

    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

float getStringWidth(const char* str, int font_size) {
    return static_cast<float>(strlen(str)) * (font_size * 0.5f);
}

void drawHeart(float x, float y, float size, bool filled) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, screen_width, 0, screen_height);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    glTranslatef(x, y, 0.0f);
    glScalef(size, size, 1.0f);

    if (filled) {
        // Glow effect
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.2f, 0.2f, 0.1f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(0.0f, 0.0f);
        for (float angle = 0; angle <= 2 * M_PI; angle += 0.005f) {
            float t = sin(angle);
            float heart_x = 16 * t * t * t;
            float heart_y = 13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle);
            glVertex2f(heart_x / 18.0f * 1.2f, heart_y / 18.0f * 1.2f);
        }
        glEnd();

        // Main filled heart
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.3f, 0.3f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        for (float angle = 0; angle <= 2 * M_PI; angle += 0.005f) {
            float t = sin(angle);
            float heart_x = 16 * t * t * t;
            float heart_y = 13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle);
            glColor4f(0.8f, 0.1f, 0.1f, 1.0f);
            glVertex2f(heart_x / 18.0f, heart_y / 18.0f);
        }
        glEnd();
    }
    else {
        // Unfilled heart outline
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glLineWidth(2.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.5f, 0.0f, 0.0f, 0.8f);
        glBegin(GL_LINE_LOOP);
        for (float angle = 0; angle <= 2 * M_PI; angle += 0.005f) {
            float t = sin(angle);
            float heart_x = 16 * t * t * t;
            float heart_y = 13 * cos(angle) - 5 * cos(2 * angle) - 2 * cos(3 * angle) - cos(4 * angle);
            glVertex2f(heart_x / 18.0f, heart_y / 18.0f);
        }
        glEnd();
        glDisable(GL_LINE_SMOOTH);
        glLineWidth(1.0f);
    }

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void initSkybox() {
    const char* skyboxFiles[6] = { "1.bmp", "3.bmp", "5.bmp", "6.bmp", "4.bmp", "2.bmp" };
    for (int i = 0; i < 6; i++) {
        skyboxTextures[i] = LoadBitmap(const_cast<char*>(skyboxFiles[i]));
        if (skyboxTextures[i] == static_cast<GLuint>(-1)) {
            char errorMsg[100];
            sprintf_s(errorMsg, sizeof(errorMsg), "Image file: %s not found", skyboxFiles[i]);
            MessageBox(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
            exit(0);
        }
    }
}

void drawSkybox(float size) {
    glDisable(GL_DEPTH_TEST);
    glPushMatrix();
    glScalef(size, size, size);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    const int faces[6] = { 4, 5, 1, 0, 2, 3 };
    const GLfloat vertices[6][4][3] = {
        {{1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1}},
        {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}},
        {{-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}},
        {{1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}},
        {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}},
        {{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}
    };
    for (int i = 0; i < 6; ++i) {
        glBindTexture(GL_TEXTURE_2D, skyboxTextures[faces[i]]);
        glBegin(GL_QUADS);
        for (int j = 0; j < 4; ++j) {
            glTexCoord2f(j == 0 || j == 3 ? 0.0f : 1.0f, j < 2 ? 0.0f : 1.0f);
            glVertex3fv(vertices[i][j]);
        }
        glEnd();
    }
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

void drawObject(obj_type* obj) {
    if (obj->id_texture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, obj->id_texture);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
    }
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < obj->polygons_qty; i++) {
        glTexCoord2f(obj->mapcoord[obj->polygon[i].a].u, obj->mapcoord[obj->polygon[i].a].v);
        glVertex3fv(&obj->vertex[obj->polygon[i].a].x);
        glTexCoord2f(obj->mapcoord[obj->polygon[i].b].u, obj->mapcoord[obj->polygon[i].b].v);
        glVertex3fv(&obj->vertex[obj->polygon[i].b].x);
        glTexCoord2f(obj->mapcoord[obj->polygon[i].c].u, obj->mapcoord[obj->polygon[i].c].v);
        glVertex3fv(&obj->vertex[obj->polygon[i].c].x);
    }
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void drawBullet(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(BULLET_COLLISION_RADIUS, BULLET_COLLISION_RADIUS * 3.0f, 10, 10);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void drawParticles() {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glDisable(GL_TEXTURE_2D);

    // Collision explosion particles
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (const auto& p : particles) {
        if (!p.is_thruster && !p.is_quad) {
            glPointSize(p.size);
            glBegin(GL_POINTS);
            glColor4f(p.r, p.g, p.b, p.a * p.life);
            glVertex3f(p.x, p.y, p.z);
            glEnd();
        }
    }

    // Thruster and bullet hit particles
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLfloat modelview[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    float right[] = { modelview[0], modelview[4], modelview[8] };
    float up[] = { modelview[1], modelview[5], modelview[9] };

    glBegin(GL_QUADS);
    for (const auto& p : particles) {
        if (p.is_thruster || p.is_quad) {
            float life_ratio = p.life / (p.max_life > 0.0f ? p.max_life : 1.0f);
            float r, g, b, a, size;
            if (p.is_thruster) {
                r = p.r * (0.6f + 0.4f * life_ratio);
                g = p.g * (0.3f + 0.3f * life_ratio);
                b = p.b * 0.1f;
                a = p.a * life_ratio * life_ratio;
                size = p.size * (1.0f - (1.0f - life_ratio) * (1.0f - life_ratio));
                if (size < 0.1f) size = 0.1f;
            }
            else {
                r = p.r;
                g = p.g;
                b = p.b;
                a = p.a * life_ratio;
                size = p.size * life_ratio;
                if (size < 0.2f) size = 0.2f;
            }

            float half_size = size * 0.5f;
            float v0[3] = { p.x - right[0] * half_size - up[0] * half_size,
                           p.y - right[1] * half_size - up[1] * half_size,
                           p.z - right[2] * half_size - up[2] * half_size };
            float v1[3] = { p.x + right[0] * half_size - up[0] * half_size,
                           p.y + right[1] * half_size - up[1] * half_size,
                           p.z + right[2] * half_size - up[2] * half_size };
            float v2[3] = { p.x + right[0] * half_size + up[0] * half_size,
                           p.y + right[1] * half_size + up[1] * half_size,
                           p.z + right[2] * half_size + up[2] * half_size };
            float v3[3] = { p.x - right[0] * half_size + up[0] * half_size,
                           p.y - right[1] * half_size + up[1] * half_size,
                           p.z - right[2] * half_size + up[2] * half_size };

            glColor4f(r, g, b, a);
            glTexCoord2f(0.0f, 0.0f); glVertex3fv(v0);
            glTexCoord2f(1.0f, 0.0f); glVertex3fv(v1);
            glTexCoord2f(1.0f, 1.0f); glVertex3fv(v2);
            glTexCoord2f(0.0f, 1.0f); glVertex3fv(v3);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void generateExplosion(float x, float y, float z, ExplosionType type, ObstacleType obs_type) {
    int particle_count = 0;
    float spread = 0.0f, initial_speed = 0.0f, r = 0.0f, g = 0.0f, b = 0.0f;
    float min_size = 0.0f, max_size = 0.0f, lifetime = 0.0f;
    bool has_gravity = false;
    bool is_quad = true;

    if (type == ExplosionType::COLLISION) {
        particle_count = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 100 : 80;
        spread = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 25.0f : 20.0f;
        initial_speed = 0.6f;
        r = 1.0f; g = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 0.3f : 0.5f; b = 0.0f;
        min_size = 3.0f; max_size = 8.0f;
        lifetime = 1.5f;
        has_gravity = true;
        is_quad = false;
    }
    else {
        particle_count = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 60 : 40;
        spread = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 10.0f : 6.0f;
        initial_speed = 1.5f;
        r = 1.0f; g = 1.0f; b = (obs_type == ObstacleType::FIGHTER || obs_type == ObstacleType::NEW_A) ? 0.6f : 0.3f;
        min_size = 2.0f; max_size = 4.0f;
        lifetime = 0.8f;
        has_gravity = false;
        is_quad = true;
    }

    for (int i = 0; i < particle_count; ++i) {
        Particle p;
        p.x = x + (float)(rand() % (int)(spread * 2) - spread);
        p.y = y + (float)(rand() % (int)(spread * 2) - spread);
        p.z = z + (float)(rand() % (int)(spread * 2) - spread);
        float angle = (float)(rand() % 360) * static_cast<float>(M_PI) / 180.0f;
        float speed = initial_speed * (0.8f + (float)(rand() % 41) / 100.0f);
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed * (type == ExplosionType::COLLISION ? 0.5f : 1.0f);
        p.vz = (float)(rand() % 201 - 100) / 100.0f * speed * 0.5f;

        p.life = lifetime * (0.8f + (float)(rand() % 41) / 100.0f);
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 1.0f;
        p.max_life = p.life;
        p.size = min_size + (float)(rand() % (int)((max_size - min_size) * 100)) / 100.0f;
        p.is_thruster = false;
        p.has_gravity = has_gravity;
        p.is_quad = is_quad;

        particles.push_back(p);
    }
}

void generateThrusterParticles(float x, float y, float z, int count, float spread, float initial_speed) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x + (float)(rand() % (int)(spread * 2) - spread) * 0.5f;
        p.y = y + (float)(rand() % (int)(spread * 2) - spread) * 0.5f;
        p.z = z + (float)(rand() % (int)(spread * 2) - spread) * 0.5f;

        p.vx = (float)(rand() % 101 - 50) / 100.0f * initial_speed * 0.2f;
        p.vy = (float)(rand() % 101 - 50) / 100.0f * initial_speed * 0.2f - 0.5f;
        p.vz = -(initial_speed + (float)(rand() % 101) / 100.0f * initial_speed * 0.5f);

        p.life = 0.5f + (float)(rand() % 51) / 100.0f * 0.5f;
        p.max_life = p.life;
        p.r = 1.0f;
        p.g = 0.9f;
        p.b = 0.5f;
        p.a = 1.0f;
        p.size = 3.0f + (float)(rand() % 21) / 10.0f;
        p.is_thruster = true;
        p.has_gravity = false;
        p.is_quad = true;

        particles.push_back(p);
    }
}

void tryRespawnObstacle(Obstacle& obs) {
    int max_attempts = 10;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        obs.x = static_cast<float>(rand() % 1600 - 800);
        obs.y = static_cast<float>(rand() % 800 - 400);
        obs.z = OBSTACLE_RESPAWN_Z + (rand() % 200 - 100);
        obs.rotation_angle_y = static_cast<float>(rand() % 360);
        int type_choice = rand() % 5;
        if (type_choice == 0) {
            obs.type = ObstacleType::ROCK1;
            obs.scale = 8.0f + static_cast<float>(rand() % 131) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else if (type_choice == 1) {
            obs.type = ObstacleType::ROCK2;
            obs.scale = 18.0f + static_cast<float>(rand() % 131) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else if (type_choice == 2) {
            obs.type = ObstacleType::FIGHTER;
            obs.scale = 2.0f + static_cast<float>(rand() % 101) / 100.0f;
            obs.collision_radius = 60.0f;
        }
        else if (type_choice == 3) {
            obs.type = ObstacleType::NEW_A;
            obs.scale = 5.0f + static_cast<float>(rand() % 51) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else {
            obs.type = ObstacleType::NEW_B;
            obs.scale = 25.0f;
            obs.collision_radius = 120.0f;
        }

        bool overlap = false;
        for (const auto& other_obs : obstacles) {
            if (&obs == &other_obs || !other_obs.active) continue;
            float dx = obs.x - other_obs.x;
            float dy = obs.y - other_obs.y;
            float dz = obs.z - other_obs.z;
            float dist = sqrt(dx * dx + dy * dy + dz * dz);
            if (dist < (obs.collision_radius + other_obs.collision_radius + MIN_DISTANCE)) {
                overlap = true;
                break;
            }
        }

        if (!overlap) {
            obs.active = true;
            return;
        }
    }
    obs.active = false;
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glShadeModel(GL_SMOOTH);
    glViewport(0, 0, screen_width, screen_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)screen_width / screen_height, 10.0f, 20000.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat light_position[] = { 0.0f, 100.0f, 0.0f, 1.0f };
    GLfloat ambient_light[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuse_light[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
    glEnable(GL_COLOR_MATERIAL);

    initSkybox();

    Load3DS(&object1, "fighter1.3ds");
    if (object1.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: fighter1.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object1.id_texture = LoadBitmap("skull.bmp");

    Load3DS(&object_rock1, "Rock1.3ds");
    if (object_rock1.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: Rock1.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object_rock1.id_texture = LoadBitmap("Rock-Texture-Surface.bmp");

    Load3DS(&object_rock2, "Rock.3ds");
    if (object_rock2.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: Rock.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object_rock2.id_texture = LoadBitmap("photo-stone-texture-pattern.bmp");

    Load3DS(&object_fighter1, "UFO.3ds");
    if (object_fighter1.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: UFO.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object_fighter1.id_texture = LoadBitmap("UFONormal.bmp");
    if (object_fighter1.id_texture == static_cast<GLuint>(-1)) {
        MessageBox(NULL, "Image file: UFONormal.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    Load3DS(&object_new_obstacle_A, "Uploads_files_5347744_Cartoon+Alien_v1_001.3ds");
    if (object_new_obstacle_A.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: Uploads_files_5347744_Cartoon+Alien_v1_001.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object_new_obstacle_A.id_texture = LoadBitmap("Cartoon Alien_v1_001_Diffuse.bmp");
    if (object_new_obstacle_A.id_texture == static_cast<GLuint>(-1)) {
        MessageBox(NULL, "Image file: Cartoon Alien_v1_001_Diffuse.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    Load3DS(&object_new_obstacle_B, "Uploads_files_4253924_Style+Sun_v1_001.3ds");
    if (object_new_obstacle_B.vertices_qty == 0) {
        MessageBox(NULL, "Failed to load 3DS file: Uploads_files_4253924_Style+Sun_v1_001.3ds", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }
    object_new_obstacle_B.id_texture = LoadBitmap("Style Sun_v1_001_Diffuse.bmp");
    if (object_new_obstacle_B.id_texture == static_cast<GLuint>(-1)) {
        MessageBox(NULL, "Image file: Style Sun_v1_001_Diffuse.bmp not found", "Error", MB_OK | MB_ICONERROR);
        exit(0);
    }

    calculateObjectRadius(&object1, SPACESHIP_RENDER_SCALE, SPACESHIP_COLLISION_RADIUS, 0.7f);

    srand(static_cast<unsigned int>(time(nullptr)));

    int generated = 0;
    while (generated < NUM_OBSTACLES) {
        Obstacle obs;
        float new_x = static_cast<float>(rand() % 1600 - 800);
        float new_y = static_cast<float>(rand() % 800 - 400);
        float new_z = static_cast<float>(rand() % static_cast<int>(fabs(OBSTACLE_RESPAWN_Z)) - static_cast<int>(fabs(OBSTACLE_RESPAWN_Z)) - 200);
        obs.rotation_angle_y = static_cast<float>(rand() % 360);
        int type_choice = rand() % 5;
        if (type_choice == 0) {
            obs.type = ObstacleType::ROCK1;
            obs.scale = 8.0f + static_cast<float>(rand() % 131) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else if (type_choice == 1) {
            obs.type = ObstacleType::ROCK2;
            obs.scale = 18.0f + static_cast<float>(rand() % 131) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else if (type_choice == 2) {
            obs.type = ObstacleType::FIGHTER;
            obs.scale = 2.0f + static_cast<float>(rand() % 101) / 100.0f;
            obs.collision_radius = 60.0f;
        }
        else if (type_choice == 3) {
            obs.type = ObstacleType::NEW_A;
            obs.scale = 5.0f + static_cast<float>(rand() % 51) / 10.0f;
            obs.collision_radius = 23.0f;
        }
        else {
            obs.type = ObstacleType::NEW_B;
            obs.scale = 25.0f;
            obs.collision_radius = 120.0f;
        }

        obs.active = true;
        bool tooClose = false;
        for (const auto& existing : obstacles) {
            float dx = new_x - existing.x;
            float dy = new_y - existing.y;
            float dz = new_z - existing.z;
            float dist = sqrt(dx * dx + dy * dy + dz * dz);
            if (dist < (obs.collision_radius + existing.collision_radius + MIN_DISTANCE)) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            obs.x = new_x;
            obs.y = new_y;
            obs.z = new_z;
            obstacles.push_back(obs);
            generated++;
        }
    }
    previousTime = glutGet(GLUT_ELAPSED_TIME);
    selectFont(36, ANSI_CHARSET, "Comic Sans MS");
}

void resize(int width, int height) {
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, screen_width, screen_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, static_cast<float>(screen_width) / screen_height, 10.0f, 20000.0f);
}

bool checkSphereCollision(float x1, float y1, float z1, float radius1,
    float x2, float y2, float z2, float radius2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float dz = z1 - z2;
    float dist_sq = dx * dx + dy * dy + dz * dz;
    float radii_sum = radius1 + radius2;
    float COLLISION_RADIUS = 0.1f;
    return dist_sq < (radii_sum * radii_sum + COLLISION_RADIUS);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posx, posy + 100, posz + 200,
        posx, posy, posz,
        0.0, 1.0, 0.0);
    glPushMatrix();
    glTranslatef(posx, posy, posz);
    drawSkybox(800.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(posx, posy, posz);
    glTranslatef(0, SPACESHIP_OFFSET_Y, SPACESHIP_OFFSET_Z);
    glScalef(SPACESHIP_RENDER_SCALE, SPACESHIP_RENDER_SCALE, SPACESHIP_RENDER_SCALE);
    glRotatef(180, 0, 1, 0);
    glRotatef(180, 1, 0, 0);
    glRotatef(180, 0, 0, 1);
    drawObject(&object1);
    glPopMatrix();

    generateThrusterParticles(posx, posy + SPACESHIP_OFFSET_Y - 30.0f, posz + SPACESHIP_OFFSET_Z + 10.0f,
        5, 6.0f, 2.0f);

    if (!gameOver && !gameWin) {
        for (auto& obs : obstacles) {
            if (obs.active) {
                glPushMatrix();
                glTranslatef(obs.x, obs.y, obs.z);
                glScalef(obs.scale, obs.scale, obs.scale);

                if (obs.type == ObstacleType::ROCK1) {
                    glTranslatef(0, 0, 0);
                    drawObject(&object_rock1);
                }
                else if (obs.type == ObstacleType::ROCK2) {
                    glTranslatef(-1, -1.5, 1.5);
                    drawObject(&object_rock2);
                }
                else if (obs.type == ObstacleType::FIGHTER) {
                    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
                    drawObject(&object_fighter1);
                }
                else if (obs.type == ObstacleType::NEW_A) {
                    glTranslatef(0.0f, -15.0f, 0.0f);
                    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
                    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);
                    drawObject(&object_new_obstacle_A);
                }
                else if (obs.type == ObstacleType::NEW_B) {
                    glTranslatef(0.0f, 0.0f, -16.0f);
                    glRotatef(0.0f, 0.0f, 1.0f, 0.0f);
                    drawObject(&object_new_obstacle_B);
                }
                glPopMatrix();
            }
        }

        for (const auto& bullet : bullets) {
            if (bullet.active) {
                drawBullet(bullet.x, bullet.y, bullet.z);
            }
        }
    }

    drawParticles();

    glDisable(GL_TEXTURE_2D);

    bool hearts_flash = (hearts <= 3 && !gameOver && !gameWin);
    bool show_hearts = true;
    if (hearts_flash) {
        int flash_time = glutGet(GLUT_ELAPSED_TIME);
        show_hearts = (flash_time % 500) < 250;
    }

    if (gameOver || gameWin) {
        if (show_hearts) {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            for (int i = 0; i < hearts; ++i) {
                drawHeart(20 + i * 30, screen_height - 55, 15.0f, true);
            }
            glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
            for (int i = hearts; i < 8; ++i) {
                drawHeart(20 + i * 30, screen_height - 55, 15.0f, false);
            }
        }

        char score_buffer[50];
        sprintf_s(score_buffer, sizeof(score_buffer), "SCORE: %d", score);
        int score_font_size = 36;
        selectFont(score_font_size, ANSI_CHARSET, "Comic Sans MS");
        float score_text_x = 20;
        float score_text_y = screen_height - 35;
        drawString(score_text_x, score_text_y, score_buffer, 1.0f, 1.0f, 1.0f);

        int font_size = 48;
        selectFont(font_size, ANSI_CHARSET, "Comic Sans MS");
        float bounce_offset = sin(glutGet(GLUT_ELAPSED_TIME) / 200.0f) * 10.0f;

        if (gameWin) {
            const char* line1 = "YOU WIN!";
            const char* line2 = "CONGRATULATIONS!";
            float line1_width = getStringWidth(line1, font_size);
            float line1_x = screen_width / 2.0f - line1_width / 2.0f;
            float line1_y = screen_height / 2.0f + 100.0f + bounce_offset;
            float line2_width = getStringWidth(line2, font_size);
            float line2_x = screen_width / 2.0f - line2_width / 2.0f;
            float line2_y = screen_height / 2.0f + 40.0f + bounce_offset;
            drawString(line1_x, line1_y, line1, 1.0f, 1.0f, 0.0f);
            drawString(line2_x, line2_y, line2, 1.0f, 1.0f, 0.0f);
        }
        else {
            const char* line1 = "GAME OVER!";
            const char* line2 = "YOU LOSE!";
            float line1_width = getStringWidth(line1, font_size);
            float line1_x = screen_width / 2.0f - line1_width / 2.0f;
            float line1_y = screen_height / 2.0f + 100.0f + bounce_offset;
            float line2_width = getStringWidth(line2, font_size);
            float line2_x = screen_width / 2.0f - line2_width / 2.0f;
            float line2_y = screen_height / 2.0f + 40.0f + bounce_offset;
            drawString(line1_x, line1_y, line1, 1.0f, 0.0f, 0.0f);
            drawString(line2_x, line2_y, line2, 1.0f, 0.0f, 0.0f);
        }
    }
    else {
        char time_buffer[50];
        sprintf_s(time_buffer, sizeof(time_buffer), "TIME: %d sec", countdown);
        int font_size = 36;
        selectFont(font_size, ANSI_CHARSET, "Comic Sans MS");
        bool show_text = countdown > 10 || static_cast<int>(t * 4.0f) % 2 == 0;
        if (show_text) {
            float text_color_r = 1.0f;
            float text_color_g = 1.0f;
            float text_color_b = 1.0f;
            if (countdown <= 10) {
                text_color_r = 1.0f;
                text_color_g = 0.0f;
                text_color_b = 0.0f;
            }
            drawString(20, screen_height - 40, time_buffer, text_color_r, text_color_g, text_color_b);
        }

        char score_buffer[50];
        sprintf_s(score_buffer, sizeof(score_buffer), "SCORE: %d", score);
        drawString(20, screen_height - 80, score_buffer, 1.0f, 1.0f, 1.0f);

        if (show_hearts) {
            glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            for (int i = 0; i < hearts; ++i) {
                drawHeart(20 + i * 30, screen_height - 110, 15.0f, true);
            }
            glColor4f(0.5f, 0.0f, 0.0f, 1.0f);
            for (int i = hearts; i < 8; ++i) {
                drawHeart(20 + i * 30, screen_height - 110, 15.0f, false);
            }
        }
    }

    glEnable(GL_TEXTURE_2D);
    glutSwapBuffers();
}

void idle() {
    if (gameOver || gameWin) {
        for (auto& p : particles) {
            p.x += p.vx;
            p.y += p.vy;
            p.z += p.vz;
            if (p.has_gravity) {
                p.vy -= 0.05f;
                p.vx *= 0.98f;
                p.vy *= 0.98f;
                p.vz *= 0.98f;
            }
            p.life -= 0.02f;
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(),
            [](const auto& p) { return p.life <= 0.0f; }), particles.end());
        glutPostRedisplay();
        return;
    }

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    int elapsedTime = currentTime - previousTime;
    if (elapsedTime >= 1000) {
        countdown--;
        previousTime = currentTime;
        if (countdown <= 0) {
            if (score >= WIN_SCORE_THRESHOLD) {
                gameWin = true;
            }
            else {
                gameOver = true;
            }
        }
    }

    t += 0.02f;
    if (t >= 1.0f) t -= 1.0f;
    float spaceship_actual_pos_x = posx;
    float spaceship_actual_pos_y = posy + SPACESHIP_OFFSET_Y;
    float spaceship_actual_pos_z = posz + SPACESHIP_OFFSET_Z;

    for (auto& obs : obstacles) {
        if (obs.active) {
            obs.z += OBSTACLE_SPEED;
            if (obs.z > OBSTACLE_DELETE_Z) {
                tryRespawnObstacle(obs);
            }

            if (checkSphereCollision(spaceship_actual_pos_x, spaceship_actual_pos_y, spaceship_actual_pos_z, SPACESHIP_COLLISION_RADIUS,
                obs.x, obs.y, obs.z, obs.collision_radius)) {
                if (obs.type == ObstacleType::FIGHTER || obs.type == ObstacleType::NEW_A) {
                    hearts -= 2;
                    generateExplosion(obs.x, obs.y, obs.z, ExplosionType::COLLISION, obs.type);
                }
                else {
                    hearts -= 1;
                    generateExplosion(obs.x, obs.y, obs.z, ExplosionType::COLLISION, obs.type);
                }
                obs.active = false;
                tryRespawnObstacle(obs);
                if (hearts <= 0) {
                    hearts = 0;
                    gameOver = true;
                }
            }
        }
    }

    for (auto it_bullet = bullets.begin(); it_bullet != bullets.end();) {
        if (it_bullet->active) {
            it_bullet->z -= BULLET_SPEED;
            if (it_bullet->z < BULLET_LIFETIME_Z) {
                it_bullet->active = false;
            }
            else {
                bool hit_obstacle = false;
                for (auto it_obs = obstacles.begin(); it_obs != obstacles.end(); ++it_obs) {
                    if (it_obs->active) {
                        if (checkSphereCollision(it_bullet->x, it_bullet->y, it_bullet->z, it_bullet->collision_radius,
                            it_obs->x, it_obs->y, it_obs->z, it_obs->collision_radius)) {
                            it_bullet->active = false;
                            hit_obstacle = true;
                            if (it_obs->type == ObstacleType::FIGHTER || it_obs->type == ObstacleType::NEW_A) {
                                it_obs->active = false;
                                hearts += 2;
                                score += 10;
                                if (hearts > 8) hearts = 8;
                                generateExplosion(it_obs->x, it_obs->y, it_obs->z, ExplosionType::BULLET_HIT, it_obs->type);
                                tryRespawnObstacle(*it_obs);
                            }
                            break;
                        }
                    }
                }
            }
            if (!it_bullet->active) {
                it_bullet = bullets.erase(it_bullet);
            }
            else {
                ++it_bullet;
            }
        }
        else {
            it_bullet = bullets.erase(it_bullet);
        }
    }

    for (auto& p : particles) {
        p.x += p.vx;
        p.y += p.vy;
        p.z += p.vz;
        if (p.has_gravity) {
            p.vy -= 0.05f;
            p.vx *= 0.98f;
            p.vy *= 0.98f;
            p.vz *= 0.98f;
        }
        p.life -= 0.02f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const auto& p) { return p.life <= 0.0f; }), particles.end());

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (gameOver || gameWin) {
        if (key == 27) exit(0);
        return;
    }

    int currentTime = glutGet(GLUT_ELAPSED_TIME);
    switch (key) {
    case 'a': case 'A': posx -= 3; break;
    case 'd': case 'D': posx += 3; break;
    case 'w': case 'W': posy += 3; break;
    case 's': case 'S': posy -= 3; break;
    case 'z': case 'Z': posz -= 2; break;
    case 'x': case 'X': posz += 8; break;
    case 'g': case 'G':
        if (currentTime - lastFireTime >= BULLET_FIRE_COOLDOWN) {
            Bullet bullet;
            bullet.x = posx;
            bullet.y = posy + SPACESHIP_OFFSET_Y;
            bullet.z = posz + SPACESHIP_OFFSET_Z;
            bullet.velocity_z = BULLET_SPEED;
            bullet.active = true;
            bullet.collision_radius = BULLET_COLLISION_RADIUS;

            bullets.push_back(bullet);
            lastFireTime = currentTime;
        }
        break;
    case 'r': case 'R':
        glPolygonMode(GL_FRONT_AND_BACK, filling ? GL_FILL : GL_LINE);
        filling = !filling;
        break;
    case 27:
        exit(0);
    }
    glutPostRedisplay();
}

void keyboard_s(int key, int x, int y) {
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(screen_width, screen_height);
    glutCreateWindow("3DS Viewer with Flying Obstacles and Bullets");

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboard_s);

    init();
    glutMainLoop();
    return 0;
}
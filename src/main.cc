#include "util.hh"
#include "engine.hh"
#include "json.hpp"
#include <cmath>
#include <utility>

//using namespace std;
using std::cout;
using std::endl;
using std::pair;
using std::map;
using namespace glm;

i32 windowWidth = 1024, windowHeight = 720;
f32 aspectRatio = (f32) windowWidth / windowHeight;
Window *window;

f32 initialFoV = 60.0;
f32 movementSpeed = 10.0;

bool mouseLocked = false;
bool objectChosen = false;
f32 mouseSpeed = 0.001;
f32 mouseWheel = 0.0;
f32 lastMouseWheel = mouseWheel;

f64 mouseRX = 0.0;
f64 mouseLimitRX = 1.4;

bool flyingEnabled = false;
f32 playerHeight = 14;

u32 KEY_DIR_U = GLFW_KEY_W;
u32 KEY_DIR_L = GLFW_KEY_A;
u32 KEY_DIR_D = GLFW_KEY_S;
u32 KEY_DIR_R = GLFW_KEY_D;
u32 KEY_FLY_U = GLFW_KEY_SPACE;
u32 KEY_FLY_D = GLFW_KEY_LEFT_SHIFT;

Camera camera(vec3(4, 14, 23), vec3(-0.15, -PI, 0));

struct Object {
    Model *model;
    Program *shader;

    vec3 origin = vec3(0.0);
    f32 scale = 1.0;
    f32 rotation = 0.0;

    bool visible = true;
    bool selectable = true;
    bool drinkable = false;
};

map<string, Program*> shaders;
map<string, Model*> models;
vector<Object*> objects;

Model sky("res/models/skydome/skydome.obj");
Model cube("res/models/cube/cube.obj");

Object *heldObject = nullptr;
//Object *heldBottle = nullptr;

f32 animationAngle = 0;
f32 clickDelayMax = 1.0;
f32 clickDelay = clickDelayMax;

mat4 P, V;
f32 FoV;

// ----------------------------------------------------------------------------


bool didClick(f32 dt, u32 button) {
    if (clickDelay > 0) {
        clickDelay -= dt;
        return false;
    }

    if (GLFW_PRESS == glfwGetMouseButton(window, button)) {
        clickDelay = clickDelayMax;
        return true;
    }
    
    return false;
}

void centerMouse() {
    if (mouseLocked) {
        glfwSetCursorPos(window, windowWidth / 2.0, windowHeight / 2.0);
    }
}

void unlockMouse() {
    if (!mouseLocked) return;
    mouseLocked = false;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void lockMouse() {
    if (mouseLocked) return;
    mouseLocked = true;
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    centerMouse();
}

bool testAABB(vec3 ray, vec3 worldMin, vec3 worldMax) {
    using std::max;
    using std::min;

    ray = 1.0f / ray;
    f32 t1 = (worldMin.x - camera.pos.x) * ray.x;
    f32 t2 = (worldMax.x - camera.pos.x) * ray.x;
    f32 t3 = (worldMin.y - camera.pos.y) * ray.y;
    f32 t4 = (worldMax.y - camera.pos.y) * ray.y;
    f32 t5 = (worldMin.z - camera.pos.z) * ray.z;
    f32 t6 = (worldMax.z - camera.pos.z) * ray.z;

    f32 tMin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    f32 tMax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us, or
    // if tmin > tmax, ray doesn't intersect AABB
    //f32 minDistance;
    if (tMax < 0 || tMin > tMax) {
        //minDistance = tMax;
        return false;
    } else {
        //minDistance = tMin;
        return true;
    }
}

void toggleFlying() {
    flyingEnabled = !flyingEnabled;
    if (flyingEnabled)
        eprintf("info: flying enabled\n");
    else
        eprintf("info: flying disabled\n");
}

void reloadShaders() {
    for (auto const& [k, v] : shaders) {
        v->reload();
    }
}

bool checkBoundaries(f32 x1, f32 x2, f32 z1, f32 z2){
    return camera.pos.x > x1 && camera.pos.x < x2
        && camera.pos.z > z1 && camera.pos.z < z2;
}


bool collisionDetection(){
    if (flyingEnabled) return false;
    return  camera.pos.x > 28 || camera.pos.x < -29 
        || camera.pos.z > 27 || camera.pos.z < -21 
        || checkBoundaries(-13, 21.5, -26, -4) 
        || checkBoundaries(-10, 18.5, -4, 0) 
        || checkBoundaries(-26.5, -4, 11.5, 18.5) 
        || checkBoundaries(-20, -13.5, 4, 26) 
        || checkBoundaries(1, 23.5, 10, 19) 
        || checkBoundaries(8.5, 15, 3.5, 25) 
        || checkBoundaries(-20, -8, 13.5, 20.5) 
        || checkBoundaries(6.5, 18.5, 11.5, 21);
}

void onUpdate(f32 dt) {
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    centerMouse();

    f32 drx = mouseSpeed * (windowHeight / 2.0 - ypos);
    f32 dry = mouseSpeed * (windowWidth / 2.0 - xpos);
    if (mouseLocked) {
        if (abs(mouseRX + drx) < mouseLimitRX) {
            mouseRX += drx;
            camera.offsetPitch(drx);
        }

        camera.offsetYaw(dry);
    }

    {
        f32 dx = 0, dy = 0, dz = 0;
        if (glfwGetKey(window, KEY_DIR_U) == GLFW_PRESS) dz += dt * movementSpeed;
        if (glfwGetKey(window, KEY_DIR_D) == GLFW_PRESS) dz -= dt * movementSpeed;
        if (glfwGetKey(window, KEY_DIR_R) == GLFW_PRESS) dx += dt * movementSpeed;
        if (glfwGetKey(window, KEY_DIR_L) == GLFW_PRESS) dx -= dt * movementSpeed;
        if (glfwGetKey(window, KEY_FLY_U) == GLFW_PRESS) dy += dt * movementSpeed;
        if (glfwGetKey(window, KEY_FLY_D) == GLFW_PRESS) dy -= dt * movementSpeed;

        camera.offsetRight(dx);
        camera.offsetFront(dz);

        if (collisionDetection()) {
            camera.offsetRight(-dx);
            camera.offsetFront(-dz);
        }

        if (flyingEnabled) playerHeight += dy;
        camera.pos.y = playerHeight;

        f32 deltaWheel = mouseWheel - lastMouseWheel;
        lastMouseWheel = mouseWheel;

        if (heldObject && flyingEnabled) {
            Object &x = *heldObject;
            x.origin = camera.pos + camera.front * vec3(10);
            x.rotation = camera.rot.y + PI;
            x.scale += deltaWheel / 10.0;
        }
    }

    clickDelay -= dt;

    //dprintf("rot: (%.2f, %.2f,  %.2f)\n", camera.rot.x, camera.rot.y, camera.rot.z);
    FoV = initialFoV;
    P = perspective(radians(FoV), aspectRatio, 1.0f, 500.0f);
    V = camera.lookAt();
}

// ----------------------------------------------------------------------------

bool drinking = false;

void onDraw(f32 dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
        Program &shader = *shaders["sky"];
        glUseProgram(shader.id);
        mat4 M(1.0);
        M = translate(M, camera.pos + vec3(0, -50, 0));
        M = scale(M, vec3(300));

        glDepthMask(false);
        glUniformMatrix4fv(shader.u("PVM"), 1, false, value_ptr(P*V*M));
        sky.draw(shader);
        glDepthMask(true);

        M = mat4(1.0);
        M = translate(M, vec3(0, -100, 0));
        M = scale(M, vec3(500, 1, 500));

        glUniformMatrix4fv(shader.u("PVM"), 1, false, value_ptr(P*V*M));
        cube.draw(shader);
    }

    {
        mat4 M(1.0);
        M = scale(M, vec3(62, 50, 58));

        Program &shader = *shaders["phong+mat"];
        glUseProgram(shader.id);
        glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
        glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));
        glUniformMatrix4fv(shader.u("E"), 1, false, value_ptr(camera.pos));
        glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));
        cube.draw(shader);
    }

    /*
    for (i32 i = 0; i < (i32) bottles.size(); i++) {
        if (heldBottle == i) continue;

        Object &object = bottles[i];
        Model &model = *object.model;

        if (!object.visible) continue;

        mat4 M(1.0);
        M = translate(M, object.origin);
        M = scale(M, object.scale);

        vec3 worldMax = M * vec4(model.maxCoords, 1);
        vec3 worldMin = M * vec4(model.minCoords, 1);
        bool selected = testAABB(camera.front, worldMin, worldMax) &&
            glm::length(camera.pos - object.origin) < 15;

        if (selected && heldBottle == -1) {
            M = scale(M, vec3(1.1));


            if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) {
                heldBottle = i;
            }
        }

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        model.draw(prog);
    }
    */


    for (i32 i = 0; i < (i32) objects.size(); i++) {
        if (objects[i] == heldObject) continue;
        Object &object = *objects[i];
        if (!object.visible) continue;
        Model &model = *object.model;
        Program &shader = *object.shader;

        glUseProgram(shader.id);
        glUniformMatrix4fv(shader.u("P"), 1, false, value_ptr(P));
        glUniformMatrix4fv(shader.u("V"), 1, false, value_ptr(V));
        glUniformMatrix4fv(shader.u("E"), 1, false, value_ptr(camera.pos));

        //dprintf("draw object %i\n", i);

        mat4 M(1.0);
        M = translate(M, object.origin);
        M = rotate(M, object.rotation, vec3(0, 1, 0));
        M = scale(M, vec3(object.scale));

        if (!heldObject && object.selectable) {
            vec3 worldMax = M * vec4(model.maxCoords, 1);
            vec3 worldMin = M * vec4(model.minCoords, 1);
            bool selected = testAABB(camera.front, worldMin, worldMax) &&
                length(camera.pos - object.origin) < 15;

            if (selected) {
                M = scale(M, vec3(1.1));

                if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) heldObject = objects[i];

                if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
                    dprintf("o: (%.2f %.2f %.2f), s: (%.2f), r: (0 %.2f 0)\n",
                            object.origin.x, object.origin.y, object.origin.z, object.scale, object.rotation);
                }
            }
        }

        glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));
        model.draw(shader);
    }

    if (heldObject) {
        Object &object = *heldObject;
        Model &model = *object.model;
        Program &prog = *object.shader;

        glUseProgram(prog.id);
        glUniformMatrix4fv(prog.u("P"), 1, false, value_ptr(P));
        glUniformMatrix4fv(prog.u("V"), 1, false, value_ptr(V));
        glUniformMatrix4fv(prog.u("E"), 1, false, value_ptr(camera.pos));

        mat4 M(1.0);

        if (drinking || !flyingEnabled) {
            M = translate(M, camera.pos + camera.front * vec3(1.8, 1, 1.8));
            M = translate(M, vec3(0, -0.8, 0));
            M = scale(M, vec3(object.scale) * 0.5f);
            M = rotate(M, camera.rot.y + PI, vec3(0, 1, 0));
        } else {
            M = translate(M, object.origin);
            M = rotate(M, object.rotation, vec3(0, 1, 0));
            M = scale(M, vec3(object.scale));

        }

        if (drinking) {
            M = rotate(M, animationAngle, vec3(1, 0, 0));

            animationAngle += 0.008;
            if (animationAngle > 1) {
                animationAngle = 0;
                object.visible = false;
                heldObject = nullptr;
                camera.effect1 += 0.05;
                //dprintf("%.2f%%\n", camera.effect1*100);
                drinking = false;
            }
        } else {

            //M = rotate(M, camera.rot.x, vec3(1, 0, 0));

            if (!drinking && didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) {
                heldObject = nullptr;
            }

            if (didClick(dt, GLFW_MOUSE_BUTTON_RIGHT)) {
                drinking = true;
            }

        }

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        model.draw(prog);
    }

    glfwSwapBuffers(window);
}

using nlohmann::json;
using std::cerr;
#include <fstream>
#include <iomanip>
#include <sstream>

string encodeFloat(f32 x) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << x;
    return s.str();
}

f32 decodeFloat(json x) {
    return std::stof(x.get<string>());
}

vector<string> encodeVec3(vec3 x) {
    //return vector<i32>({ (i32) (x.x*PREC), (i32) (x.y * PREC), (i32) (x.z * PREC) });
    return {encodeFloat(x.x), encodeFloat(x.y), encodeFloat(x.z)};
}

vec3 decodeVec3(json x) {
    //return vec3((f32) x[0] / PREC, (f32) x[1] / PREC, (f32) x[2] / PREC);
    return vec3(decodeFloat(x[0]), decodeFloat(x[1]), decodeFloat(x[2]));
}


void saveScene(const string &path) {
    dprintf("saving scene to %s\n", path.c_str());

    json j;

    for (auto& [k, v] : shaders) {
        j["shaders"][k]["vs"] = v->vs;
        j["shaders"][k]["fs"] = v->fs;
    }

    for (auto& [k, v] : models) {
        j["models"][k] = v->path;
    }

    for (auto& x : objects) {
        json o;
        o["model"] = x->model->key;
        o["shader"] = x->shader->key;
        o["origin"] = encodeVec3(x->origin);
        o["scale"] = encodeFloat(x->scale);
        o["rotation"] = encodeFloat(x->rotation);
        o["selectable"] = x->selectable;
        o["drinkable"] = x->drinkable;
        j["objects"].push_back(o);
    }

    std::ofstream file(path);
    file.precision(4);
    file << std::setw(4) << j << endl;
}

void loadScene(const string &path) {
    dprintf("loading scene from %s\n", path.c_str());

    std::ifstream file(path);
    json j;
    file >> j;

    for (auto& [k, v] : j["shaders"].items()) {
        Program *x = new Program(v["vs"], v["fs"]);
        x->key = k;
        shaders.insert({k, x});
    }

    for (auto& [k, v] : j["models"].items()) {
        Model *x = new Model(v);
        x->key = k;
        models.insert({k, x});
    }

    for (auto& v : j["objects"]) {
        Object *x = new Object;

        x->shader = shaders[v["shader"]];
        x->model = models[v["model"]];
        x->origin = decodeVec3(v["origin"]);

        if (v.count("rotation") == 1) x->rotation = decodeFloat(v["rotation"]);
        if (v.count("scale") == 1) x->scale = decodeFloat(v["scale"]);

        if (v.count("selectable") == 1) x->selectable = v["selectable"].get<bool>();
        if (v.count("drinkable") == 1) x->drinkable = v["drinkable"].get<bool>();

        objects.push_back(x);
    }
}

void onInit() {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL);

    loadScene("res/scenes/bar.json");

    for (auto const& [k, v] : shaders) v->load();
    for (auto const& [k, v] : models) v->load();

    sky.load();
    cube.load();

    glBindVertexArray(0);
}

void onExit() {
    for (auto& [k, v] : models) {
        v->unload();
        delete v;
    }

    for (auto& [k, v] : shaders) {
        v->unload();
        delete v;
    }

    sky.unload();
    cube.unload();
}

void onResize(Window *window, i32 width, i32 height) {
    windowWidth = width, windowHeight = height;
    if (height == 0) return;
    glViewport(0, 0, width, height);
    aspectRatio = (f32) width / (f32) height;
    centerMouse();
}

void onMouseScroll(Window *window, f64 xOffset, f64 yOffset) {
    mouseWheel += (f32) yOffset;
}

void onMouseClick(Window *window, i32 button, i32 action, i32 mods) {
    if (action == GLFW_PRESS) {
        f64 x, y;
        glfwGetCursorPos(window, &x, &y);
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (!mouseLocked) lockMouse();
        }
    }
}

void onKeyboard(Window *window, i32 key, i32 code, i32 action, i32 mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) unlockMouse();
        if (key == GLFW_KEY_M) toggleFlying();
        if (key == GLFW_KEY_R) reloadShaders();
        if (key == GLFW_KEY_P) dprintf("pos: (%.2f %.2f %.2f)\n", camera.pos.x, camera.pos.y, camera.pos.z);
        if (key == GLFW_KEY_J) saveScene("res/scenes/bar.json");
        if (key == GLFW_KEY_O) {
        }

    }
}

void onError(i32 code, char *message) {
    eprintf("error %d: %s\n", code, message);
}

int main(void) {
    ilInit();

    if (!glfwInit()) fatalError("failed to init GLEW");
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "CG&V Project", NULL, NULL);
    if (!window) fatalError("failed to open GLFW window");
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetWindowSizeCallback(window, onResize);
    glfwSetKeyCallback(window, onKeyboard);
    glfwSetScrollCallback(window, onMouseScroll);
    glfwSetMouseButtonCallback(window, onMouseClick);
    glfwPollEvents();

    glewExperimental = true;
    if (glewInit() != GLEW_OK) fatalError("failed to init GLEW");

    onInit();
    glfwSetTime(0);
    while (!glfwWindowShouldClose(window)) {
        f32 dt = glfwGetTime();
        glfwSetTime(0);
        onUpdate(dt);
        onDraw(dt);
        glfwPollEvents();
    }

    onExit();
    glfwTerminate();
    exit(0);
}


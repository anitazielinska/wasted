#include "util.hh"
#include "engine.hh"
#include <cmath>

//using namespace std;
using std :: cout;
using std :: endl;
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

f64 mouseRX = 0.0;
f64 mouseLimitRX = 1.4;

bool flyingEnabled = true;
f32 playerHeight = 14;

u32 KEY_DIR_U = GLFW_KEY_W;
u32 KEY_DIR_L = GLFW_KEY_A;
u32 KEY_DIR_D = GLFW_KEY_S;
u32 KEY_DIR_R = GLFW_KEY_D;
u32 KEY_FLY_U = GLFW_KEY_SPACE;
u32 KEY_FLY_D = GLFW_KEY_LEFT_SHIFT;

Camera camera(vec3(4, 14, 23), vec3(-0.15, -PI, 0));

Model sky("res/models/skydome/skydome.obj");
Model cube("res/models/cube/cube.obj");
Model testCube("res/models/cube/cube.obj");
Model beerCan("res/models/poly/Beer.obj");
Model beerBottle("res/models/beer/BeerBottle.obj");
Model wineBottle("res/models/wine/Wine.obj");
Model bar("res/models/bar/Bar.obj");

f32 animationAngle = 0;

Program flatShader("res/shaders/flat_v.glsl", "res/shaders/flat_f.glsl");
Program phongShader("res/shaders/phong_v.glsl", "res/shaders/phong_f.glsl");

struct Bottle {
    Model *model;
    vec3 origin;
    vec3 scale;
    bool shouldDraw = true;

    Bottle(Model *model, vec3 origin, vec3 scale):
        model(model), origin(origin), scale(scale) {}
};

vector<Bottle> bottles;
i32 heldBottle = -1;

mat4 P, V, M;
f32 FoV;

// ----------------------------------------------------------------------------

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
    flatShader.reload();
    phongShader.reload();
}

bool checkBoundaries(f32 x1, f32 x2, f32 z1, f32 z2){
    if (camera.pos.x > x1 and camera.pos.x < x2 and camera.pos.z > z1 and camera.pos.z < z2) return true;
    return false;
}


bool collisionDetection(){
    if (camera.pos.x > 28 or camera.pos.x < -29 or
        camera.pos.z > 27 or camera.pos.z < -21 or
        checkBoundaries(-13, 21.5, -26, -4) or
        checkBoundaries(-10, 18.5, -4, 0) or
        checkBoundaries(-26.5, -4, 11.5, 18.5) or
        checkBoundaries(-20, -13.5, 4, 26) or
        checkBoundaries(1, 23.5, 10, 19) or
        checkBoundaries(8.5, 15, 3.5, 25) or
        checkBoundaries(-20, -8, 13.5, 20.5) or
        checkBoundaries(6.5, 18.5, 11.5, 21)) return true;
    return false;
}


void onUpdate(f32 dt) {
    f64 xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    centerMouse();

    if (mouseLocked) {
        f64 drx = mouseSpeed * (windowHeight / 2.0 - ypos);
        f64 dry = mouseSpeed * (windowWidth / 2.0 - xpos);

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

        if (collisionDetection()){
        camera.offsetRight(-dx);
         camera.offsetFront(-dz);
        }
        //camera.offsetUp(dy);
        if (flyingEnabled) playerHeight += dy;
        camera.pos.y = playerHeight;
    }

    //dprintf("rot: (%.2f, %.2f,  %.2f)\n", camera.rot.x, camera.rot.y, camera.rot.z);
    //dprintf("x: %.2f, y: %.2f,  z: %.2f)\n", camera.pos.x, camera.pos.y, camera.pos.z);
    FoV = initialFoV - 5 * mouseWheel;
    P = perspective(radians(FoV), aspectRatio, 1.0f, 500.0f);
    V = camera.lookAt();
}

// ----------------------------------------------------------------------------
void onDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(flatShader.id);
    glUniformMatrix4fv(flatShader.u("P"), 1, false, value_ptr(P));
    glUniformMatrix4fv(flatShader.u("V"), 1, false, value_ptr(V));

    {
        mat4 M(1.0);
        M = translate(M, camera.pos + vec3(0, -50, 0));
        M = scale(M, vec3(300));
        glDepthMask(false);
        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        sky.draw(flatShader);
        glDepthMask(true);
    }

    glUseProgram(flatShader.id);
    glUniformMatrix4fv(flatShader.u("P"), 1, false, value_ptr(P));
    glUniformMatrix4fv(flatShader.u("V"), 1, false, value_ptr(V));
    //glUniform3fv(phongShader.u("ep"), 1, value_ptr(camera.pos));

    {
        mat4 M(1.0);
        M = scale(M, vec3(62, 50, 58));
        M = translate(M, vec3(0, 0, 0));

        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        cube.draw(flatShader);
    }

    {
        mat4 M(1.0);
        M = translate(M, vec3(0, 0, 0));
        M = scale(M, vec3(3));
        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        bar.draw(flatShader);
    }

    for (int i = 0; i < bottles.size(); i++) {
        if (heldBottle == i) continue;

        Bottle &bottle = bottles[i];
        Model &model = *bottle.model;

        if (!bottle.shouldDraw) continue;

        mat4 M(1.0);
        M = translate(M, bottle.origin);
        M = scale(M, bottle.scale);

        vec3 worldMax = M * vec4(model.maxCoords, 1);
        vec3 worldMin = M * vec4(model.minCoords, 1);
        bool selected = testAABB(camera.front, worldMin, worldMax);

        if (selected) {
            M = scale(M, vec3(1.1));

            if (heldBottle == -1 && GLFW_PRESS == glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
                heldBottle = i;
            }
        }

        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        model.draw(flatShader);
    }

    if (heldBottle != -1) {
        Bottle &bottle = bottles[heldBottle];
        Model &model = *bottle.model;

        mat4 M(1.0);
        M = translate(M, camera.pos + camera.front * vec3(1.8, 1, 1.8));
        M = translate(M, vec3(0, -0.8, 0));
        M = scale(M, vec3(0.5));
        M = scale(M, bottle.scale);

        f32 a = camera.rot.y + PI;
        M = rotate(M, a, vec3(0, 1, 0));
        M = rotate(M, animationAngle, vec3(1, 0, 0));

        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        model.draw(flatShader);

        animationAngle += 0.008;
        if (animationAngle > 1) {
            bottle.shouldDraw = false;
            heldBottle = -1;
            animationAngle = 0;
            camera.effect1 += 0.05;
            dprintf("%.2f%\n", camera.effect1*100);
        }
    }

    glfwSwapBuffers(window);
}

void onInit() {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    reloadShaders();

    sky.load();
    cube.load();
    testCube.load();
    beerCan.load();
    beerBottle.load();
    wineBottle.load();
    bar.load();

    for (int i = 0; i < 5; i++) {
        bottles.push_back(Bottle(&beerBottle, vec3(-8+i, 8, -7), vec3(10, 10, 10)));
    }

    for (int i = 0; i < 5; i++) {
        bottles.push_back(Bottle(&beerCan, vec3(-3+i, 8, -7), vec3(1.0)));
    }

    for (int i = 0; i < 5; i++) {
        bottles.push_back(Bottle(&wineBottle, vec3(2+i, 8, -7), vec3(0.01, 0.01, 0.01)));
    }



    glBindVertexArray(0);
}

void onExit() {
    flatShader.unload();
    phongShader.unload();
    sky.unload();
    beerCan.unload();
    beerBottle.unload();
    wineBottle.unload();
    testCube.unload();
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
        if (button == GLFW_MOUSE_BUTTON_LEFT)
            if (!mouseLocked) lockMouse();
    }
}

void onKeyboard(Window *window, i32 key, i32 code, i32 action, i32 mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) unlockMouse();
        if (key == GLFW_KEY_M) toggleFlying();
        if (key == GLFW_KEY_R) reloadShaders();
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
        onDraw();
        glfwPollEvents();
    }

    onExit();
    glfwTerminate();
    exit(0);
}


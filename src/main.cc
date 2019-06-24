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

Program flatShader("res/shaders/flat_v.glsl", "res/shaders/flat_f.glsl");
Program matShader("res/shaders/mat_v.glsl", "res/shaders/mat_f.glsl");
Program skyShader("res/shaders/sky_v.glsl", "res/shaders/sky_f.glsl");

Model sky("res/models/skydome/skydome.obj");
Model suit("res/models/nanosuit/nanosuit.obj");
Model cube("res/models/cube/cube.obj");
Model testCube("res/models/cube/cube.obj");

Model bar("res/models/poly/Bar.obj");
Model player("res/models/poly/player.obj");
Model plant("res/models/poly/plant.obj");
Model lamp("res/models/poly/Standing_lamp_01.obj");
Model lamp2("res/models/poly/lantern.obj");

Model beer("res/models/poly/beer.obj");
Model beers("res/models/poly/beer_flight.obj");
Model bottle1("res/models/poly/bottle1.obj");
Model bottle2("res/models/poly/bottle2.obj");
Model bottle3("res/models/poly/bottle3.obj");
Model bottle4("res/models/poly/bottle4.obj");
Model bottle5("res/models/poly/bottle5.obj");

vector<Program*> programs({
    &flatShader, &matShader, &skyShader
});

vector<Model*> models({
    &sky, &cube, &testCube, &suit, &bar, &lamp, &lamp2, &plant, &player,
    &beer, &beers, &bottle1, &bottle2, &bottle3, &bottle4, &bottle5
});

struct Object {
    Model *model;
    vec3 origin;
    vec3 scale;
    vec3 rotation;
    bool visible = true;
    bool selectable = false;

    Object(Model *model, vec3 origin, vec3 scale = vec3(1.0), bool selectable = false):
        model(model), origin(origin), scale(scale), selectable(selectable) {}
};

vector<Object> objects;
vector<Object> bottles;

i32 heldObject = -1;
i32 heldBottle = -1;

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
    for (Program *x : programs) x->reload();
    for (Model *x : models) { 
        x->unload();
        x->load();
    }
}

f32 lastMouseWheel = mouseWheel;

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
        //camera.offsetUp(dy);
        if (flyingEnabled) playerHeight += dy;
        camera.pos.y = playerHeight;


        f32 dwh = mouseWheel - lastMouseWheel;
        lastMouseWheel = mouseWheel;

        if (heldObject != -1) {
            Object &x = objects[heldObject];
            //x.origin += camera.front * vec3(1, 0, 1) * dz;
            //x.origin += camera.right * drx;
            //x.origin += camera.up * dy;
            x.origin = camera.pos + camera.front * vec3(10);
            x.rotation.y = camera.rot.y + PI;
            x.scale += vec3(dwh / 10.0);
            //x.origin -= vec3(0, 5, 0);
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
        Program &prog = skyShader;
        glUseProgram(prog.id);
        mat4 M(1.0);
        M = translate(M, camera.pos + vec3(0, -50, 0));
        M = scale(M, vec3(300));

        glDepthMask(false);
        glUniformMatrix4fv(prog.u("PVM"), 1, false, value_ptr(P*V*M));
        sky.draw(prog);
        glDepthMask(true);

        M = mat4(1.0);
        M = translate(M, vec3(0, -100, 0));
        M = scale(M, vec3(500, 1, 500));

        glUniformMatrix4fv(prog.u("PVM"), 1, false, value_ptr(P*V*M));
        cube.draw(prog);
    }

    Program &prog = flatShader;
    glUseProgram(prog.id);
    glUniformMatrix4fv(prog.u("P"), 1, false, value_ptr(P));
    glUniformMatrix4fv(prog.u("V"), 1, false, value_ptr(V));
    glUniformMatrix4fv(prog.u("E"), 1, false, value_ptr(camera.pos));

    {
        mat4 M(1.0);
        M = scale(M, vec3(62, 50, 58));

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        cube.draw(prog);
    }

    {
        mat4 M(1.0);
        M = translate(M, vec3(0, 0, 0));
        M = scale(M, vec3(3));

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        bar.draw(prog);
    }

    for (i32 i = 0; i < (i32) bottles.size(); i++) {
        if (heldBottle == i) continue;

        Object &bottle = bottles[i];
        Model &model = *bottle.model;

        if (!bottle.visible) continue;

        mat4 M(1.0);
        M = translate(M, bottle.origin);

        vec3 worldMax = M * vec4(model.maxCoords, 1);
        vec3 worldMin = M * vec4(model.minCoords, 1);
        bool selected = testAABB(camera.front, worldMin, worldMax) &&
            glm::length(camera.pos - bottle.origin) < 15;

        if (selected && heldBottle == -1) {
            M = scale(M, vec3(1.1));


            if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) {
                heldBottle = i;
            }
        }

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        model.draw(prog);
    }

    if (heldBottle != -1) {
        Object &bottle = bottles[heldBottle];
        Model &model = *bottle.model;

        if (drinking) {

            mat4 M(1.0);
            M = translate(M, camera.pos + camera.front * vec3(1.8, 1, 1.8));
            M = translate(M, vec3(0, -0.8, 0));
            M = scale(M, vec3(0.5));
            M = rotate(M, camera.rot.y + PI, vec3(0, 1, 0));
            M = rotate(M, animationAngle, vec3(1, 0, 0));

            glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
            model.draw(prog);

            animationAngle += 0.008;
            if (animationAngle > 1) {
                animationAngle = 0;
                bottle.visible = false;
                heldBottle = -1;
                camera.effect1 += 0.05;
                //dprintf("%.2f%%\n", camera.effect1*100);
                drinking = false;
            }

        } else {

            mat4 M(1.0);
            //M = translate(M, camera.pos + camera.front * vec3(mouseWheel / 10));
            M = translate(M, camera.pos + camera.front * vec3(2));
            M = translate(M, vec3(0, -0.8, 0));
            M = scale(M, vec3(0.5));
            M = rotate(M, camera.rot.y + PI, vec3(0, 1, 0));
            M = rotate(M, camera.rot.x, vec3(1, 0, 0));
            glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
            model.draw(prog);

            if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) {
                drinking = true;
            }
            if (didClick(dt, GLFW_MOUSE_BUTTON_RIGHT)) {
                heldBottle = -1;
            }

        }
    }

    for (i32 i = 0; i < (i32) objects.size(); i++) {
        Object &object = objects[i];
        if (!object.visible) continue;
        Model &model = *object.model;
        //dprintf("draw object %i\n", i);

        mat4 M(1.0);
        M = translate(M, object.origin);
        M = rotate(M, object.rotation.y, vec3(0, 1, 0));
        M = scale(M, object.scale);

        if (object.selectable) {
            if (heldObject == -1) {
                vec3 worldMax = M * vec4(model.maxCoords, 1);
                vec3 worldMin = M * vec4(model.minCoords, 1);
                bool selected = testAABB(camera.front, worldMin, worldMax);
                if (selected) {
                    M = scale(M, vec3(1.1));

                    if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) heldObject = i;

                    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
                        dprintf("o: (%.2f %.2f %.2f), s: (%.2f), r: (0 %.2f 0)\n",
                                object.origin.x, object.origin.y, object.origin.z, object.scale.x, object.rotation.y);
                    }
                }
            } else {
                if (didClick(dt, GLFW_MOUSE_BUTTON_LEFT)) heldObject = -1;
            }
        }

        glUniformMatrix4fv(prog.u("M"), 1, false, value_ptr(M));
        model.draw(prog);
    }

    glfwSwapBuffers(window);
}

void onInit() {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);

    for (Program *x : programs) x->load();
    for (Model *x : models) x->load();

    for (i32 i = 0; i < 10; i++) {
        bottles.push_back(Object(&beer, vec3(-8+i, 8, -7)));
    }

    objects.push_back(Object(&lamp, vec3(25.0, 1.50, -22.5), vec3(1.40)));
    objects.push_back(Object(&lamp, vec3(-16.0, 1.50, -22.5), vec3(1.40)));

    Object oSuit(&suit, vec3(-24, 1.5, -19.0), vec3(1), false);
    oSuit.rotation.y = 0.43;
    objects.push_back(oSuit);
    
    objects.push_back(Object(&lamp2, vec3(0), vec3(10), true));
    objects.push_back(Object(&plant, vec3(27.35, 1.5, 25.3), vec3(1.6), false));


    objects.push_back(Object(&player, vec3(0), vec3(10), true));
    objects.push_back(Object(&beers, vec3(0), vec3(5.5), true));
    //objects.push_back(Object(&bottle1, vec3(0), vec3(1), true));
    objects.push_back(Object(&bottle2, vec3(0), vec3(1), true));
    objects.push_back(Object(&bottle3, vec3(0), vec3(1), true));
    //objects.push_back(Object(&bottle4, vec3(0), vec3(1), true));
    objects.push_back(Object(&bottle5, vec3(0), vec3(1), true));

    glBindVertexArray(0);
}

void onExit() {
    for (Model *x : models) x->unload();
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


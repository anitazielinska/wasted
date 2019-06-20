#include "util.hh"
#include "engine.hh"

//using namespace std;
using std :: cout;
using std :: endl;
using namespace glm;

i32 windowWidth = 1024, windowHeight = 720;
f32 aspectRatio = (f32) windowWidth / windowHeight;
Window *window;

f32 initialFoV = 45.0;
f32 movementSpeed = 10.0;

bool mouseLocked = false;
bool objectChosen = false;
f32 mouseSpeed = 0.001;
f32 mouseWheel = 0.0;

bool flyingEnabled = true;
f32 playerHeight = 10;

u32 KEY_DIR_U = GLFW_KEY_W;
u32 KEY_DIR_L = GLFW_KEY_A;
u32 KEY_DIR_D = GLFW_KEY_S;
u32 KEY_DIR_R = GLFW_KEY_D;
u32 KEY_FLY_U = GLFW_KEY_SPACE;
u32 KEY_FLY_D = GLFW_KEY_LEFT_SHIFT;

Camera camera(vec3(0, 0, 10), vec3(0, -PI, 0));

Model sky("res/models/skydome/skydome.obj");
Model cube("res/models/cube/cube.obj");
Model testCube("res/models/cube/cube.obj");
Model bottle("res/models/bottles/BP_beer.fbx");
Model bar("res/models/bar/Bar.obj");

f32 cubeAngle = 0;


Program flatShader("res/shaders/flat_v.glsl", "res/shaders/flat_f.glsl");
Program phongShader("res/shaders/phong_v.glsl", "res/shaders/phong_f.glsl");

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

void onUpdate(f32 dt) {
	f64 xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	centerMouse();

	if (mouseLocked) {
		f64 drx = mouseSpeed * (windowHeight / 2.0 - ypos);
		f64 dry = mouseSpeed * (windowWidth / 2.0 - xpos);
		camera.offsetPitch(drx);
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
	}

    //dprintf("pos: (x: %.2f, y: %.2f, z: %.2f)\n", camera.pos.x, camera.pos.y, camera.pos.z);

	if (objectChosen) cubeAngle += 0.01;

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
        M = translate(M, vec3(-8, 8, -16));
        M = scale(M, vec3(15, 15, 15));
        M = rotate(M, cubeAngle, vec3(0, 0, 1));
        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        bottle.draw(flatShader);

        M = translate(M, bottle.center);
        M = scale(M, bottle.size);
        vec3 worldMax = M * vec4(testCube.maxCoords, 1);
        vec3 worldMin = M * vec4(testCube.minCoords, 1);

        //bounding box preview
        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        testCube.draw(flatShader);

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			if (testAABB(camera.front, worldMin, worldMax))
				dprintf("did click the bounding box!\n");
		}
    }

    {
        mat4 M(1.0);
        M = translate(M, vec3(0, 0, 0));
        M = scale(M, vec3(3));
        glUniformMatrix4fv(flatShader.u("M"), 1, false, value_ptr(M));
        bar.draw(flatShader);
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
	bottle.load();
	bar.load();

	glBindVertexArray(0);
}

void onExit() {
	flatShader.unload();
	phongShader.unload();
	sky.unload();
	bottle.unload();
    cube.unload();
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


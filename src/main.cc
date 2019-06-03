#include "common.hh"
#include "game.hh"

#include <cstdio>

using namespace glm;
using namespace std;

i32 windowWidth = 1024, windowHeight = 720;
f32 aspectRatio = (f32) windowWidth / windowHeight;
Window *window;

f32 initialFoV = 45.0;
f32 movementSpeed = 3.0;

bool mouseLocked = false;
f32 mouseSpeed = 0.001;
f32 mouseWheel = 0.0;

bool flyingEnabled = false;
f32 playerHeight = 0;

u32 KEY_DIR_U = GLFW_KEY_W;
u32 KEY_DIR_L = GLFW_KEY_A;
u32 KEY_DIR_D = GLFW_KEY_S;
u32 KEY_DIR_R = GLFW_KEY_D;
u32 KEY_FLY_U = GLFW_KEY_SPACE;
u32 KEY_FLY_D = GLFW_KEY_LEFT_SHIFT;

Camera camera(vec3(0, 0, 5), vec3(0, -PI, 0));
ColorCube cube;
Sky sky;

mat4 P, V, M;

// --------------------------------------

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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	centerMouse();
}

void toggleFlying() {
	flyingEnabled = !flyingEnabled;
	if (flyingEnabled)
		fprintf(stderr, "info: flying enabled\n");
	else
		fprintf(stderr, "info: flying disabled\n");
}

// --------------------------------------


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
		if (flyingEnabled) {
			playerHeight += dy;
		}
		camera.pos.y = playerHeight;
	}

	f32 FoV = initialFoV - 5 * mouseWheel;
	P = perspective(radians(FoV), aspectRatio, -1.0f, 1.0f);
	V = camera.lookAt();
}

// --------------------------------------

void onDraw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	sky.draw(P, V, camera.pos);
	cube.draw(P, V);
	glfwSwapBuffers(window);
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
		if (button == GLFW_MOUSE_BUTTON_LEFT) lockMouse();
	}
}

void onKeyboard(Window *window, i32 key, i32 code, i32 action, i32 mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) unlockMouse();
		if (key == GLFW_KEY_M) toggleFlying();
	}
}

void onError(i32 code, char *message) {
	fprintf(stderr, "error %d: %s\n", code, message);
}

int main(void) {
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

	glClearColor(0.0, 0.0, 0.4, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);

	cube.load();
	sky.load();

	glfwSetTime(0);
	while (!glfwWindowShouldClose(window)) {
		f32 dt = glfwGetTime();
		glfwSetTime(0);
		onUpdate(dt);
		onDraw();
		glfwPollEvents();
	}

	glfwTerminate();
	exit(0);
}


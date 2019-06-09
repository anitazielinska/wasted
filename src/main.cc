#include "util.hh"
#include "engine.hh"

using namespace std;
using namespace glm;

i32 windowWidth = 1024, windowHeight = 720;
f32 aspectRatio = (f32) windowWidth / windowHeight;
Window *window;

f32 initialFoV = 45.0;
f32 movementSpeed = 10.0;

bool mouseLocked = false;
f32 mouseSpeed = 0.001;
f32 mouseWheel = 0.0;

bool flyingEnabled = true;
f32 playerHeight = 2;

u32 KEY_DIR_U = GLFW_KEY_W;
u32 KEY_DIR_L = GLFW_KEY_A;
u32 KEY_DIR_D = GLFW_KEY_S;
u32 KEY_DIR_R = GLFW_KEY_D;
u32 KEY_FLY_U = GLFW_KEY_SPACE;
u32 KEY_FLY_D = GLFW_KEY_LEFT_SHIFT;

Camera camera(vec3(0, 0, 5), vec3(0, -PI, 0));

Model sky("res/models/skydome/skydome.obj");
Model cube("res/models/cube/cube.obj");
Model suit("res/models/nanosuit/nanosuit.obj");

f32 cubeAngle = 0;

Program skyShader("res/shaders/sky_v.glsl", "res/shaders/sky_f.glsl");

mat4 P, V, M;

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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	centerMouse();
}

void toggleFlying() {
	flyingEnabled = !flyingEnabled;
	if (flyingEnabled)
		eprintf("info: flying enabled\n");
	else
		eprintf("info: flying disabled\n");
}

void reloadShaders() {
	skyShader.reload();
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

	cubeAngle += 0.01;

	f32 FoV = initialFoV - 5 * mouseWheel;
	P = perspective(radians(FoV), aspectRatio, 1.0f, 500.0f);
	V = camera.lookAt();
}

void drawSky(Program &shader) {
	mat4 M(1.0);
	M = translate(M, camera.pos + vec3(0, -50, 0));
	M = scale(M, vec3(300));

	glUniformMatrix4fv(shader.u("M"), 1, false, value_ptr(M));

	glDepthMask(false);
	sky.draw(shader);
	glDepthMask(true);
}

// ----------------------------------------------------------------------------

void onDraw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(skyShader.id);
	glUniformMatrix4fv(skyShader.u("P"), 1, false, value_ptr(P));
	glUniformMatrix4fv(skyShader.u("V"), 1, false, value_ptr(V));

	drawSky(skyShader);

	for (i32 x = 0; x < 10; x++) {
	for (i32 y = 0; y < 10; y++) {
		mat4 M(1.0);
		M = translate(M, vec3(2*x - 5, 0.1, 2*y - 5));
		M = rotate(M, cubeAngle, vec3(0, 1, 0));
		glUniformMatrix4fv(skyShader.u("M"), 1, false, value_ptr(M));
		cube.draw(skyShader);
	}
	}

	{
		mat4 M(1.0);
		M = scale(M, vec3(200, 1, 200));
		M = translate(M, vec3(0, -1, 0));
		glUniformMatrix4fv(skyShader.u("M"), 1, false, value_ptr(M));
		cube.draw(skyShader);
	}

	{
		mat4 M(1.0);
		M = translate(M, vec3(0, 0, -20));
		glUniformMatrix4fv(skyShader.u("M"), 1, false, value_ptr(M));
		suit.draw(skyShader);
	}


	glfwSwapBuffers(window);
}

void onInit() {
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);

	reloadShaders();

	sky.load();
	suit.load();
	cube.load();

	glBindVertexArray(0);
}

void onExit() {
	skyShader.unload();
	sky.unload();
	suit.unload();
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
		if (button == GLFW_MOUSE_BUTTON_LEFT) lockMouse();
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


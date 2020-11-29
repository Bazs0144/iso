#include "framework.h"
#include "DebugOpenGL.hpp"
#include "quad.hpp"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glut.h"
#include "imgui/ImGuizmo.h"

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU
Texture3D texture;
Quad fsquad;
vec3 background = vec3(.1, 0, .1);
float resolution;
float isolevel = 50.0;
vec3 rotate = vec3(10, 10, 10);
float distance = 1;
mat4 m4 = mat4(
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
);

struct Camera {

	vec3 wEye, wLookat, wVup, wRight;
	float fov, asp, fp, bp;
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 75.0f * (float)M_PI / 180.0f;
		fp = 1; bp = 20;
		vec3 w = wEye - wLookat;
		float l = length(w);
		float windowSize = l * tanf(fov / 2);
		wRight = normalize(cross(wVup, w)) * windowSize;
	}

};

struct Light {
	vec3 Le;
	vec4 wLightPos;
};

Camera camera;
Light light;

void initScene() {
	camera.wEye = vec3(10, 10, 10);
	camera.wLookat = vec3(0.5, 0.5, 0.5);
	camera.wVup = vec3(0, 1, 0);
	light.wLightPos = vec4(0, 10, 10, -8);
	light.Le = vec3(0.8, 0.8, 0.8);
}

void setUniforms() {
	gpuProgram.Use();
	gpuProgram.setUniform(isolevel, "isolevel");
	gpuProgram.setUniform(resolution, "R");
	gpuProgram.setUniform(camera.wLookat, "lat");
	gpuProgram.setUniform(camera.wEye, "eye");
	gpuProgram.setUniform(normalize(camera.wVup), "up");
	gpuProgram.setUniform(vec3(0.0f, 0.6f, 0.6f), "kd"); //whatever
	gpuProgram.setUniform(background, "background");
	gpuProgram.setUniform(light.Le, "light.Le");
	gpuProgram.setUniform(light.wLightPos, "light.wLightPos");
	gpuProgram.setUniform(distance, "dist");
}

// Our state
// static bool show_demo_window = false;
static bool exact = false;
static float min_range = 0;
static float max_range = 5000;
static bool open = true;
static bool isCameraOpened = false;
static float cameraView[16] =
{ 1.f, 0.f, 0.f, 0.f,
  0.f, 1.f, 0.f, 0.f,
  0.f, 0.f, 1.f, 0.f,
  0.f, 0.f, 0.f, 1.f };
float dist = 1.f;

void my_display_code()
{
	//ImGui::ShowDemoWindow();
	{
	ImGui::Begin("editor", &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);// Create a window called "Hello, world!" and append into it.
		ImGui::SetWindowSize(ImVec2(400, 700));
		ImGui::SetWindowPos(ImVec2(700, 0));

		if (ImGui::CollapsingHeader("Isolevel")) {
			 // Edit bools storing our window open/close state
			if (!exact) {
				ImGui::InputFloat("minimum value:", &min_range);
				ImGui::InputFloat("maximum value:", &max_range);
				ImGui::SliderFloat("isolevel:", &isolevel, min_range, max_range);            // Edit 1 float using a slider from 0.0f to 1.0f
			}
			else {
				ImGui::InputFloat("isolevel", &isolevel);
			}

			ImGui::Checkbox("Exact number", &exact);
		}
		if (ImGui::CollapsingHeader("Background Color")) {
			ImVec4 c = ImVec4(background.x, background.y, background.z, 1.00f);
			ImGui::ColorPicker3("Background Color", (float*)&c);
			background = vec3(c.x, c.y, c.z);
		}
		if (ImGui::CollapsingHeader("Camera")) {
			isCameraOpened = true;
			m4 = mat4(
				cameraView[0], cameraView[1], cameraView[2], cameraView[3],
				cameraView[4], cameraView[5], cameraView[6], cameraView[7],
				cameraView[8], cameraView[9], cameraView[10], cameraView[11],
				cameraView[12], cameraView[13], cameraView[14], cameraView[15]
			);
			//vec4 temp = ((vec4(rotate, 1.f)) * m4);
			//rotate = camera.wLookat + vec3(temp.x, temp.y, temp.z);
			ImGui::SliderFloat("distance", &distance, .1f, 10);
			//rotate = normalize(vec3(temp.x, temp.y, temp.z) - camera.wLookat);
		
		}
		else {
			isCameraOpened = false;
		}
		if (ImGui::CollapsingHeader("Light")) {
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		if (isCameraOpened) {
			ImGuiIO& io = ImGui::GetIO();
			ImGuizmo::ViewManipulate(cameraView, dist, ImVec2(io.DisplaySize.x - 400 - 200, 0), ImVec2(150, 150), 0);
		}
	}
}


// Initialization, create an OpenGL context
void onInitialization() {
	auto err = glGetError();

	DebugOpenGL::init();
	DebugOpenGL::enableCallback(true);
	DebugOpenGL::enableLowSeverityMessages(false);

	glViewport(0, 0, windowWidth - 400, windowHeight);
	glEnable(GL_DEPTH_TEST); //kell?
	ShaderProgramSource source = parserShader("./vertex.vert", "./fragment.frag");

	texture.create("./res/stagbeetle-small.dat");
	resolution = max(max(texture.x, texture.y), texture.z);

	// create program for the GPU
	gpuProgram.create(&(source.VertexSource[0]), &(source.FragmentSource[0]), "outColor");
	
	initScene(); //setup camera and light
	setUniforms();

	fsquad.init();
}

// Window has become invalid: Redraw
void onDisplay() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGLUT_NewFrame();
	ImGuizmo::BeginFrame();

	my_display_code();

	ImGui::Render();
	ImGuiIO& io = ImGui::GetIO();

	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear frame buffer


	initScene(); //setup camera and light
	setUniforms();

	// Set color to (0, 1, 0) = green

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	gpuProgram.Use();
	//gpuProgram.setUniform(MVPtransf[0][0], "MVP");

	int location;
	location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	gpuProgram.setUniform(texture, std::string("vol"), 0); //setting the sampler and linking to the texture

	//glBindVertexArray(vao);  // Draw call
	//glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	fsquad.render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


	glutSwapBuffers(); // exchange buffers for double buffering
	glutPostRedisplay();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	char* buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	glutPostRedisplay();
}

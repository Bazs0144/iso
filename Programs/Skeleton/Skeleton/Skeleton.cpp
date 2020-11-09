#include "framework.h"

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU
Texture3D texture;
unsigned short resolution;
unsigned short isolevel = 50;



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

	//mat4 V() {
	//	vec3 w = normalize(wEye - wLookat);
	//	vec3 u = normalize(cross(wVup, w));
	//	vec3 v = cross(w, u);
	//	return TranslateMatrix(wEye * (-1)) * mat4(u.x, v.x, w.x, 0,
	//		u.y, v.y, w.y, 0,
	//		u.z, v.z, w.z, 0,
	//		0, 0, 0, 1);
	//}

	//mat4 P() {
	//	return mat4(1 / (tan(fov / 2) * asp), 0, 0, 0,
	//		0, 1 / tan(fov / 2), 0, 0,
	//		0, 0, -(fp + bp) / (bp - fp), -1,
	//		0, 0, -2 * fp * bp / (bp - fp), 0);
	//}
};

struct Light {
	vec3 Le;
	vec4 wLightPos;
};

Camera camera;
Light light;

void initScene() {
	camera.wEye = vec3(0, 0, 2);
	camera.wLookat = vec3(0, 0, 0);
	camera.wVup = vec3(0, 1, 0);
	light.wLightPos = vec4(-10, 10, 10, 8);
	light.Le = vec3(0.8, 0.8, 0.8);
}

void setUniforms() {
	gpuProgram.setUniform(isolevel, "isolevel");
	gpuProgram.setUniform(resolution, "R");
	gpuProgram.setUniform(camera.wLookat, "lat");
	gpuProgram.setUniform(camera.wEye, "eye");
	gpuProgram.setUniform(normalize(camera.wVup), "up");
	gpuProgram.setUniform(vec3(0.0f, 0.6f, 0.6f), "kd"); //whatever
	gpuProgram.setUniform(vec4(0,0,0,0), "background");
	gpuProgram.setUniform(light.Le, "light.Le");
	gpuProgram.setUniform(light.wLightPos, "light.wLightPos");
}

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glEnable(GL_DEPTH_TEST); //kell?
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_3D);
	ShaderProgramSource source = parserShader("./vertex.vert", "./fragment.frag");

	// create program for the GPU
	gpuProgram.create(&(source.VertexSource[0]), &(source.FragmentSource[0]), "outColor");
	initScene(); //setup camera and light
	setUniforms(); 

	texture = Texture3D("./res/stagbeetle.dat");
	resolution = max(max(texture.x, texture.y), texture.z);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active
	unsigned int vbo;		// vertex buffer object
	glGenBuffers(1, &vbo);	// Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Geometry: fullscreen quad
	float cVtx[] = { -1,-1, 1,-1, 1,1, -1, 1 };
	glBufferData(GL_ARRAY_BUFFER, sizeof(cVtx), cVtx, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);  // AttribArray 0
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed

}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	// Set color to (0, 1, 0) = green
	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, .1f, 0.0f, .1f); // 3 floats

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	gpuProgram.setUniform(texture, std::string("vol")); //setting the sampler and linking to the texture

	glBindVertexArray(vao);  // Draw call
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glutSwapBuffers(); // exchange buffers for double buffering
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
}

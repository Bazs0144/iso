#include "framework.h"




unsigned short dimensions[3]; //x,y,z dims
unsigned short resolution = 100;
unsigned short isolevel = 2000;

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

//struct Material {
//
//	vec3 kd, ks, ka;
//	float shininess;
//};

struct Light {

	vec3 La, Le;
	vec4 wLightPos;

	void Animate(float t) {	}
};

struct RenderState {
	//mat4  V, P, MVP, M, Minv,
	//Material* material;
	Light light;
	float isolevel, R, dt;
	vec3 eye, lat, ri, up;
	vec3 kd, background;
};


class IsoShader : public GPUProgram {
	const char* vertexSource = R"(
		#version 330
		precision highp float;

		in vec3 uvin;
		out vec3 uvout;
	
		void main(){
			uvout = uvin;
		}

	)";

	const char* fragmentSource = R"(
		#version 330
		precision highp float;

		uniform sampler3D vol;
		uniform float isolevel, R, dt;
		uniform vec3 eye, lat, ri, up;
		uniform Light light;
		uniform vec3 kd, background;

		in vec2 uv;
		out vec4 color;

		void main() { 
		  vec3 p = lat+ri*(2*uv.x-1)+up*(2*uv.y-1);
		  vec3 dir = normalize(p–eye);
		  vec3 t0 = (vec3(0,0,0)-eye)/dir, t1 = (vec3(1,1,1)-eye)/dir;
		  vec3 ti = min(t0, t1), to = max(t0, t1);
		  float en=max(max(ti.x,ti.y),ti.z), ex=min(min(to.x,to.y),to.z);
		  color = background;
		  vec3 dx=vec3(1/R,0,0), dy=vec3(0,1/R,0), dz=vec3(0,0,1/R); 
		  for(float t = en; t < ex; t += dt) {
			 vec3 q = eye + dir * t;
			 vec3 L = normalize(light.wLightPos.xyz-q*light.wLightPos.w); //since wLightPos in hom.coord.
			 if (texture(vol, q).x > isolevel) {
				vec3 N = vec3(texture(vol, q+dx) – texture(vol, q-dx),
						   texture(vol, q+dy) – texture(vol, q-dy),
							  texture(vol, q+dz) – texture(vol, q-dz)); 
			 color = light.Le * kd * max(dot(L, normalize(N)), 0);
			 }
		  }
		}

	)";

public:
	IsoShader() { create(vertexSource, fragmentSource, "fragmentColor"); }

	void Bind(RenderState state) {
		Use(); 		// make this program run
		setUniform(state.isolevel, "isolevel");
		setUniform(state.R, "R");
		setUniform(state.dt, "dt");
		setUniform(state.lat, "lat");
		setUniform(state.eye, "eye");
		setUniform(state.ri, "ri");
		setUniform(state.up, "up");
		setUniform(state.kd, "kd");
		setUniform(state.background, "background");
	}

	void setUniformLight(const Light& light, const std::string& name) {
		setUniform(light.La, name + ".La");
		setUniform(light.Le, name + ".Le");
		setUniform(light.wLightPos, name + ".wLightPos");
	}

	//void setUniformMaterial(const Material& material, const std::string& name) {
//	setUniform(material.kd, name + ".kd");
//	setUniform(material.ks, name + ".ks");
//	setUniform(material.ka, name + ".ka");
//	setUniform(material.shininess, name + ".shininess");
//}

};

//struct VertexData {
//
//	vec3 position, normal;
//	vec2 texcoord;
//};

Camera camera;
Light light;
IsoShader isoShader;

//read volume data from file
void loadVolumeData(unsigned short* vd) {
	FILE* fp = fopen("stagbeetle832x832x494.dat", "rb");
	fread((void*)dimensions, 3, sizeof(unsigned short), fp);
	int datasize = int(dimensions[0]) * int(dimensions[1]) * int(dimensions[2]);
	vd = new unsigned short[datasize];
	fread((void*)vd, datasize, sizeof(unsigned short), fp);
	fclose(fp);
}

class FullScreenTexturedQuad {
	unsigned int vao = 0, textureID = 0;
	unsigned short* volData;
public:
	FullScreenTexturedQuad(int windowWidth, int windowHeight) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		unsigned int vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		float vertexCoords[] = { -1, -1,  1, -1,  1, 1,  -1, 1 };
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		loadVolumeData(volData);
		unsigned int textureID;
		glGenTextures(1, &textureID);
		const unsigned int textureUnit = 0; //selecting the first tex.unit
		int samplerLocation = glGetUniformLocation(isoShader.getId(), "vol");
		if (samplerLocation >= 0) {
			glActiveTexture(GL_TEXTURE0 + textureUnit); //selecting textureunit 0 for our texture to bind to
			glBindTexture(GL_TEXTURE_3D, textureID);
			glUniform1i(samplerLocation, textureUnit); //linking the sampler to the same tex.unit where the texture was bound
		}
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP); //or clamp_to_border?
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_INTENSITY, dimensions[0], dimensions[1], dimensions[2], 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, &volData[0]);
		delete[] volData;
	}

	void Draw() {
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
};

FullScreenTexturedQuad* quad;

void initScene() {
	camera.wEye = vec3(0, 0, 6);
	camera.wLookat = vec3(0, 0, 0);
	camera.wVup = vec3(0, 1, 0);
	light.wLightPos = vec4(-10, 10, 10, 3);
	light.La = vec3(0.1f, 0.1f, 0.1f);
	light.Le = vec3(0.8, 0.8, 0.8);
}

void Animate(float tstart, float tend) {}

void setupRenderState() {
	RenderState state;
	state.eye = camera.wEye;
	state.light = light;
	state.lat = camera.wLookat;
	state.up = normalize(camera.wVup);
	state.ri = normalize(cross(state.up, state.lat - state.eye));
	state.kd = vec3(0.0f, 0.6f, 0.6f);
	state.background = vec3(0, 0, 0);
	state.R = resolution;
	state.dt = 1 / resolution;
	isoShader.Bind(state);
}


void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	glEnable(GL_DEPTH_TEST); //kell?
	glDisable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_3D);
	quad = new FullScreenTexturedQuad(windowWidth, windowHeight);
	initScene();
}


void onDisplay() {
	glClearColor(0.5f, 0.5f, 0.8f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	setupRenderState();
	quad->Draw();
	glutSwapBuffers();
}


void onKeyboard(unsigned char key, int pX, int pY) {
	glutPostRedisplay();
}


void onKeyboardUp(unsigned char key, int pX, int pY) { }


void onMouse(int button, int state, int pX, int pY) { }


void onMouseMotion(int pX, int pY) {
}


void onIdle() {
	static float tend = 0;
	const float dt = 0.1f;
	float tstart = tend;
	tend = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

	for (float t = tstart; t < tend; t += dt) {
		float Dt = fmin(dt, tend - t);
	}
	glutPostRedisplay();
}


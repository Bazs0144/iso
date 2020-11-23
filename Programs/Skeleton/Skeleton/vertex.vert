#version 330

precision highp float;		// normal floats, makes no difference on desktop computers

uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
layout(location = 0) in vec3 vp;	// Varying input: vp = vertex position is expected in attrib array 0

out vec2 uv;
	
void main(){
	uv = (vp.xy + 1.0f)/2.0f;
	gl_Position = vec4(vp, 1);
}
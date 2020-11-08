#version 330

precision highp float;		// normal floats, makes no difference on desktop computers

uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

out vec2 uv;
	
void main(){
	uv = (vp + 1.0f)/2.0f;
}
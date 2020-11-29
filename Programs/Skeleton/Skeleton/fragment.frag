#version 330
precision highp float;	// normal floats, makes no difference on desktop computers

struct Light {
	vec3 La, Le;
	vec4 wLightPos;
};

uniform sampler3D vol;
uniform float isolevel, R; //resolution, step
uniform vec3 eye, lat, up;
uniform Light light;
uniform vec3 kd, background;

in vec2 uv;
out vec4 color;

void main() { 
	float dt = 1.0f/R;
	vec3 ri = normalize(cross(up, lat - eye));
	vec3 p = lat + ri * (2 * uv.x - 1) + up * (2 * uv.y - 1);
	vec3 dir = normalize(p - eye);
	//calculating intersection with 3 planes in one step, each vec3 coord represents a plane
	vec3 t0 = (vec3(0,0,0) - eye)/dir; //intersection coords with x=0, y=0, z=0 planes
	vec3 t1 = (vec3(1,1,1) - eye)/dir; //intersection coords with x=1, y=1, z=1 planes
	vec3 ti = min(t0, t1); //closest points
	vec3 to = max(t0, t1); //farthest points
	float en = max(max(ti.x,ti.y),ti.z); //actual entrance point
	float ex = min(min(to.x,to.y),to.z); //actual exit point
	color = vec4(background.x, background.y, background.z, 1);
	vec3 dx=vec3(1/R,0,0), dy=vec3(0,1/R,0), dz=vec3(0,0,1/R); 
	for(float t = en; t < ex; t += dt) { //stepping from entrance to exit point
		vec3 q = eye + dir * t;
		vec3 L = normalize(light.wLightPos.xyz - q*light.wLightPos.w); //wLightPos in hom.coord., also accurate in case of directional lightsource
		float density = texture(vol,q).x;
		if (density > isolevel) {
			vec3 N = vec3(float(texture(vol, q+dx) - texture(vol, q-dx)),
						float(texture(vol, q+dy) - texture(vol, q-dy)),
						float(texture(vol, q+dz) - texture(vol, q-dz)));
			vec3 illum = light.Le * kd * max(dot(L, normalize(N)), 0.0f);
			color = vec4(illum.x, illum.y, illum.z, 1);
		}
	}
	//color = texture(vol, vec3(uv, 64));
}

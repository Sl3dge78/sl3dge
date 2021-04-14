#version 460

layout(location = 0) in vec3 normal;

layout(location = 0) out vec4 out_color;

void main() {
	
	vec3 light_dir = normalize(vec3(0.5,-0.5,0.5));
	
	float ndotl = dot(light_dir, normal);
	
	vec3 color = vec3(0.1);

	if(ndotl > 0) {
		color += vec3(ndotl);
	}

    out_color = vec4(color, 1.0);
}
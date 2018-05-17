#version 430

in vec3 pos;
in vec2 tex;
in vec3 normal;

uniform vec3 lightPos;
uniform vec3 eyePos;

uniform sampler2D texture_diffuse1;

out vec4 fragCol;

void main() {

	vec4 albedo = texture(texture_diffuse1, tex);
	vec4 ambient = albedo * 0.2f;

	vec3 lightDir = normalize(lightPos - pos);
	vec3 eyeDir = normalize(eyePos - pos);
	vec4 diffuse = max(0.0f, dot(lightDir,  normal)) * albedo;
	
	vec3 half_v = normalize(lightDir + eyeDir);

	vec4 specular = vec4(0.0f);
	float spec_factor = pow(max(dot(normal, half_v), 0.0f), 32);
	specular = spec_factor * vec4(0.3f, 0.3f, 0.3f, 0.3f);

	fragCol = ambient + diffuse + specular;

	//debug view
	//see normal
	//fragCol = vec4(normal, 1.0f);
	//see albedo
	//fragCol = albedo;
	//see position
	//fragCol = vec4(pos ,1.0);
}
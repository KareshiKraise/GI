#version 430

#define NUM_CLUSTERS 4

layout(binding = 0) uniform sampler2D gpos;
layout(binding = 1) uniform sampler2D gnormal;
layout(binding = 2) uniform sampler2D gmaterial;
layout(binding = 3) uniform sampler2D gdepth;
layout(binding = 4) uniform sampler2D shadowmap;
layout(binding = 5) uniform sampler2DArray val_shadowmaps;

in vec2 tex;

uniform vec3 lightColor;
uniform vec3 lightDir;
uniform vec3 eye_pos;
uniform mat4 lightMat;
uniform int num_vpls;
uniform mat4 M;
uniform float parabola_near;
uniform float parabola_far;

out vec4 color;


layout(std140, binding = 0) uniform vpl_matrices{
	mat4 parabolic_mats[NUM_CLUSTERS];
};

struct plight {
	vec4 p; //position + radius
	vec4 n; //normal + cluster index
	vec4 c; //color	
};

//first bounce list
layout(std430, binding = 1) buffer lightSSBO {
	plight list[];
};

//second bounce list
layout(std430, binding = 3) buffer light2SSBO {
	plight list_2nd[];
};


float shadow_test(vec3 normal, vec4 fragPosLS){
    vec3 projected = fragPosLS.xyz/fragPosLS.w;
    projected = projected * 0.5 + 0.5;
    float closest = texture(shadowmap, projected.xy).r;
    float current = projected.z;

    normal = normalize(normal);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = 0.0f;
    shadow = (current - bias)  > closest ? 0.0f : 1.0f;
    return shadow;    
}

#define EPSILON 0.1f
float ParabolicShadow(vec4 worldPos, mat4 lightV, float layer) {

	vec4 lpos = lightV * M * worldPos;
	float L = length(lpos.xyz);
	lpos /= L;
	float ret = 0.0f;

	float fdepth;
	float fscene_depth;

	vec2 tex_coord;
	tex_coord.x = (lpos.x / (1.0f + lpos.z)) * 0.5f + 0.5f;
	tex_coord.y = (lpos.y / (1.0f + lpos.z)) * 0.5f + 0.5f;

	fscene_depth = (L - parabola_near) / (parabola_far - parabola_near);

	fdepth = texture(val_shadowmaps, vec3(tex_coord, layer)).r;
	ret = ((fscene_depth - EPSILON) > fdepth) ? 0.0f : 1.0f;

	return ret;
}

vec3 area_irradiance(plight vpl, vec3 xpos, vec3 xnormal){
  
    vec3 direction = normalize(vpl.p.xyz - xpos);
    float dist = length(xpos-vpl.p.xyz);    
    float d2 = dist*dist;
    float irradiance = max(dot(xnormal, direction), 0.f) * max(dot(vpl.n.xyz, -direction), 0.f) ;   
    return vec3(irradiance);
}

void main(){  
    
    vec4 fragPos = vec4(texture(gpos, tex).xyz, 1.0);
    vec3 fragNormal = texture(gnormal, tex).xyz;           
    float shadow_factor = shadow_test(fragNormal, lightMat * fragPos);

    vec3 albedo = texture(gmaterial, tex).rgb;
    float spec = texture(gmaterial, tex).a;          
    float diffuse = max(0.0, dot(lightDir, fragNormal));
    float ambient = 0.0f;
    
    vec3 eye_dir = normalize(eye_pos - fragPos.xyz);
	vec3 half_vector = normalize(eye_dir + lightDir);

	float spec_val = pow( max( dot(fragNormal.xyz, half_vector), 0.0), 16);
    spec_val = spec_val * spec;
    
    vec3 direct = (shadow_factor * (diffuse + spec_val)) * vec3(1.0f);
       
    vec3 indirect = vec3(0.f);

    for(int i=0;i<num_vpls;++i)
    {    
        plight curr_light = list[i];        
        float layer = curr_light.n.w;
        float ret = ParabolicShadow(vec4(fragPos.xyz, 1.0), parabolic_mats[int(layer)], layer);
        indirect += ret * area_irradiance(curr_light, fragPos.xyz, fragNormal.xyz);
    }

    indirect /= (num_vpls);

    color = vec4((direct+indirect)*albedo, 1.0f);
    
    //color = vec4(spec, spec, spec, 1.0f);    
    //color = (ambient + ((diffuse) * shadow_factor)) * vec4(albedo,1.0); 
}
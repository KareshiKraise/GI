#version 430

layout(local_size_x = 16, local_size_y = 16) in;

uniform mat4 invProj;
uniform float Wid;
uniform float Hei;

struct frustum {
	vec4 planes[4];
};

layout(std430, binding = 3) buffer frusta {
	frustum f[];
};


vec4 unproject(vec4 v) {
	v = invProj * v;
	v /= v.w;
	return v;
}

//create a plane from three non colinear points
vec4 create_plane(vec3 p0, vec3 p1, vec3 p2) {
	vec3 v1 = p1 - p0;
	vec3 v2 = p2 - p0;

	vec4 N;
	N.xyz = normalize(cross(v1, v2));
	N.w = dot(N.xyz, p0);

	return N;
}

//create a plane from two vectors (assuming one of three points is the origin 0,0,0)
vec4 create_plane2(vec4 a, vec4 b) {
	vec4 normal;
	normal.xyz = normalize(cross(a.xyz, b.xyz));
	normal.w = 0;
	return normal;
}

void main() {
	//Compute frustum		
	//min and max coords of screen space tile
	if (gl_LocalInvocationIndex == 0)
	{
		uint minX = gl_WorkGroupSize.x * gl_WorkGroupID.x;
		uint minY = gl_WorkGroupSize.y * gl_WorkGroupID.y;
		uint maxX = gl_WorkGroupSize.x * (gl_WorkGroupID.x + 1);
		uint maxY = gl_WorkGroupSize.y * (gl_WorkGroupID.y + 1);

		float c_minY = (float(minY) / Hei)  *  2.0f - 1.0f;
		float c_minX = (float(minX) / Wid)  *  2.0f - 1.0f;
		float c_maxX = (float(maxX) / Wid)  *  2.0f - 1.0f;
		float c_maxY = (float(maxY) / Hei)  *  2.0f - 1.0f;

		//Corners in NDC
		vec4 corners[4];

		//project corners to far plane
		corners[0] = unproject(vec4(c_minX, c_maxY, 1.0f, 1.0f));
		corners[1] = unproject(vec4(c_maxX, c_maxY, 1.0f, 1.0f));
		corners[2] = unproject(vec4(c_maxX, c_minY, 1.0f, 1.0f));
		corners[3] = unproject(vec4(c_minX, c_minY, 1.0f, 1.0f));

		unsigned int index = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
		for (int i = 0; i < 4; i++)
		{
			//create planes with normals pointing to the inside splace of the frustum
			f[index].planes[i] = create_plane2(corners[i], corners[(i + 1) & 3]);
		}
	}

	barrier();
}
#ifdef VERTEX_SHADER

layout (location = 0) in v3 Position;

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

void main()
{
#ifdef MESH_SHADER
	gl_Position = Projection * View * Model * V4(Position, 1.0f);

#else
	gl_Position = Projection * View * V4(Position, 1.0f);

#endif

}


#endif
#ifdef FRAGMENT_SHADER

layout(location = 0) out v2 Depth;

uniform f32 FarPlane;
uniform b32 Orthogonal;

void main()
{

	if(Orthogonal)
	{
		gl_FragDepth = gl_FragCoord.z;
	}
	else
	{
		gl_FragDepth = gl_FragCoord.z/(gl_FragCoord.w*FarPlane);
	}

	Depth.x = gl_FragDepth;
	Depth.y = Square(Depth.x);
}

#endif

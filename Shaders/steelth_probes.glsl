#ifdef COMPUTE_SHADER

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer SurfelBuffer
{
	surface_element Surfels[MAX_SURFELS];
};

layout(std430, binding = 1) buffer SurfelRefBuffer
{
	u32 SurfelRefs[MAX_SURFELREFS];
};

layout(std430, binding = 2) buffer ProbeBuffer
{
	light_probe Probes[MAX_PROBES];
};

void main()
{
	u32 Index = gl_GlobalInvocationID.x;
	light_probe Probe = Probes[Index];

	for(u32 ProbeDirIndex = 0;
		ProbeDirIndex < 6;
		++ProbeDirIndex)
	{
		Probes[Index].LightContributions_SkyVisibility[ProbeDirIndex].xyz = V3(0,0,0);
	}

	for(u32 SurfelRefIndex = Probe.FirstSurfel;
		SurfelRefIndex < Probe.OnePastLastSurfel;
		++SurfelRefIndex)
	{
		u32 SurfelIndex = SurfelRefs[SurfelRefIndex];
		surface_element Surfel = Surfels[SurfelIndex];
		v3 SurfelDirection = (Surfel.P - Probe.P);
		f32 SurfelDist = Length(SurfelDirection);
		SurfelDirection = (1.0f/SurfelDist)*SurfelDirection;

		for(u32 ProbeDirIndex = 0;
			ProbeDirIndex < 6;
			++ProbeDirIndex)
		{
			v3 Direction = ProbeDirections[ProbeDirIndex];
			f32 ProbeNormCos = Inner(Direction, SurfelDirection);
			if(ProbeNormCos > 0.0f)
			{
				f32 SurfelNormCos = Maximum(Inner(Surfel.Normal, -SurfelDirection), 0.0f);
				f32 SolidAngle = Surfel.Area*SurfelNormCos / Square(SurfelDist);

				Probes[Index].LightContributions_SkyVisibility[ProbeDirIndex].xyz += ProbeNormCos * SolidAngle * Surfel.LitColor / Pi32;
			}
		}
	}
}

#endif

#ifdef COMPUTE_SHADER

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer ProbeBuffer
{
	light_probe Probes[MAX_PROBES];
};

layout(rgba16f, binding = 1) uniform image3D IrradianceVolume;

uniform s32 ProbeCount;
uniform v3 Center;

void main()
{
	v3i Coords = V3i(gl_WorkGroupID);
	v3 RegionDim = V3(IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM,
					  IRRADIANCE_VOLUME_HEIGHT * IRRADIANCE_VOXEL_DIM,
					  IRRADIANCE_VOLUME_WIDTH * IRRADIANCE_VOXEL_DIM);
	v3 RegionMin = Center - 0.5f*RegionDim;
	v3 VoxelDim = V3(IRRADIANCE_VOXEL_DIM, IRRADIANCE_VOXEL_DIM, IRRADIANCE_VOXEL_DIM);
	v3 VoxelP = RegionMin + V3(Coords.x, Coords.y, Coords.z)*VoxelDim;
	v3 LightContribution = V3(0,0,0);

	u32 ClosestProbes[4];
	f32 MinDistSq[4] = {1000000000.0f, 1000000000.0f, 1000000000.0f, 1000000000.0f};
	for(u32 ProbeIndex = 0;
		ProbeIndex < ProbeCount;
		++ProbeIndex)
	{
		light_probe Probe = Probes[ProbeIndex];
		f32 DistSq = LengthSq(Probe.P - VoxelP);
		if(DistSq < MinDistSq[3])
		{
			MinDistSq[3] = DistSq;
			ClosestProbes[3] = ProbeIndex;

			u32 TempIndex;
			f32 TempDist;
			if(MinDistSq[3] < MinDistSq[2])
			{
				TempDist = MinDistSq[2];
				MinDistSq[2] = MinDistSq[3];
				MinDistSq[3] = TempDist;

				TempIndex = ClosestProbes[2];
				ClosestProbes[2] = ClosestProbes[3];
				ClosestProbes[3] = TempIndex;
			}
			if(MinDistSq[2] < MinDistSq[1])
			{
				TempDist = MinDistSq[1];
				MinDistSq[1] = MinDistSq[2];
				MinDistSq[2] = TempDist;

				TempIndex = ClosestProbes[1];
				ClosestProbes[1] = ClosestProbes[2];
				ClosestProbes[2] = TempIndex;
			}
			if(MinDistSq[1] < MinDistSq[0])
			{
				TempDist = MinDistSq[0];
				MinDistSq[0] = MinDistSq[1];
				MinDistSq[1] = TempDist;

				TempIndex = ClosestProbes[0];
				ClosestProbes[0] = ClosestProbes[1];
				ClosestProbes[1] = TempIndex;
			}
		}
	}

	v4 VoxelColors[6];
	if(MinDistSq[0] == 0.0f)
	{
		light_probe ClosestProbe = Probes[ClosestProbes[0]];
		VoxelColors[0] = ClosestProbe.LightContributions_SkyVisibility[0];
		VoxelColors[1] = ClosestProbe.LightContributions_SkyVisibility[1];
		VoxelColors[2] = ClosestProbe.LightContributions_SkyVisibility[2];
		VoxelColors[3] = ClosestProbe.LightContributions_SkyVisibility[3];
		VoxelColors[4] = ClosestProbe.LightContributions_SkyVisibility[4];
		VoxelColors[5] = ClosestProbe.LightContributions_SkyVisibility[5];
	}
	else
	{
		light_probe Probe0 = Probes[ClosestProbes[0]];
		light_probe Probe1 = Probes[ClosestProbes[1]];
		light_probe Probe2 = Probes[ClosestProbes[2]];
		light_probe Probe3 = Probes[ClosestProbes[3]];
		f32 Numerator = 1.0f/((1.0f/MinDistSq[0]) +
					 	 	  (1.0f/MinDistSq[1]) +
					 	 	  (1.0f/MinDistSq[2]) +
					 	 	  (1.0f/MinDistSq[3]));
		VoxelColors[0] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[0] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[0] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[0] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[0];
		VoxelColors[1] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[1] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[1] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[1] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[1];
		VoxelColors[2] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[2] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[2] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[2] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[2];
		VoxelColors[3] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[3] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[3] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[3] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[3];
		VoxelColors[4] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[4] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[4] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[4] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[4];
		VoxelColors[5] = (Numerator/MinDistSq[0])*Probe0.LightContributions_SkyVisibility[5] +
				   (Numerator/MinDistSq[1])*Probe1.LightContributions_SkyVisibility[5] +
				   (Numerator/MinDistSq[2])*Probe2.LightContributions_SkyVisibility[5] +
				   (Numerator/MinDistSq[3])*Probe3.LightContributions_SkyVisibility[5];
	}

	v3i Offset = V3i(IRRADIANCE_VOLUME_WIDTH, 0, 0);
	imageStore(IrradianceVolume, Coords + 0*Offset, VoxelColors[0]);
	imageStore(IrradianceVolume, Coords + 1*Offset, VoxelColors[1]);
	imageStore(IrradianceVolume, Coords + 2*Offset, VoxelColors[2]);
	imageStore(IrradianceVolume, Coords + 3*Offset, VoxelColors[3]);
	imageStore(IrradianceVolume, Coords + 4*Offset, VoxelColors[4]);
	imageStore(IrradianceVolume, Coords + 5*Offset, VoxelColors[5]);
}

#endif

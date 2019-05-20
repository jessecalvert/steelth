/*@H
* File: steelth_renderer_camera.h
* Author: Jesse Calvert
* Created: April 23, 2019, 17:11
* Last modified: April 23, 2019, 17:14
*/

#pragma once

enum camera_type
{
	Camera_Null,
	Camera_Orthogonal,
	Camera_Perspective,
};

INTROSPECT()
struct camera
{
	camera_type Type;
	union
	{
		struct
		{
			// NOTE: projection
			f32 HorizontalFOV;
			f32 AspectRatio;
		};

		struct
		{
			// NOTE: orthogonal
			f32 HalfWidth;
			f32 HalfHeight;
		};
	};

	f32 Near;
	f32 Far;

	v3 P;
	quaternion Rotation;

	mat4 Projection_;
	mat4 View_;
	mat4 InvView_;
};

inline void
MoveCamera(camera *Camera, v3 NewP)
{
	Camera->P = NewP;
}

inline void
RotateCamera(camera *Camera, quaternion NewRotation)
{
	Camera->Rotation = NormalizeIfNeeded(NewRotation);
}

inline void
ChangeCameraSettings(camera *Camera, f32 HalfWidth_HorizontalFOV, f32 HalfHeight_AspectRatio,
	f32 Near, f32 Far)
{
	switch(Camera->Type)
	{
		case Camera_Orthogonal:
		{
			Camera->HalfWidth = HalfWidth_HorizontalFOV;
			Camera->HalfHeight = HalfHeight_AspectRatio;
			Camera->Near = Near;
			Camera->Far = Far;
		} break;

		case Camera_Perspective:
		{
			Camera->HorizontalFOV = HalfWidth_HorizontalFOV;
			Camera->AspectRatio = HalfHeight_AspectRatio;
			Camera->Near = Near;
			Camera->Far = Far;
		} break;

		InvalidDefaultCase;
	}
}

internal camera
DefaultScreenCamera(v2i ScreenResolution)
{
	camera Result = {};
	Result.Type = Camera_Orthogonal;
	v2 ScreenDim = V2((f32)ScreenResolution.x, (f32)ScreenResolution.y);
	v3 ScreenCamP = V3(0.5f*ScreenDim, 5.0f);
	f32 ScreenCamNear = 0.0f;
	f32 ScreenCamFar = 10.0f;
	f32 ScreenCamWidthOverHeight = ScreenDim.x/ScreenDim.y;
	f32 ScreenCamHalfHeight = 0.5f*ScreenDim.y;
	f32 ScreenCamHalfWidth = ScreenCamHalfHeight*ScreenCamWidthOverHeight;
	ChangeCameraSettings(&Result, ScreenCamHalfWidth, ScreenCamHalfHeight,
		ScreenCamNear, ScreenCamFar);
	MoveCamera(&Result, ScreenCamP);
	return Result;
}

internal void
CameraSetMatrices(camera *Camera)
{
	Camera->Rotation = NormalizeOrNoRotation(Camera->Rotation);
	Camera->View_ = ViewMat4(Camera->P, Camera->Rotation);
	Camera->InvView_ = InvViewMat4(Camera->P, Camera->Rotation);

	switch(Camera->Type)
	{
		case Camera_Orthogonal:
		{
			f32 HalfWidth = Camera->HalfWidth;
			f32 HalfHeight = Camera->HalfHeight;
			Assert(HalfWidth && HalfHeight);

			Camera->Projection_ = OrthogonalProjectionMat4(HalfWidth, HalfHeight, Camera->Near, Camera->Far);
		} break;

		case Camera_Perspective:
		{
			Camera->Projection_ = PerspectiveProjectionMat4(Camera->HorizontalFOV, Camera->AspectRatio, Camera->Near, Camera->Far);
		} break;

		InvalidDefaultCase;
	}
}

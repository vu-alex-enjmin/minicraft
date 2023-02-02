#ifndef __AVATAR__
#define __AVATAR__

#include "engine/utils/types_3d.h"
#include "engine/render/camera.h"
#include "engine/utils/timer.h"
#include "world.h"

#define GRAVITY -9.80665

class MAvatar
{
public:
	YVec3f Position;
	YVec3f Speed;

	bool Move;
	bool Jump;
	float Height;
	float CurrentHeight;
	float Width;
	bool avance;
	bool recule;
	bool gauche;
	bool droite;
	bool Standing;
	bool InWater;
	bool Crouch;
	bool Run;

	YCamera * Cam;
	MWorld * World;

	YTimer _TimerStanding;

	MAvatar(YCamera * cam, MWorld * world)
	{
		Position = YVec3f((MWorld::MAT_SIZE_METERS) / 2, (MWorld::MAT_SIZE_METERS) / 2, (MWorld::MAT_HEIGHT_METERS));
		Height = 1.8f;
		CurrentHeight = Height;
		Width = 0.6f;
		Cam = cam;
		avance = false;
		recule = false;
		gauche = false;
		droite = false;
		Standing = false;
		Jump = false;
		World = world;
		InWater = false;
		Crouch = false;
		Run = false;
	}

	void update(float elapsed)
	{
		if (elapsed > 1.0f / 60.0f)
			elapsed = 1.0f / 60.0f;

		YVec3f acceleration;
		acceleration.Z += GRAVITY * MCube::CUBE_SIZE;

		float horizontal = 0.0f;
		if (gauche)
			horizontal += -1.0f;
		if (droite)
			horizontal += 1.0f;

		float forward = 0.0f;
		if (recule)
			forward += -1.0f;
		if (avance)
			forward += 1.0f;

		float speed = 500.0f;
		YVec3f forwardVec = Cam->UpRef.cross(Cam->RightVec).normalize();
		YVec3f movement = ((forwardVec * forward) + (Cam->RightVec * horizontal)) * elapsed * speed;

		Speed.X = movement.X;
		Speed.Y = movement.Y;
		Speed += acceleration * elapsed;

		if (Jump)
		{
			Jump = false;
			float jumpHeight = 1.25;
			Speed.Z = std::sqrt(2 * jumpHeight * std::abs(GRAVITY) * MCube::CUBE_SIZE);
		}
		
		YVec3f newPosition = Position + Speed * elapsed;

		float valueColMin;
		float oneShotValueColMin;

		MWorld::MAxis axis = World->getMinCol(
			newPosition,
			Speed, Width, Height,
			valueColMin,
			false
		);

		MWorld::MAxis oneShotAxis = World->getMinCol(
			newPosition,
			Speed, Width, Height,
			oneShotValueColMin,
			true
		);

		if (axis == 0)
		{
			Position = newPosition;
			return;
		}


		valueColMin *= 1.01f;
		oneShotValueColMin *= 1.01f;
		YVec3f axisVec;

		if (oneShotAxis != 0)
		{
			if (axis == MWorld::AXIS_X)
				axisVec = YVec3f(oneShotValueColMin, 0, 0);
			else if (axis == MWorld::AXIS_Y)
				axisVec = YVec3f(0, oneShotValueColMin, 0);
			else if (axis == MWorld::AXIS_Z)
				axisVec = YVec3f(0, 0, oneShotValueColMin);
			if ((axis == MWorld::AXIS_Z) && (oneShotValueColMin > 0))
				Speed.Z = -1.0f;
		}
		else
		{
			if (axis == MWorld::AXIS_X)
				axisVec = YVec3f(valueColMin, 0, 0);
			else if (axis == MWorld::AXIS_Y)
				axisVec = YVec3f(0, valueColMin, 0);
			else if (axis == MWorld::AXIS_Z)
				axisVec = YVec3f(0, 0, valueColMin);
			if ((axis == MWorld::AXIS_Z) && (valueColMin > 0))
				Speed.Z = -1.0f;
		}
		
		axis = World->getMinCol(
			newPosition + axisVec,
			Speed, Width, Height,
			valueColMin,
			false
		);
		
		if (axis == 0)
		{
			Position = newPosition + axisVec;
			return;
		}

		for (int i = 0; i < 100; i++)
		{
			axis = World->getMinCol(
				newPosition,
				Speed, Width, Height,
				valueColMin,
				false
			);

			if (axis == 0)
				break;
			
			valueColMin *= 1.01f;
			axisVec = YVec3f();
			if (axis == MWorld::AXIS_X)
				axisVec = YVec3f(valueColMin, 0, 0);
			else if (axis == MWorld::AXIS_Y)
				axisVec = YVec3f(0, valueColMin, 0);
			else if (axis == MWorld::AXIS_Z)
				axisVec = YVec3f(0, 0, valueColMin);
			if ((axis == MWorld::AXIS_Z) && (valueColMin > 0))
				Speed.Z = -1.0f;

			newPosition += axisVec;
		}
		if (axis == 0)
			Position = newPosition;
	}
};

#endif
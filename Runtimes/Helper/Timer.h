// Meso Engine 2024
#pragma once

#include <lvk/LVK.h>

class FTimer
{
private:
	double StartTime;
public:
	FTimer()
	{
		Start();
	}
	double Start()
	{
		StartTime = glfwGetTime();
		return StartTime;
	}
	double Step(bool bOverride = true)
	{
		if (bOverride)
		{
			double StartTimeOld = StartTime;
			StartTime = glfwGetTime();
			return StartTime - StartTimeOld;
		}
		else
		{
			return glfwGetTime() - StartTime;
		}
	}
};
#pragma once

#include "Timer.h"
#include <map>
#include <lvk/LVK.h>

class FTimerSet
{
private:
	std::map<std::string, double> StartTime;
	std::map<std::string, double> TempTime;
	std::map<std::string, double> TotalTime;
	std::map<std::string, uint32_t> TotalRecordCount;
public:
	void Start(const std::string& Key)
	{
		if (TotalRecordCount.find(Key) == TotalRecordCount.end())
		{
			TotalRecordCount[Key] = 0;
			TotalTime[Key] = 0.0;
			TempTime[Key] = 0.0;
		}
		StartTime[Key] = glfwGetTime();
	}
	void Record(const std::string& Key, uint32_t RecordTimes = 1)
	{
		double EndTime = glfwGetTime();
		TotalTime[Key] += EndTime - StartTime[Key];
		TotalRecordCount[Key] += RecordTimes;
		StartTime[Key] = glfwGetTime();
	}
	void Stage(const std::string& Key)
	{
		double EndTime = glfwGetTime();
		TempTime[Key] = EndTime - StartTime[Key];
		StartTime[Key] = glfwGetTime();
	}
	void RecordStage(const std::string& Key, uint32_t RecordTimes = 1)
	{
		TotalTime[Key] += TempTime[Key];
		TotalRecordCount[Key] += RecordTimes;
		TempTime[Key] = 0.0;
		StartTime[Key] = glfwGetTime();
	}
	double GetAverage(const std::string& Key)
	{
		if (TotalRecordCount.find(Key) == TotalRecordCount.end())
		{
			return 0.0;
		}
		else
		{
			return TotalTime[Key] / (double)std::max(1u, TotalRecordCount[Key]);
		}
	}
};
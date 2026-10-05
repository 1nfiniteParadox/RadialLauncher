#pragma once

#include <string>
#include <vector>

struct RadialProfile
{
    std::string name;

    int optionCount;

    int launcherSize;

    double centerDeadZone;

    double innerRegionSize;

    double itemDistance;

    std::vector<std::string> appPaths;
};

extern RadialProfile activeProfile;
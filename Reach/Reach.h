#pragma once
#include <Windows.h>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
#define MINECRAFT_TICK 600
#define MINECRAFT_HANDLE checkMinecraftHandle()

#pragma comment(lib,"winmm.lib")

void ReachScanner();

void ChangeReach(int min_reach, int max_reach, int chance, bool while_sprinting);
#include "PerlinNoise.hpp"

constexpr siv::PerlinNoise::seed_type seed = 1223u;
constexpr unsigned int SCR_WIDTH = 1280;
constexpr unsigned int SCR_HEIGHT = 768;
constexpr int renderDistance = 4;
constexpr int cleanupRadius = renderDistance + 2;

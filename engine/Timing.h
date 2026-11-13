#pragma once
namespace ECSEngine {
// AI-ASSISTED (Claude)
/// Fixed simulation step in seconds; matches the 60 FPS cap in ECSEngine::Run().
constexpr float kFixedDt = 1.0f / 60.0f;
}
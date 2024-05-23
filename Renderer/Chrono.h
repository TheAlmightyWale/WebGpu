#include <Chrono>
namespace Clock
{
	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::time_point<Clock>;
	using Duration = std::chrono::nanoseconds;

	constexpr float k_nanosecondsInSecond = 1'000'000'000;

	namespace {
		Clock g_clock;
		TimePoint lastFrameTime = g_clock.now();
		Duration deltaTime;
	}

	static void Tick()
	{
		auto nowTime = g_clock.now();
		deltaTime = nowTime.time_since_epoch() - lastFrameTime.time_since_epoch();
		lastFrameTime = nowTime;
	}

	static float GetDelta()
	{
		return (float)deltaTime.count() / k_nanosecondsInSecond;
	}
}



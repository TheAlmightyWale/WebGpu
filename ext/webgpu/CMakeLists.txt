include(FetchContent)

FetchContent_Declare(
	webgpu
	GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
	GIT_TAG b763f12
)

FetchContent_MakeAvailable(webgpu)
include(FetchContent)

FetchContent_Declare(
	webgpu
	GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
	GIT_TAG main
)

FetchContent_MakeAvailable(webgpu)
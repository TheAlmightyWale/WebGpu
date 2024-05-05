#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
//disable info strings in release
#ifdef RELEASE
#define STBI_NO_FAILURE_STRINGS
#else
#define STBI_FAILURE_USERMSG
#endif // DEBUG

#pragma warning( push )                    // Save the current warning state.
#pragma warning( disable : 4505 )          // 4505: unreferenced function with internal linkage has been removed
#include <stb_image.h>
#pragma warning( pop )
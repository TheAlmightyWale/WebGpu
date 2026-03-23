#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
//disable info strings in release
#ifdef RELEASE
#define STBI_NO_FAILURE_STRINGS
#else
#define STBI_FAILURE_USERMSG
#endif // DEBUG

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#elif WIN32
#pragma warning( push )                    // Save the current warning state.
#pragma warning( disable : 4505 )          // 4505: unreferenced function with internal linkage has been removed
#endif

#include <stb_image.h>
#ifdef WIN32
#pragma warning( pop )
#elif __GNUC__
#pragma GCC diagnostic pop
#endif
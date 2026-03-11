# Fixed-Width Integer Types Fix Plan

## Goal Description
Fix build errors in `direct_camera_test.cpp` and `direct_media_test.cpp` caused by undefined `uint32_t`, `int32_t`, etc., in `bionic` headers, likely due to `libc++`'s `stdint.h` shadowing or misconfiguration in the Android 15 build environment.

## Proposed Changes
### External Libhybris
#### [MODIFY] [direct_camera_test.cpp](file:///media/jimmykamanga/fe25dd70-1428-4fc6-b925-53de8484505a/home/kaliuser/android/external/libhybris/compat/camera/direct_camera_test.cpp)
- Insert typedefs for `uint32_t`, `int32_t`, `uint64_t`, `int64_t` using Clang/GCC built-in macros (`__UINT32_TYPE__`, etc.) before `#include <sys/types.h>`.
- This ensures types are defined regardless of `stdint.h` resolution issues.

#### [MODIFY] [direct_media_test.cpp](file:///media/jimmykamanga/fe25dd70-1428-4fc6-b925-53de8484505a/home/kaliuser/android/external/libhybris/compat/media/direct_media_test.cpp)
- Apply the same typedef fix as above.
- Also remove the problematic `#define __STDC...` macros if they exist (they were not present in the error log for media test, but good to check/ensure consistency).

## Verification Plan
### Automated Tests
- The user will run the build again.
- Success is defined by the compilation of these modules passing without "unknown type name" errors.

### Manual Verification
- Review the code to ensure `typedef`s are guarded or placed correctly to avoid conflicts if `stdint.h` simply starts working later.

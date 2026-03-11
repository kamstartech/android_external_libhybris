# Libhybris Build Fix Task List

- [x] Fix build errors in `direct_camera_test` and `direct_media_test`
    - [x] Resolve missing integer type definitions (`uint32_t`, `intptr_t`, etc.) in `bionic` headers
        - [x] Add fallback typedefs in `direct_camera_test.cpp`
        - [x] Add fallback typedefs in `direct_media_test.cpp`
    - [x] Fix header shadowing issues
        - [x] Remove conflicting `external/libcxx/include` from `compat/camera/Android.mk`
        - [x] Remove conflicting `external/libcxx/include` from `compat/media/Android.mk`
    - [x] Fix C++ compiler errors (strict warnings)
        - [x] Update string literal usage (`char*` -> `const char*`) in `direct_camera_test.cpp`
        - [x] Fix designated initializer syntax in `direct_camera_test.cpp`
    - [x] Resolve linker errors
        - [x] Add `liblog` to `LOCAL_SHARED_LIBRARIES` in `compat/camera/Android.mk`
        - [x] Add `liblog` to `LOCAL_SHARED_LIBRARIES` in `compat/media/Android.mk`
- [x] Verify successful build
    - [x] Check for binaries on device
    - [x] Execute `direct_media_test` (passes launch check)
    - [x] Execute `direct_camera_test` (passes launch check)

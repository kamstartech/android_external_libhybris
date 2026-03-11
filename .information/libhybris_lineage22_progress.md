# Libhybris Porting Progress - Lineage 22 (Android 15)

## Overview
Recent activity in `external/libhybris` indicates an active effort to port libhybris to Android 15 (Lineage 22). The work focuses on modernizing the compatibility layers to match updated Android C++ APIs and ABIs.

## Component Status

### SurfaceFlinger (`compat/surface_flinger`)
**Status:** Major Updates
- **Display Identification**: Moved from legacy `getBuiltInDisplay` (0/1) to `getPhysicalDisplayIds`.
- **Display Info**: Replaced `DisplayInfo` with `StaticDisplayInfo` and `DynamicDisplayInfo` to retrieve resolution and density.
- **Power Management**: Updated to use `setDisplayPowerMode` instead of the obsolete `unblankDisplay`.
- **Layer Stack**: Updated to use `android::ui::LayerStack::fromValue(0)`.

### Camera (`compat/camera`)
**Status:** Major Updates
- **Attribution**: Implemented `AttributionSourceState` (required by Android 12+ and enforced in 15) for `Camera::connect` and `getNumberOfCameras`.
- **String Handling**: Updated `String8` usage from `.string()` to `.c_str()`.

### Gralloc (`hybris/gralloc`)
**Status:** Stubs / Workaround
- **UI Symbols**: Several UI initialization and allocation functions (`hybris_ui_initialize`, `graphic_buffer_allocator_allocate`) strictly stubbed to return `-ENOSYS` or do nothing.
- **Logging**: Switched to direct `android/log.h` usage.
- **Observation**: This suggests that the legacy Gralloc path is being disabled or bypassed, possibly relying on a different mechanism for buffer allocation or just getting it to link for now.

### Media (`compat/media`)
**Status:** Minor Updates
- **String Handling**: Simple updates (`.string()` -> `.c_str()`) to fix compilation errors.

### Build System
- **New File**: `Android.mk` added to the root, likely to facilitate building the compat layers within the LineageOS build environment.

## Summary of Changes
- **24+ modified files** across compatibility layers.
- **Focus**: Resolving compilation errors due to API removals and signature changes in Android 15 framework headers.
- **Next Steps**: Functional verification is likely needed, especially for the Gralloc stubs and the new Display identification logic.

## Build Error Resolution (Tests)
### Issues Resolved
- **Compilation Failures**: `direct_camera_test.cpp` and `direct_media_test.cpp` failed due to missing fixed-width integer types (`uint32_t`, `intptr_t`, etc.) in `bionic` headers.
- **Header Conflicts**: `Android.mk` files manually added `external/libcxx/include`, causing `libc++` headers to resolve incorrectly and shadow system definitions.
- **Strict Compiler Checks**: `-Werror` flagged deprecated string conversions and GNU-style designated initializers in C++ code.
- **Linker Errors**: Undefined reference to `__android_log_print`.

### Fixes Applied
- **Type Definitions**: Added manual fallback `typedef`s for `uint32_t`, `intptr_t`, etc., using compiler built-in macros.
- **Makefile Cleanup**: Removed redundant include paths (`external/libcxx/include`, `bionic`, etc.) from `compat/camera/Android.mk` and `compat/media/Android.mk` to fix header shadowing.
- **Code Modernization**: Updated `char *` to `const char *` and converted struct initializers to C99 syntax (`.field = value`).
- **Dependencies**: Added `liblog` to `LOCAL_SHARED_LIBRARIES` to resolve linker errors.

### Outcome
- `direct_camera_test` and `direct_media_test` executables now build successfully.

[![bugsplat-github-banner-basic-outline](https://user-images.githubusercontent.com/20464226/149019306-3186103c-5315-4dad-a499-4fd1df408475.png)](https://bugsplat.com)
<br/>

# <div align="center">BugSplat</div>

### **<div align="center">Crash and error reporting built for busy developers.</div>**

<div align="center">
    <a href="https://bsky.app/profile/bugsplatco.bsky.social"><img alt="Follow @bugsplatco on Bluesky" src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor%3Dbugsplatco.bsky.social&query=%24.followersCount&style=social&logo=bluesky&label=Follow%20%40bugsplatco.bsky.social"></a>
    <a href="https://discord.gg/bugsplat"><img alt="Join BugSplat on Discord" src="https://img.shields.io/discord/664965194799251487?label=Join%20Discord&logo=Discord&style=social"></a>
</div>

<br/>

# Crashpad prebuilt farm (used by bugsplat-native)

`.github/workflows/crashpad.yml` builds Crashpad at **one pinned commit** (`CRASHPAD_COMMIT_PINNED`)
for every platform [bugsplat-native](https://github.com/BugSplat-Git/bugsplat-native) ships on and
publishes the result as a GitHub pre-release tagged `crashpad-<commit7>`:

| artifact | runner / toolchain | notes |
|---|---|---|
| `windows-x64`, `windows-x86`, `windows-arm64` | windows-2022, Crashpad's clang-cl against the runner's MSVC STL, `/MD` | consumers need MSVC toolset >= 14.44; includes `crashpad_wer` and the WER helper source |
| `macos-universal` | macos-15 / Xcode 16, arm64 + x86_64 lipo'd | `mac_deployment_target=12.0` (the floor: `kIOMainPortDefault`) |
| `linux-x86_64`, `linux-aarch64` | ubuntu-24.04 / ubuntu-24.04-arm, clang 18 | `crashpad_http_transport_impl=socket`, no libcurl |
| `android-arm64-v8a`, `android-armeabi-v7a`, `android-x86_64` | ubuntu-24.04, the image's NDK (recorded in `PREBUILT.json`) | `android_api_level=26`; 32-bit ARM needs a one-line GN `not_needed()` workaround that `scripts/farm/build.sh` applies and `PREBUILT.json` records |
| `ios-device`, `ios-simulator`, `tvos-device`, `tvos-simulator` | macos-15 / Xcode 16; simulators are arm64 + x86_64 | in-process client libraries only, `ios_deployment_target=14.0` |

Each `crashpad-<commit7>-<artifact>.tar.xz` mirrors a Crashpad checkout, so a consumer's build
glue is identical for a prebuilt and a GN build: public headers (`client/`, `util/`, `snapshot/`,
`minidump/`, `handler/`, `tools/`, `compat/`, `build/`, mini_chromium `base/` + `build/`, lss),
`handler/win/wer/crashpad_wer.cc`, `out/release/obj/**` static libraries plus the `tool_support`
object, `out/release/gen/**`, the reference `crashpad_handler`, `args.gn`, `LICENSE` and
`PREBUILT.json` (commit, GN args, toolchain, any build-config patch). Every tarball ships with a
`.sha256`; the release job re-verifies all of them and writes `crashpad.lock` (commit, per-artifact
file/sha256/size/url), which bugsplat-native pins in `cmake/crashpad.lock` and consumes through
`cmake/FetchCrashpadPrebuilt.cmake`. The lock of the current pin is also committed here as
[`crashpad.lock`](./crashpad.lock).

**Bumping Crashpad** (quarterly, or for a Crashpad security fix): change `CRASHPAD_COMMIT_PINNED`
(or pass `crashpad_commit` to *Run workflow*), run the workflow, check the pre-release, copy its
`crashpad.lock` into bugsplat-native and this repo, let bugsplat-native's `prebuilt-smoke`
workflow prove the desktop platforms, then promote the pre-release. Pull requests touching the
workflow or `scripts/farm/**` build everything without publishing.

The scripts are reusable by hand: `scripts/farm/build.sh <work> <commit> <out> "<args.gn>" <targets>`
(`build.ps1` on Windows), `scripts/farm/lipo.sh` to merge two out dirs, and
`scripts/farm/package.sh <crashpad> <out> <artifact> <dest>` to produce the tarball and its checksum.

# MyCMakeCrasher Project

A cross-platform application demonstrating Crashpad integration with CMake.

## About ℹ️

This project demonstrates how to:
- Set up a CMake project for cross-platform development
- Integrate Google's Crashpad library for crash reporting
- Create a simple crashing application that generates crash reports
- Configure a WER callback for catching stack buffer overruns and fail fast exceptions (Windows only)

## Prerequisites ☑️

- CMake (version 3.10 or higher)
- A C++ compiler (gcc, clang, MSVC, etc.)
- Install [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up) - Google's tools for working with Chromium source code
- A BugSplat account and database (sign up at [bugsplat.com](https://www.bugsplat.com))

## Configuration ⚙️

### BugSplat Setup

1. Sign up for a BugSplat account at [bugsplat.com](https://www.bugsplat.com) if you haven't already
2. Create a new database in your BugSplat dashboard
3. Open `main.h` and define your database name:
   ```cpp
   #define BUGSPLAT_DATABASE "your-database-name"  // Replace with your database name from BugSplat
   ```

### Installing depot_tools

1. Clone the depot_tools repository:

```bash
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
```

2. Add depot_tools to your PATH:

```bash
# For Linux/macOS - add to your .bashrc or .zshrc
export PATH="$PATH:/path/to/depot_tools"
```

```powershell
# For Windows - add to system environment variables or use the following powershell commands:
$env:Path = "C:\path\to\depot_tools;$env:Path"
[Environment]::SetEnvironmentVariable("PATH", $env:PATH, "User")
```

3. For Windows, you also need to run:

```powershell
# From the depot_tools directory
gclient
```

### Symbol Uploads

Symbol uploads must be configured so that crash reports contain function names, file names, and line numbers. The symbol upload process uses BugSplat's [symbol-upload](https://github.com/BugSplat-Git/symbol-upload) utility, which is automatically downloaded as needed.

On Windows, symbol-upload-windows.exe will search for `.pdb` files, on macOS symbol-upload-macos will search for `.dSYM` files, and on Linux, symbol-upload-linux will search for `.debug` files. All files are automatically converted to Crashpad/Breakpad compatible `.sym` files before uploading.

To configure symbol upload, ensure you have:

1. Defined your BugSplat database name in `main.h` as described in the [BugSplat Setup](#bugsplat-setup) section
2. Set up your BugSplat API credentials (`BUGSPLAT_CLIENT_ID` and `BUGSPLAT_CLIENT_SECRET`) in the `.env` file. You can generate a Client ID/Client Secret pair on the [Integrations](https://app.bugsplat.com/v2/database/integrations#oauth) page.

## Building the Project 🏗️

The build scripts will automatically fetch and build Crashpad using `depot_tools`, then build the main application using CMake.

### macOS

```bash
# Execute the build script
./scripts/build_macos.sh

# Run the application
./build/Debug/MyCMakeCrasher
```

### Linux

```bash
# Execute the build script
./scripts/build_linux.sh

# Run the application
./build/Debug/MyCMakeCrasher
```

### Windows

```powershell
# Execute the build script
.\scripts\build_windows_msvc.ps1

# Run the application
.\build\Debug\MyCMakeCrasher.exe
```

## Testing Crash Reporting 🧪

> [!NOTE]
> To configure your Windows app to catch stack buffer overruns please see the [WER](https://github.com/BugSplat-Git/bugsplat-crashpad/wiki/WER) page in this repo's Wiki.

The application will crash immediately upon launch to demonstrate the crash reporting functionality. Crashes will be automatically uploaded to BugSplat. You can test various types of crashes by commenting/uncommenting calls to `loadCrashFunction` in [main.cpp](./main.cpp). 

```cpp
// ========================================
// CRASH TYPE SELECTION
// ========================================
// Uncomment ONE of the following crash types to test different scenarios:

// 1. NULL POINTER DEREFERENCE
crash_func_t crash_func = loadCrashFunction("crash");

// 2. ACCESS VIOLATION
// crash_func_t crash_func = loadCrashFunction("crashAccessViolation");

// 3. STACK OVERFLOW
// crash_func_t crash_func = loadCrashFunction("crashStackOverflow");

// 4. STACK BUFFER OVERRUN (WER callback required for Windows see https://github.com/BugSplat-Git/bugsplat-crashpad/wiki/WER)
// crash_func_t crash_func = loadCrashFunction("crashStackOverrun");
```

Once you've generated a crash, you can view the report on the [Dashboard](https://app.bugsplat.com/v2/dashboard) page. Be sure to verify the correct database is selected in the dropdown.

<img width="1728" alt="BugSplat Dashboard" src="https://github.com/user-attachments/assets/36572a23-991d-416b-8bdb-bb3627c803cb" />

Click the value in the `ID` column to see the report's stack trace and associated metadata.

<img width="1728" alt="image" src="https://github.com/user-attachments/assets/da7bfbbb-7340-46ad-8d33-5e6ef03052e7" />

Thanks for using BugSplat!

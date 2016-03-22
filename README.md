# OSVR-Vive
> Maintained at <https://github.com/OSVR/OSVR-Vive>
>
> For details, see <http://osvr.github.io>

This is a plugin for OSVR that provides access to the tracker data on HTC Vive HMDs and controllers. It also contains tools to extract a display descriptor with distortion mesh data from a Vive, as well as additional tools that build from source but are not installed into binary snapshots.

## Compiling

To compile, this project requires OSVR, Eigen, and Boost, as well as the submodules included in the repository (clone with `git clone --recursive`). Compile as you would other CMake-based projects, setting `CMAKE_PREFIX_PATH` to show the way to dependencies in general. You may need to set `EIGEN3_INCLUDE_DIR` specifically.

You may also use a pre-compiled set of binaries from the project.

## Installation and Usage
Once built and installed (or binaries downloaded), you'll have files as follows:
- A plugin file - on Windows, this is a `.dll` file in something like `bin/osvr-plugins-0`. Put that file in the same directory of your OSVR server as the other plugins.
- In `bin`, a `ViveDisplayExtractor` tool - copy to the `bin` directory of your OSVR server.
- A sample config file.

To run, you'll need to have first:

- One time setup: Installed SteamVR and run the Room Setup procedure successfully.
- Each time you want to run an OSVR application, in the SteamVR menu, disable "Direct Mode" then exit SteamVR.

One time setup: You'll need to generate a custom display descriptor and mesh distortion data file for your Vive using the included tool. Making sure that `ViveDisplayExtractor` is alongside `osvr_server` and that your Vive is plugged in, run `ViveDisplayExtractor`. When successful, you'll see something like:

```
[DisplayExtractor] Writing distortion mesh data file:
C:/Users/Ryan/Desktop/OSVR/bin/displays/HTC_Vive_PRE_meshdata.json

[DisplayExtractor] Writing display descriptor file:
C:/Users/Ryan/Desktop/OSVR/bin/displays/HTC_Vive_PRE.json

[DisplayExtractor] Press enter to quit...
```


## Licenses
This plugin: Licensed under the Apache License, Version 2.0.

Vendored projects:
- `boost-process` (incubator/under review) - Boost Software License 1.0
- Valve SteamVR `openvr` (specifically `openvr_driver` headers) - MIT license.

Note: At runtime, this plugin dynamically loads the Lighthouse SteamVR plugin distributed with SteamVR, runs the `vrpathreg` tool included with SteamVR and parses its output, as well as loads some SteamVR configuration settings (room calibration, etc) from JSON files.

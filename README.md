# OSVR-Vive
> Maintained at <https://github.com/OSVR/OSVR-Vive>
>
> For details, see <http://osvr.github.io>

This is a plugin for OSVR that provides access to the tracker data on HTC Vive HMDs and controllers.

Then, just put the `.dll` file from this plugin in the same directory of your OSVR server as the other plugins - usually something like `osvr-plugins-0` - and use the config file provided here as an example, along with the two display descriptor files in the `displays` directory.

This project also contains a number of related tools for extracting information about the Vive from the driver, such as the data used to generate the display descriptor and distortion mesh file.

## Licenses
This plugin: Licensed under the Apache License, Version 2.0.

Vendored projects:
- `boost-process` (incubator/under review) - Boost Software License 1.0
- Valve SteamVR `openvr` (specifically `openvr_driver` headers) - MIT license.

Note: At runtime, this plugin dynamically loads the Lighthouse SteamVR plugin distributed with SteamVR, runs the `vrpathreg` tool included with SteamVR and parses its output, as well as loads some SteamVR configuration settings (room calibration, etc) from JSON files.

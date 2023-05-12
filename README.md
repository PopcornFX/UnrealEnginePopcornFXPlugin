# Unreal Engine PopcornFX Plugin

Integrates the **PopcornFX Runtime SDK** into **Unreal Engine 4** and **Unreal Engine 5** as a Plugin.
* **Version:** `v2.16.2`
* **Unreal Engine:** `4.27`, `5.0`, `5.1` and `5.2`
* **Supported platforms:** `Windows`, `MacOS`, `Linux`, `iOS`, `Android`, `PS4`, `PS5`, `XboxOne`, `Xbox Series`, `Switch`

[Contact-us](http://www.popcornfx.com/contact-us/) to request access to the plugin for consoles.

## Setup

The plugin can only be installed in Code projects (Blueprint-only projects are not supported).

On windows, it is required to install **[Visual studio](https://docs.unrealengine.com/4.27/en-US/ProductionPipelines/DevelopmentSetup/VisualStudioSetup/)** to compile the plugin as we do not provide compiled PopcornFX plugin binaries yet.

### Installing from downloaded archive

Simply extract the archive in a project's `Plugins/` folder.

The plugin can also be extracted in an engine installation's `Plugins/` folder: if the engine is downloaded instead of being compiled from source, the plugin will need to be compiled manually first (ie. in a separate dummy UE project).

Check out the **[General installation steps](https://www.popcornfx.com/docs/popcornfx-v2/plugins/ue4-plugin/installation-and-setup/)** to see how to compile the plugin!

### Installing from GitHub source repository

Currently, you will need to download & install **[7-zip](https://www.7-zip.org/download.html)** and add it to your PATH environment variable (this is a requirement we want to remove in future updates): we currently use 7zip as it produces smaller archives of the PopcornFX Runtime SDK.

Once Visual studio and 7-zip are installed, you can launch `Download_SDK_All.bat` to download PopcornFX Runtime SDK. `Download_SDK_Desktop.bat` and `Download_SDK_Mobile.bat` can be used separately if you only need Desktop or Mobile platforms.

Once PopcornFX Runtime SDK is downloaded and placed in `PopcornFX_Runtime_SDK/` folder, the plugin is ready to be compiled.

Check out the **[General installation steps](https://www.popcornfx.com/docs/popcornfx-v2/plugins/ue4-plugin/installation-and-setup/)** to see how to compile the plugin!

## Quick Links: Documentation and Support

* [**Plugin** documentation](https://www.popcornfx.com/docs/popcornfx-v2/plugins/ue4-plugin/) (Install, Import, Setup, Troubleshooting, etc..)
* [PopcornFX **Editor** documentation](https://www.popcornfx.com/docs/popcornfx-v2/)
* [PopcornFX **Discord** server](https://discord.gg/4ka27cVrsf)

## License

See [LICENSE.md](/LICENSE.md).

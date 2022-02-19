Unreal ImGui
============
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 4 plug-in that integrates [Dear ImGui](https://github.com/ocornut/imgui) developed by Omar Cornut.

Dear ImGui is an immediate-mode graphical user interface library that is very lightweight and easy to use. It can be very useful when creating debugging tools.

:stop_button: Read Me First
---------------------------
Please note that this is a forked project from [https://github.com/segross/UnrealImGui](https://github.com/segross/UnrealImGui). I do not take credit for the work he's put into making Dear ImGui work in Unreal Engine. The only work I've done on this forked version is to update it to work with UE5, updated Dear ImGui and to add ImPlot into it (which are also listed below for brevity).

I've removed large portions of this readme.md to keep redundant information between the base project and this fork to a minimum. If you wish to read the original readme.md, please see this link: [UnrealImGui ReadMe.md](https://github.com/segross/UnrealImGui/blob/master/README.md).

Fork Additions
--------------
 - Updated core source files for Unreal Engine 5, new ImGui and ImPlot
 - Updated ImGui to 1.87
 - Added ImPlot v0.13 WIP

Status
------
UnrealImGui Version: 1.22

ImGui version: 1.87

ImPlot version: v0.13 WIP

Supported Unreal Engine version: 5.0*

\* *The original repository has support for later versions of UE4. I've not tested this fork on UE4 variants, I only know it works for UE5 currently.*

How to Set up
-------------
On top of reading the base repository's [How to Set up](https://github.com/segross/UnrealImGui/blob/master/README.md#how-to-set-up) segment, you'll need to add the following line to your `[GameName].Build.cs` file otherwise you'll get linking errors:

```cpp
// Tell the compiler we want to import the ImPlot symbols when linking against ImGui plugin 
PrivateDefinitions.Add(string.Format("IMPLOT_API=DLLIMPORT"));
```

Using ImPlot
------------
It's pretty easy to use ImPlot, it's pretty much the same drill as using Dear ImGui with the UnrealImGui plugin. You can see documentation on how to use ImPlot here: [ImPlot](https://github.com/epezent/implot).

The only thing you won't need to do is call the `ImPlot::CreateContext()` and `ImPlot::DestroyContext` routines as they're already called when ImGui's context is created within UnrealImGui's guts.

See also
--------
 - [Original Project by segross](https://github.com/segross/UnrealImGui)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [ImPlot](https://github.com/epezent/implot)


License
-------
Unreal ImGui (and this fork) is licensed under the MIT License, see LICENSE for more information.

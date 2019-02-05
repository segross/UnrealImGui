Unreal ImGui
============

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 4 plug-in which integrates [Dear ImGui](https://github.com/ocornut/imgui) framework developed by Omar Cornut.

Dear ImGui simplifies and helps with creation of quality visualisation and debugging tools for your Unreal projects.


Status
------
ImGui version: 1.65

Supported engine version: 4.21*

\* *Plugin has been tested and if necessary updated to compile and work with this engine version. As long as possible I will try to maintain backward compatibility of existing features and possibly but not necessarily when adding new features. Right now it should be at least backward compatible with the engine version 4.15.*


About
-----

This plug-in adds to Unreal project ImGui module. Adding it as as a dependency to other modules enables them to use Dear ImGui framework.

The base aim of the project is to provide a basic integration of Dear ImGui, without imposing any patters how it should be used in projects.

This is a work in progress but it supports key Unreal features, like Multi-PIE sessions etc.

When running Multi-PIE session, each world gets its own ImGui context to draw world specific data. When world update begins contexts are switched automatically, so using ImGui during objects update should be as easy as calling ImGui API functions.

For scenarios where automatic context switching above is not enough I'm planning to add mechanism allowing to explicitly select contexts. After that I plan to add example project, more usability features, better documentation and integration of Remote ImGui which enables using ImGui from a browser and to investigate possibility of opening ImGui for Blueprints.


How to use it
-------------

To use this plug-in, you will need a C++ Unreal project.

Content of this repository needs to be placed in *Plugins* directory under project's root: */Plugins/ImGui*. After you compile and run you should notice that *ImGui* module is now available.

To use that in other modules you will need to declare it as a public or private dependency in those modules' Build.cs files:

```
PublicDependencyModuleNames.AddRange(new string[] { "ImGui" });
```
or

```
PrivateDependencyModuleNames.AddRange(new string[] { "ImGui" });
```

You should now be able to use ImGui.


### Console commands

Console support evolved from using console variables to plain variables combined with interface and console commands to toggle them. This means that it is possible to bind keys to those commands (currently only one is supported natively) and call interface functions from user code (also in shipping builds, if needed).

- `ImGui.ToggleInput` - Toggle ImGui input mode (disabled by default). If enabled, input is routed to ImGui and with a few exceptions is consumed. It is possible to assign keyboard short-cut to this command trough ImGui properties panel.
- `ImGui.ToggleKeyboardNavigation` - Toggle ImGui keyboard navigation feature (disabled by default but will be replaced with configurable setting).
- `ImGui.ToggleGamepadNavigation` - Toggle ImGui gamepad navigation feature (disabled by default but will be replaced with configurable setting).
- `ImGui.ToggleDemo` - Toggle visibility of the ImGui demo (disabled by default).

### Console variables

Self-debug functionality is based on console variable but it will be also replaced by a command available in developer mode.

- `ImGui.Debug.Widget` - Show self-debug for the widget that renders ImGui output. 0: disabled (default); 1: enabled.
Note: this console variable will be replaced with optional toggle command.


### Canvas Map Mode

When input mode is enabled, it is possible to activate *Canvas Map Mode* (better name welcomed) by pressing and holding `Left Shift` + `Left Alt` keys. In this mode it is possible to drag ImGui canvas and change its scale. It can be helpful to temporarily reach areas of canvas that otherwise would be inaccessible and to change what part of the canvas should be visible in normal mode.

In canvas map mode:
- **Mouse Wheel** - to zoom in and out.
- **Right Mouse Button** - to drag ImGui canvas (not available at maximum zoom out).
- **Middle Mouse Button** - to drag frame that represents part of the ImGui canvas that is visible in normal mode (only available after zooming out). To start dragging mouse needs to be in the centre of that frame.
- It is still possible to use remaining keys and gestures to use ImGui, but primary goal is to select part of the canvas visible in normal mode.
- Releasing `Left Shift` and/or `Left Alt` key switches back to normal mode and automatically sets scale to 1.


See also
--------

 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [An Introduction to UE4 Plugins](https://wiki.unrealengine.com/An_Introduction_to_UE4_Plugins).


License
-------

Unreal ImGui is licensed under the MIT License, see LICENSE for more information.

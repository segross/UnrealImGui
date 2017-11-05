Unreal ImGui
============

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 4 plug-in which integrates [Dear ImGui](https://github.com/ocornut/imgui) framework developed by Omar Cornut.

Dear ImGui simplifies and helps with creation of quality visualisation and debugging tools for your Unreal projects.


Status
------
Supported engine version: 4.18*

\* *Plugin has been updated to compile and work with at least this version of the engine. Commits updating plugin to newer version of the engine are tagged with this version's number. If newer engine version doesn't require any change then it want be tagged, so there is a chance that plugin works with version newer than officially supported. As long as possible I will try to maintain backward compatibility when updating to newer engine version (but not necessarily when adding new features). Right now plugin should be at least backward compatible to engine version 4.15.*

About
-----

This plug-in adds to Unreal project ImGui module. Adding it as as a dependency to other modules enables them to use Dear ImGui framework.

The base aim of the project is to provide a basic integration of Dear ImGui, without imposing any patters how it should be used in projects.

This is a work in progress but it supports key Unreal features, like Multi-PIE sessions etc.

When running Multi-PIE session, each world gets its own ImGui context where world specific data can be visualised. When world update begins contexts are switched automatically, so using ImGui during objects update should be as easy as calling ImGui API functions.

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


*Console variables:*

- **ImGui.InputEnabled** - Allows to enable or disable ImGui input mode. 0: disabled (default); 1: enabled, input is routed to ImGui and with a few exceptions is consumed. Note: this is going to be supported by a keyboard short-cut, but in the meantime ImGui input can be enabled/disabled using console.
- **ImGui.ShowDemo** - Show ImGui demo. 0: disabled (default); 1: enabled.
- **ImGui.Debug.Widget** - Show self-debug for the widget that renders ImGui output. 0: disabled (default); 1: enabled.


See also
--------

 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [An Introduction to UE4 Plugins](https://wiki.unrealengine.com/An_Introduction_to_UE4_Plugins).


License
-------

Unreal ImGui is licensed under the MIT License, see LICENSE for more information.

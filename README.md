Unreal ImGui
============

[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 4 plug-in that integrates [Dear ImGui](https://github.com/ocornut/imgui) developed by Omar Cornut.

Dear ImGui is an immediate-mode graphical user interface library that is very lightweight and easy to use. It can be very useful when creating debugging tools.

Status
------
Version: 1.17 WIP

ImGui version: 1.65

Supported engine version: 4.22*

\* *Plugin has been tested and if necessary updated to compile and work with this engine version. As long as possible I will try to maintain backward compatibility of existing features and possibly but not necessarily when adding new features. Right now it should be at least backward compatible with the engine version 4.15.*


About
-----

The main goal of this project is to provide a basic integration of Dear ImGui, avoiding custom extensions but with support for Unreal specific features, like Multi-PIE sessions.

### Key features

- Multi-PIE support with each world getting its own ImGui context.
- Automatic switching between different ImGui contexts.
- Delegates for functions with ImGui content.
- Support for Unreal textures.

How to Set up
-------------

To use this plug-in, you will need a C++ Unreal project.

### Installation

Content of this repository needs to be placed in the *Plugins* directory under the project root: *[Project Root]/Plugins/ImGui/*. After you compile and run you should notice that *ImGui* module is now available.

Note that plugins can be also placed in the engine directory *[UE4 Root]/Engine/Plugins/* but I didn't try it with this project.

### Setting up module dependencies

To use ImGui in other modules you need to add it as a private or public dependency in their Build.cs files:

```C#
PrivateDependencyModuleNames.Add("ImGui");
```

or

```C#
PublicDependencyModuleNames.Add("ImGui");
```

You might also want to use ImGui only in certain builds:

```C#
if (Target.Configuration != UnrealTargetConfiguration.Shipping)
{
	PrivateDependencyModuleNames.Add("ImGui");
}
```

### Conditional compilation

You can conditionally compile ImGui code by checking `IMGUI_API`:

```C++
#ifded IMGUI_API
#include <imgui.h>
#endif

// ... somewhere in your code
#ifded IMGUI_API
// ImGui code
#endif
```

Above code is fine but it requires wrapping include directives and does not follow the usual pattern used in Unreal. To improve that, you can add the following code to one of your headers (or a precompiled header, if you use one):

```C++
// ImGuiCommon.h
#pragma once

#ifdef IMGUI_API
#define WITH_IMGUI 1
#else
#define WITH_IMGUI 0
#endif // IMGUI_API

#if WITH_IMGUI
#include <imgui.h>
#endif // WITH_IMGUI
```

And then use it like this:

```C++
#include "ImGuiCommon.h"

// ... somewhere in your code
#if WITH_IMGUI
// ImGui code
#endif
```

How to use it
-------------

### Using ImGui in code

ImGui can be used from functions registered as ImGui delegates or directly in game code. However, if code is running outside of the game thread or is executed outside of the world update scope, delegates are a better choice.

Delegates have an additional advantage that their content will work also when game is paused.

### ImGui Delegates

To use ImGui delegates, include `ImGuiDelegates.h`.

There are two major types of ImGui delegates: world and multi-context. First are created on demand for every world and are cleared once that world becomes invalid. They are designed to be used primarily by worlds objects. In opposition, multi-context delegates are called for every updated world, so the same code can be called multiple times per frame but in different contexts.

Delegates are called typically during world post actor tick event but they have alternative versions called during world tick start. In engine versions that does not support world post actor tick, that is below 4.18, all delegates are called during world tick start.

Delegates are called in order that allows multi-context delegates to add content before and after world objects:
- multi-context early debug
- world early debug
- world update
- world debug
- multi-context debug.

> `FImGuiModule` has delegates interface but it is depreciated and will be removed soon. Major issue with that interface is that it needs a module instance, what can be a problem when trying to register static objects. Additional issue is a requirement to always unregister with a handle.

### Multi-context

In multi-PIE sessions each world gets its own ImGui context which is selected at the beginning of the world update. All that happens in the background and should allow debug code to stay context agnostic.

If your content is rendered in the wrong context, try using one of the [ImGui delegates](#imgui-delegates) that should be always called after the right context is already set in ImGui.

### Using Unreal textures

Unreal ImGui allows to register textures in order to use them in ImGui. To do that, include `ImGuiModule.h` and use `FImGuiModule` interface.

After registration you will get a texture handle, declared in `ImGuiTextureHandle.h`, that you need to pass to the ImGui API.

```C++
// Texture handle defined like this
FImGuiTextureHandle TextureHandle;

// Registration and update
TextureHandle = FImGuiModule::Get().RegisterTexture("TextureName", Texture);

// Release
FImGuiModule::Get().ReleaseTexture(TextureHandle);

// Find by name
TextureHandle = FImGuiModule::Get().FindTextureHandle("TextureName");

// Example of usage (it is implicitly converted to ImTextureID)
ImGui::Image(TextureHandle, Size);
```

### Input mode

Right after the start ImGui will work in render-only mode. To interact with it, you need to activate input mode either by changing `Input Enabled` [property](#properties) from code, using `ImGui.ToggleInput` [command](#console-commands) or with a [keyboard shortcut](#keyboard-shortcuts).

In input mode, ImGui will consume all input events. The reason behind the input mode and its default behaviour is a separation between debug and game inputs to prevent accidental changes in both layers.

#### Sharing input

It is possible to modify rules to share keyboard or gamepad inputs.

The default behaviour can be configured in [input settings](#input) and changed runtime using `Keyboard Input Shared` and `Gamepad Input Shared` [properties](#properties) or `ImGui.ToggleKeyboardInputSharing`and `ImGui.ToggleGamepadInputSharing` [commands](#console-commands).

>*More work is needed for mouse input. Originally I didn't plan this feature so most probably I will come back to it after refactoring input handling. Ideally, I would like something that is more customizable from code with potentially a few implementations rather than one implementation with growing number of properties. In the meantime, if more control is needed, then `SImGuiWidget` is a good place to look at.*

#### Keyboard and gamepad navigation

When using ImGui on consoles you most probably need to enable keyboard and/or gamepad navigaiton. Both are ImGui features that allow to use it without mouse. See ImGui documentation for more details.

You can toggle those features by changing `Keyboard Navigation` and `Gamepad Navigation` [properties](#properties) or using `ImGui.ToggleKeyboardNavigation`and `ImGui.ToggleGamepadNavigation` [commands](#console-commands).

#### Navigating around ImGui canvas

Most of the time ImGui canvas will be larger from the viewport. When ImGui is in the input mode, it is possible to change which part of the canvas should be visible on the screen. To do that press and hold `Left Shift` + `Left Alt` and use mouse to adjust.

- `Mouse Wheel` - Zoom in or out.
- `Right Mouse Button` - Drag canvas with ImGui content.
- `Middle Mouse Button` - Drag frame representing which part of the canvas will be visible after ending the navigation mode.

The orange rectangle represents a scaled viewport and the area that will be visible on the screen after releasing keys. Zooming will scale that box and dragging with the middle mouse button will move everything.

The black rectangle represents a canvas border. You can drag canvas using the right mouse button. Dragging canvas will add a render offset between ImGui content and the viewport.

> Image(s) needed.

### Properties

ImGui module has a set of properties that allow to modify its behaviour:

- `Input Enabled` - Whether ImGui should receive [input](#input-mode). It is possible to assign a [keyboard shortcut](#keyboard-shortcuts) to toggle this property.
- `Keyboard Navigation`- Whether ImGui [keyboard navigation feature](#keyboard-and-gamepad-navigation) is enabled.
- `Gamepad Navigation` - Whether ImGui [gamepad navigation feature](#keyboard-and-gamepad-navigation) is enabled.
- `Keyboard Input Shared` - Whether keyboard input should be [shared with the game](#sharing-input). Default behaviour can be configured in [input settings](#input).
- `Gamepad Input Shared` - Whether gamepad input should be [shared with the game](#sharing-input). Default behaviour can be configured in [input settings](#input).
- `Show Demo` - Whether to show ImGui demo.

All properties can be changed by corresponding [console commands](#console-commands) or from code.

You can get properties interface through the module instance:

```C++
FImGuiModule::Get().GetProperties();
```

### Console commands

- `ImGui.ToggleInput` - Toggle ImGui input mode. It is possible to assign a [keyboard shortcut](#keyboard-shortcuts) to this command.
- `ImGui.ToggleKeyboardNavigation` - Toggle ImGui keyboard navigation.
- `ImGui.ToggleGamepadNavigation` - Toggle ImGui gamepad navigation.
- `ImGui.ToggleKeyboardInputSharing` - Toggle ImGui keyboard input sharing.
- `ImGui.ToggleGamepadInputSharing` - Toggle ImGui gamepad input sharing.
- `ImGui.ToggleDemo` - Toggle ImGui demo.

### Console debug variables

There is a self-debug functionality build into this plugin. This is hidden by default as it is hardly useful outside of this pluguin. To enable it, go to `ImGuiModuleDebug.h` and change `IMGUI_MODULE_DEVELOPER`.

- `ImGui.Debug.Widget` - Show debug for SImGuiWidget.
- `ImGui.Debug.Input` - Show debug for input state.

### Settings
Plugin settings can be found in *Project Settings/Plugins/ImGui* panel. Right now they amount to a few properties that customize input handling.

##### Extensions
- `ImGui Input Handler Class` - Path to own implementation of ImGui Input Handler that allows limited customization of the input handling. If not set, then the default implementation is used.

>*If you decide to implement an own handler, please keep in mind that I'm thinking about replacing it.*

##### Input
- `Share Keyboard Input` - Whether by default, ImGui should [shared with game](#sharing-input) keyboard input.
- `Share Gamepad Input` - Whether by default, ImGui should [shared with game](#sharing-input) gamepad input.
- `Use Software Cursor` - Whether ImGui should draw its own cursor in place of the hardware one.

##### Keyboard shortcuts
- `Toggle Input` - Allows to define a shortcut key to a command that toggles the input mode. Note that this is using `DebugExecBindings` which is not available in shipping builds.

See also
--------

 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [An Introduction to UE4 Plugins](https://wiki.unrealengine.com/An_Introduction_to_UE4_Plugins).


License
-------

Unreal ImGui is licensed under the MIT License, see LICENSE for more information.

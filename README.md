Unreal ImGui
============
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 4 plug-in that integrates [Dear ImGui](https://github.com/ocornut/imgui) developed by Omar Cornut.

Dear ImGui is an immediate-mode graphical user interface library that is very lightweight and easy to use. It can be very useful when creating debugging tools.

:stop_button: Read Me First
---------------------------
Please note that this is a forked project from [https://github.com/segross/UnrealImGui](https://github.com/segross/UnrealImGui). I do not take credit for the work he's put into making Dear ImGui work in Unreal Engine. The work I've done to this fork is listed below.

I've removed large portions of this readme.md to keep redundant information between the base project and this fork to a minimum. If you wish to read the original readme.md, please see this link: [UnrealImGui ReadMe.md](https://github.com/segross/UnrealImGui/blob/master/README.md).

Also note that the NetImGui branch is not up to date with any of this fork's changes.

Fork Additions/Fixes
--------------------
 - Updated core source files for Unreal Engine 5, new ImGui and ImPlot
 - Updated Dear ImGui to 1.87
 - Added ImPlot v0.13 WIP
 - `ImGui::IsKey*` now functional with all known ImGui keys.
 - Updated input handling flow to be [standard compliant](https://github.com/ocornut/imgui/issues/4921) with Dear ImGui 1.87 which makes ImGui react better at low FPS. Will add `IMGUI_DISABLE_OBSOLETE_KEYIO` preprocessor once I've ripped out old style input.
 - Allowed `UTexture` for Texture Manager so render targets can also be rendered to quads rather than just being limited to using `UTexture2D` instances.

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

# Additional Knowledge

Using ImPlot
------------
It's pretty easy to use ImPlot, it's pretty much the same drill as using Dear ImGui with the UnrealImGui plugin. You can see documentation on how to use ImPlot here: [ImPlot](https://github.com/epezent/implot).

The only thing you won't need to do is call the `ImPlot::CreateContext()` and `ImPlot::DestroyContext` routines as they're already called when ImGui's context is created within UnrealImGui's guts.

Drawing a UTextureRenderTarget2D
----
One might want to render viewports into the world in an ImGui window. You can do this pretty simply by generating a `UTextureRenderTarget2D` then assigning that to a `ASceneCapture2D` actor in your world. Here's some sample code for generating an correctly managing the `UTextureRenderTarget2D`:
```cpp
void Init()
{
    TextureRenderTarget = NewObject<UTextureRenderTarget2D>();
    if(IsValid(TextureRenderTarget))
    {
        TextureRenderTarget->InitAutoFormat(512, 512);
        TextureRenderTarget->UpdateResourceImmediate(true);
    }

    // ... Generate a unique TextureName here
    // Register this render target as an ImGui interop handled texture
    ImGuiTextureHandle = FImGuiModule::Get().FindTextureHandle(TextureName);
	if(!ImGuiTextureHandle.IsValid())
	{
		if(IsValid(TextureRenderTarget))
		{
			ImGuiTextureHandle = FImGuiModule::Get().RegisterTexture(TextureName, TextureRenderTarget, true);
		}
	}
}

~Class()
{
    // Requires releasing to avoid memory leak
    FImGuiModule::Get().ReleaseTexture(ImGuiTextureHandle);
}

void Render()
{
    // Actually submit the draw command to ImGui to render the quad with the texture
    if(ImGuiTextureHandle.IsValid())
	{
		ImGui::Image(ImGuiTextureHandle.GetTextureId(), {512.f, 512.f});
	}
}
```

Then generating the `ASceneCapture2D`:
```cpp
void Init()
{
    FActorSpawnParameters SpawnInfo;
    SceneCapture2D = World->SpawnActor<ASceneCapture2D>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
    SceneCaptureComponent2D->TextureTarget = TextureRenderTarget;
    SceneCaptureComponent2D->UpdateContent();

    // Need to use this in order to force capture to use tone curve and also set alpha to scene alpha (1)
    SceneCaptureComponent2D->CaptureSource = ESceneCaptureSource::SCS_FinalToneCurveHDR;
}
```

### Troubleshooting
If you're using a scene capture and your quad is not drawing at all, make sure your scene capture "Capture Source" is set to "Final Color (with tone curve) in Linear sRGB gamut" to avoid alpha being set to 0 (since there's no way to instruct ImGui to ignore alpha without modding the core UnrealImGui plugin).

If you're getting crashes or seg faults during rendering, make sure you're using `UPROPERTY()` on your class variables!

# Misc

See also
--------
 - [Original Project by segross](https://github.com/segross/UnrealImGui)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [ImPlot](https://github.com/epezent/implot)


License
-------
Unreal ImGui (and this fork) is licensed under the MIT License, see LICENSE for more information.

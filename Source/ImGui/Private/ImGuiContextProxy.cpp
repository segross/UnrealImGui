// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextProxy.h"

#include "ImGuiImplementation.h"
#include "ImGuiInteroperability.h"

#include <Runtime/Launch/Resources/Version.h>


static constexpr float DEFAULT_CANVAS_WIDTH = 3840.f;
static constexpr float DEFAULT_CANVAS_HEIGHT = 2160.f;


namespace
{
	FString GetSaveDirectory()
	{
#if (ENGINE_MAJOR_VERSION > 4 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 18))
		const FString SavedDir = FPaths::ProjectSavedDir();
#else
		const FString SavedDir = FPaths::GameSavedDir();
#endif

		FString Directory = FPaths::Combine(*SavedDir, TEXT("ImGui"));

		// Make sure that directory is created.
		IPlatformFile::GetPlatformPhysical().CreateDirectory(*Directory);

		return Directory;
	}

	FString GetIniFile(const FString& Name)
	{
		static FString SaveDirectory = GetSaveDirectory();
		return FPaths::Combine(SaveDirectory, Name + TEXT(".ini"));
	}
}

FImGuiContextProxy::FImGuiContextProxy(const FString& InName)
	: Name(InName)
	, IniFilename(TCHAR_TO_ANSI(*GetIniFile(InName)))
{
	// Create context.
	Context = ImGui::CreateContext();

	// Set this context in ImGui for initialization (any allocations will be tracked in this context).
	SetAsCurrent();

	// Start initialization.
	ImGuiIO& IO = ImGui::GetIO();

	// Set session data storage.
	IO.IniFilename = IniFilename.c_str();

	// Use pre-defined canvas size.
	IO.DisplaySize = { DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT };

	// When GetTexData is called for the first time it builds atlas texture and copies mouse cursor data to context.
	// When multiple contexts share atlas then only the first one will get mouse data. A simple workaround is to use
	// a temporary atlas if shared one is already built.
	unsigned char* Pixels;
	const bool bIsAltasBuilt = IO.Fonts->TexPixelsAlpha8 != nullptr;
	if (bIsAltasBuilt)
	{
		ImFontAtlas().GetTexDataAsRGBA32(&Pixels, nullptr, nullptr);
	}
	else
	{
		IO.Fonts->GetTexDataAsRGBA32(&Pixels, nullptr, nullptr);
	}

	// Initialize key mapping, so context can correctly interpret input state.
	ImGuiInterops::SetUnrealKeyMap(IO);

	// Begin frame to complete context initialization (this is to avoid problems with other systems calling to ImGui
	// during startup).
	BeginFrame();
}

FImGuiContextProxy::FImGuiContextProxy(FImGuiContextProxy&& Other)
	: Context(std::move(Other.Context))
	, bHasActiveItem(Other.bHasActiveItem)
	, DrawEvent(std::move(Other.DrawEvent))
	, InputState(std::move(Other.InputState))
	, DrawLists(std::move(Other.DrawLists))
	, Name(std::move(Other.Name))
	, IniFilename(std::move(Other.IniFilename))
{
	Other.Context = nullptr;
}

FImGuiContextProxy& FImGuiContextProxy::operator=(FImGuiContextProxy&& Other)
{
	// Swapping context so it can be destroyed with the other object.
	using std::swap;
	swap(Context, Other.Context);

	// Just moving remaining data that doesn't affect cleanup.
	bHasActiveItem = Other.bHasActiveItem;
	DrawEvent = std::move(Other.DrawEvent);
	InputState = std::move(Other.InputState);
	DrawLists = std::move(Other.DrawLists);
	Name = std::move(Other.Name);
	IniFilename = std::move(Other.IniFilename);

	return *this;
}

FImGuiContextProxy::~FImGuiContextProxy()
{
	if (Context)
	{
		// Set this context in ImGui for de-initialization (any de-allocations will be tracked in this context).
		SetAsCurrent();

		// Save context data and destroy.
		ImGuiImplementation::SaveCurrentContextIniSettings(IniFilename.c_str());
		ImGui::DestroyContext(Context);

		// Set default context in ImGui to keep global context pointer valid.
		ImGui::SetCurrentContext(&ImGuiImplementation::GetDefaultContext());
	}
}

void FImGuiContextProxy::Tick(float DeltaSeconds, FSimpleMulticastDelegate* SharedDrawEvent)
{
	if (bIsFrameStarted)
	{
		// Broadcast draw event to allow listeners to draw their controls to this context.
		if (DrawEvent.IsBound())
		{
			DrawEvent.Broadcast();
		}
		if (SharedDrawEvent && SharedDrawEvent->IsBound())
		{
			SharedDrawEvent->Broadcast();
		}

		// Ending frame will produce render output that we capture and store for later use. This also puts context to
		// state in which it does not allow to draw controls, so we want to immediately start a new frame.
		EndFrame();
	}

	// Begin a new frame and set the context back to a state in which it allows to draw controls.
	BeginFrame(DeltaSeconds);

	// Update context information.
	bHasActiveItem = ImGui::IsAnyItemActive();
}

void FImGuiContextProxy::BeginFrame(float DeltaTime)
{
	if (!bIsFrameStarted)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DeltaTime = DeltaTime;

		if (InputState)
		{
			ImGuiInterops::CopyInput(IO, *InputState);
		}

		ImGui::NewFrame();

		bIsFrameStarted = true;
	}
}

void FImGuiContextProxy::EndFrame()
{
	if (bIsFrameStarted)
	{
		// Prepare draw data (after this call we cannot draw to this context until we start a new frame).
		ImGui::Render();

		// Update our draw data, so we can use them later during Slate rendering while ImGui is in the middle of the
		// next frame.
		UpdateDrawData(ImGui::GetDrawData());

		bIsFrameStarted = false;
	}
}

void FImGuiContextProxy::UpdateDrawData(ImDrawData* DrawData)
{
	if (DrawData && DrawData->CmdListsCount > 0)
	{
		DrawLists.SetNum(DrawData->CmdListsCount, false);

		for (int Index = 0; Index < DrawData->CmdListsCount; Index++)
		{
			DrawLists[Index].TransferDrawData(*DrawData->CmdLists[Index]);
		}
	}
	else
	{
		// If we are not rendering then this might be a good moment to empty the array.
		DrawLists.Empty();
	}
}

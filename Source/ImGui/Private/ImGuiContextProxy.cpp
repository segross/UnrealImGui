// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextProxy.h"

#include "ImGuiImplementation.h"
#include "ImGuiInteroperability.h"


static constexpr float DEFAULT_CANVAS_WIDTH = 3840.f;
static constexpr float DEFAULT_CANVAS_HEIGHT = 2160.f;

FImGuiContextProxy::FImGuiContextProxy()
{
	// Create context.
	Context = ImGui::CreateContext();

	// Set this context in ImGui for initialization (any allocations will be tracked in this context).
	SetAsCurrent();

	// Start initialization.
	ImGuiIO& IO = ImGui::GetIO();

	// Use pre-defined canvas size.
	IO.DisplaySize = { DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT };

	// Load texture atlas.
	unsigned char* Pixels;
	IO.Fonts->GetTexDataAsRGBA32(&Pixels, nullptr, nullptr);

	// Initialize key mapping, so context can correctly interpret input state.
	ImGuiInterops::SetUnrealKeyMap(IO);

	// Begin frame to complete context initialization (this is to avoid problems with other systems calling to ImGui
	// during startup).
	BeginFrame();
}

FImGuiContextProxy::FImGuiContextProxy(FImGuiContextProxy&& Other)
	: Context(std::move(Other.Context))
	, DrawEvent(std::move(Other.DrawEvent))
	, InputState(std::move(Other.InputState))
	, DrawLists(std::move(Other.DrawLists))
{
	Other.Context = nullptr;
}

FImGuiContextProxy& FImGuiContextProxy::operator=(FImGuiContextProxy&& Other)
{
	Context = std::move(Other.Context);
	Other.Context = nullptr;
	DrawEvent = std::move(Other.DrawEvent);
	InputState = std::move(Other.InputState);
	DrawLists = std::move(Other.DrawLists);
	return *this;
}

FImGuiContextProxy::~FImGuiContextProxy()
{
	if (Context)
	{
		// Set this context in ImGui for de-initialization (any de-allocations will be tracked in this context).
		SetAsCurrent();

		// Shutdown to save data etc.
		ImGui::Shutdown();

		// Destroy the context.
		ImGui::DestroyContext(Context);

		// Set default context in ImGui to keep global context pointer valid.
		ImGui::SetCurrentContext(&GetDefaultContext());
	}
}

void FImGuiContextProxy::Tick(float DeltaSeconds)
{
	if (bIsFrameStarted)
	{
		// Broadcast draw event to allow listeners to draw their controls to this context.
		if (DrawEvent.IsBound())
		{
			DrawEvent.Broadcast();
		}

		// Ending frame will produce render output that we capture and store for later use. This also puts context to
		// state in which it does not allow to draw controls, so we want to immediately start a new frame.
		EndFrame();
	}

	// Begin a new frame and set the context back to a state in which it allows to draw controls.
	BeginFrame(DeltaSeconds);
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

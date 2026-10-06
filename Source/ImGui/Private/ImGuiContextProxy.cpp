// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiContextProxy.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiImplementation.h"
#include "ImGuiInteroperability.h"
#include "Utilities/Arrays.h"
#include "VersionCompatibility.h"

#include <GenericPlatform/GenericPlatformFile.h>
#include <Math/UnrealMathUtility.h>
#include <Misc/Paths.h>
#include <HAL/PlatformApplicationMisc.h>

#include <cfloat>


static constexpr float DEFAULT_CANVAS_WIDTH = 3840.f;
static constexpr float DEFAULT_CANVAS_HEIGHT = 2160.f;


namespace
{
	FString GetSaveDirectory()
	{
#if ENGINE_COMPATIBILITY_LEGACY_SAVED_DIR
		const FString SavedDir = FPaths::GameSavedDir();
#else
		const FString SavedDir = FPaths::ProjectSavedDir();
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

	struct FGuardCurrentContext
	{
		FGuardCurrentContext()
			: OldContext(ImGui::GetCurrentContext())
		{
		}

		~FGuardCurrentContext()
		{
			if (bRestore)
			{
				ImGui::SetCurrentContext(OldContext);
			}
		}

		FGuardCurrentContext(FGuardCurrentContext&& Other)
			: OldContext(MoveTemp(Other.OldContext))
		{
			Other.bRestore = false;
		}

		FGuardCurrentContext& operator=(FGuardCurrentContext&&) = delete;

		FGuardCurrentContext(const FGuardCurrentContext&) = delete;
		FGuardCurrentContext& operator=(const FGuardCurrentContext&) = delete;

	private:

		ImGuiContext* OldContext = nullptr;
		bool bRestore = true;
	};

	enum class EStyleSizeSentinel
	{
		None,
		NonPositiveOrMax,
		Negative
	};

	struct FScalarStyleSize
	{
		float ImGuiStyle::* Member;
		EStyleSizeSentinel Sentinel = EStyleSizeSentinel::None;
	};

	// Keep these fields in sync with ImGuiStyle::ScaleAllSizes(). Scale from an unrounded
	// baseline instead of repeatedly truncating sizes, so DPI round trips preserve the theme.
	constexpr FScalarStyleSize ScalarStyleSizes[] =
	{
		{ &ImGuiStyle::_MainScale },
		{ &ImGuiStyle::WindowRounding },
		{ &ImGuiStyle::WindowBorderSize },
		{ &ImGuiStyle::WindowBorderHoverPadding },
		{ &ImGuiStyle::ChildRounding },
		{ &ImGuiStyle::ChildBorderSize },
		{ &ImGuiStyle::PopupRounding },
		{ &ImGuiStyle::PopupBorderSize },
		{ &ImGuiStyle::FrameBorderSize },
		{ &ImGuiStyle::FrameRounding },
		{ &ImGuiStyle::IndentSpacing },
		{ &ImGuiStyle::ColumnsMinSpacing },
		{ &ImGuiStyle::ScrollbarSize },
		{ &ImGuiStyle::ScrollbarRounding },
		{ &ImGuiStyle::ScrollbarPadding },
		{ &ImGuiStyle::GrabMinSize },
		{ &ImGuiStyle::GrabRounding },
		{ &ImGuiStyle::LogSliderDeadzone },
		{ &ImGuiStyle::ImageRounding },
		{ &ImGuiStyle::ImageBorderSize },
		{ &ImGuiStyle::TabRounding },
		{ &ImGuiStyle::TabBorderSize },
		{ &ImGuiStyle::TabMinWidthBase },
		{ &ImGuiStyle::TabMinWidthShrink },
		{ &ImGuiStyle::TabCloseButtonMinWidthSelected, EStyleSizeSentinel::NonPositiveOrMax },
		{ &ImGuiStyle::TabCloseButtonMinWidthUnselected, EStyleSizeSentinel::NonPositiveOrMax },
		{ &ImGuiStyle::TabBarBorderSize },
		{ &ImGuiStyle::TabBarOverlineSize },
		{ &ImGuiStyle::TreeLinesSize },
		{ &ImGuiStyle::TreeLinesRounding },
		{ &ImGuiStyle::MenuItemRounding },
		{ &ImGuiStyle::SelectableRounding },
		{ &ImGuiStyle::DragDropTargetRounding, EStyleSizeSentinel::Negative },
		{ &ImGuiStyle::DragDropTargetBorderSize },
		{ &ImGuiStyle::DragDropTargetPadding },
		{ &ImGuiStyle::ColorMarkerSize },
		{ &ImGuiStyle::InputTextCursorSize },
		{ &ImGuiStyle::SeparatorSize },
		{ &ImGuiStyle::SeparatorTextBorderSize },
		{ &ImGuiStyle::MouseCursorScale }
	};

	constexpr ImVec2 ImGuiStyle::* VectorStyleSizes[] =
	{
		&ImGuiStyle::WindowPadding,
		&ImGuiStyle::WindowMinSize,
		&ImGuiStyle::FramePadding,
		&ImGuiStyle::ItemSpacing,
		&ImGuiStyle::ItemInnerSpacing,
		&ImGuiStyle::CellPadding,
		&ImGuiStyle::TouchExtraPadding,
		&ImGuiStyle::SeparatorTextPadding,
		&ImGuiStyle::DisplayWindowPadding,
		&ImGuiStyle::DisplaySafeAreaPadding
	};

	bool IsStyleSizeSentinel(float Value, EStyleSizeSentinel Sentinel)
	{
		return (Sentinel == EStyleSizeSentinel::NonPositiveOrMax && (Value <= 0.f || Value == FLT_MAX))
			|| (Sentinel == EStyleSizeSentinel::Negative && Value < 0.f);
	}

	float ScaleStyleSize(float Value, float PreviousValue, float& UnscaledValue, float OldScale, float NewScale,
		EStyleSizeSentinel Sentinel = EStyleSizeSentinel::None)
	{
		if (Value != PreviousValue)
		{
			// A theme edit is expressed in the current DPI's units. Update only this component's
			// baseline; unchanged components retain their original precision.
			UnscaledValue = IsStyleSizeSentinel(Value, Sentinel) ? Value : Value / OldScale;
		}

		return IsStyleSizeSentinel(UnscaledValue, Sentinel) ? UnscaledValue : UnscaledValue * NewScale;
	}

	void ScaleStyleSizes(ImGuiStyle& Style, ImGuiStyle& UnscaledStyle, const ImGuiStyle& PreviousStyle,
		float OldScale, float NewScale)
	{
		for (const FScalarStyleSize& Size : ScalarStyleSizes)
		{
			Style.*Size.Member = ScaleStyleSize(Style.*Size.Member, PreviousStyle.*Size.Member,
				UnscaledStyle.*Size.Member, OldScale, NewScale, Size.Sentinel);
		}

		for (ImVec2 ImGuiStyle::* Member : VectorStyleSizes)
		{
			ImVec2& Value = Style.*Member;
			const ImVec2& PreviousValue = PreviousStyle.*Member;
			ImVec2& UnscaledValue = UnscaledStyle.*Member;
			Value.x = ScaleStyleSize(Value.x, PreviousValue.x, UnscaledValue.x, OldScale, NewScale);
			Value.y = ScaleStyleSize(Value.y, PreviousValue.y, UnscaledValue.y, OldScale, NewScale);
		}
	}
}

FImGuiContextProxy::FImGuiContextProxy(const FString& InName, int32 InContextIndex, ImFontAtlas* InFontAtlas, float InDPIScale, FTextureManager& InTextureManager)
	: TextureManager(InTextureManager)
	, Name(InName)
	, ContextIndex(InContextIndex)
	, IniFilename(TCHAR_TO_ANSI(*GetIniFile(InName)))
{
	// Create context.
	Context = ImGui::CreateContext(InFontAtlas);

	// Set this context in ImGui for initialization (any allocations will be tracked in this context).
	SetAsCurrent();

	// Start initialization.
	ImGuiIO& IO = ImGui::GetIO();
	IO.BackendPlatformName = "UnrealEngine";
	IO.BackendRendererName = "UnrealEngine_Slate";
	IO.BackendFlags |= ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_RendererHasTextures;
	if (sizeof(SlateIndex) >= sizeof(uint32)) IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
	// Slate owns the graphics state; ImGui callbacks cannot change its renderer state.
	PlatformIO.DrawCallback_ResetRenderState = [](const ImDrawList*, const ImDrawCmd*) {};
	PlatformIO.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char*
	{
		static std::string ClipboardText;
		FString Text;
		FPlatformApplicationMisc::ClipboardPaste(Text);
		ClipboardText = TCHAR_TO_UTF8(*Text);
		return ClipboardText.c_str();
	};
	PlatformIO.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* Text)
	{
		FPlatformApplicationMisc::ClipboardCopy(UTF8_TO_TCHAR(Text));
	};

	// Set session data storage.
	IO.IniFilename = IniFilename.c_str();

	// Start with the default canvas size.
	ResetDisplaySize();
	IO.DisplaySize = { static_cast<float>(DisplaySize.X), static_cast<float>(DisplaySize.Y) };

	// Set the initial DPI scale.
	SetDPIScale(InDPIScale);

	// Begin frame to complete context initialization (this is to avoid problems with other systems calling to ImGui
	// during startup).
	BeginFrame();
}

FImGuiContextProxy::~FImGuiContextProxy()
{
	if (Context)
	{
		// It seems that to properly shutdown context we need to set it as the current one (at least in this framework
		// version), even though we can pass it to the destroy function.
		SetAsCurrent();
		if (bIsFrameStarted)
		{
			// Refresh the texture list after atlas updates, before releasing the last context's resources.
			ImGui::EndFrame();
			bIsFrameStarted = false;
		}
		for (ImTextureData* TextureData : ImGui::GetPlatformIO().Textures)
		{
			if (TextureData->RefCount <= 1) TextureManager.ReleaseImGuiTexture(*TextureData);
		}

		// Save context data and destroy.
		ImGui::DestroyContext(Context);
	}
}

void FImGuiContextProxy::ResetDisplaySize()
{
	DisplaySize = { DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT };
}

void FImGuiContextProxy::SetDPIScale(float Scale)
{
	if (!FMath::IsFinite(Scale) || Scale <= 0.f)
	{
		Scale = 1.f;
	}

	if (DPIScale == Scale && bHasDPIScaleStyle)
	{
		return;
	}

	FGuardCurrentContext GuardContext;
	SetAsCurrent();
	ImGuiStyle& Style = ImGui::GetStyle();
	if (!bHasDPIScaleStyle)
	{
		UnscaledStyle = Style;
		LastDPIScaleStyle = Style;
		bHasDPIScaleStyle = true;
	}

	ScaleStyleSizes(Style, UnscaledStyle, LastDPIScaleStyle, DPIScale, Scale);
	Style.FontScaleDpi = Scale;
	LastDPIScaleStyle = Style;
	DPIScale = Scale;
}

void FImGuiContextProxy::DrawEarlyDebug()
{
	if (bIsFrameStarted && !bIsDrawEarlyDebugCalled)
	{
		bIsDrawEarlyDebugCalled = true;

		SetAsCurrent();

		// Delegates called in order specified in FImGuiDelegates.
		BroadcastMultiContextEarlyDebug();
		BroadcastWorldEarlyDebug();
	}
}

void FImGuiContextProxy::DrawDebug()
{
	if (bIsFrameStarted && !bIsDrawDebugCalled)
	{
		bIsDrawDebugCalled = true;

		// Make sure that early debug is always called first to guarantee order specified in FImGuiDelegates.
		DrawEarlyDebug();

		SetAsCurrent();

		// Delegates called in order specified in FImGuiDelegates.
		BroadcastWorldDebug();
		BroadcastMultiContextDebug();
	}
}

void FImGuiContextProxy::Tick(float DeltaSeconds)
{
	// Making sure that we tick only once per frame.
	if (LastFrameNumber != GFrameNumber)
	{
		LastFrameNumber = GFrameNumber;

		SetAsCurrent();

		if (bIsFrameStarted)
		{
			// Make sure that draw events are called before the end of the frame.
			DrawDebug();

			// Ending frame will produce render output that we capture and store for later use. This also puts context to
			// state in which it does not allow to draw controls, so we want to immediately start a new frame.
			EndFrame();
		}

		// Update context information (some data need to be collected before starting a new frame while some other data
		// may need to be collected after).
		bHasActiveItem = ImGui::IsAnyItemActive();
		MouseCursor = ImGuiInterops::ToSlateMouseCursor(ImGui::GetMouseCursor());

		// Begin a new frame and set the context back to a state in which it allows to draw controls.
		BeginFrame(DeltaSeconds);

		// Update remaining context information.
		bWantsMouseCapture = ImGui::GetIO().WantCaptureMouse;
	}
}

void FImGuiContextProxy::BeginFrame(float DeltaTime)
{
	if (!bIsFrameStarted)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DeltaTime = FMath::Max(DeltaTime, 0.000001f);

		ImGuiInterops::CopyInput(IO, InputState);
		InputState.ClearUpdateState();

		IO.DisplaySize = { static_cast<float>(DisplaySize.X), static_cast<float>(DisplaySize.Y) };

		ImGuiImplementation::UpdateFontAtlas(*IO.Fonts);
		ImGui::NewFrame();

		bIsFrameStarted = true;
		bIsDrawEarlyDebugCalled = false;
		bIsDrawDebugCalled = false;
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
	if (DrawData) TextureManager.UpdateImGuiTextures(*DrawData);

	if (DrawData && DrawData->CmdListsCount > 0)
	{
		DrawLists.SetNum(DrawData->CmdListsCount, EAllowShrinking::No);

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

void FImGuiContextProxy::BroadcastWorldEarlyDebug()
{
	if (ContextIndex != Utilities::INVALID_CONTEXT_INDEX)
	{
		FSimpleMulticastDelegate& WorldEarlyDebugEvent = FImGuiDelegatesContainer::Get().OnWorldEarlyDebug(ContextIndex);
		if (WorldEarlyDebugEvent.IsBound())
		{
			WorldEarlyDebugEvent.Broadcast();
		}
	}
}

void FImGuiContextProxy::BroadcastMultiContextEarlyDebug()
{
	FSimpleMulticastDelegate& MultiContextEarlyDebugEvent = FImGuiDelegatesContainer::Get().OnMultiContextEarlyDebug();
	if (MultiContextEarlyDebugEvent.IsBound())
	{
		MultiContextEarlyDebugEvent.Broadcast();
	}
}

void FImGuiContextProxy::BroadcastWorldDebug()
{
	if (DrawEvent.IsBound())
	{
		DrawEvent.Broadcast();
	}

	if (ContextIndex != Utilities::INVALID_CONTEXT_INDEX)
	{
		FSimpleMulticastDelegate& WorldDebugEvent = FImGuiDelegatesContainer::Get().OnWorldDebug(ContextIndex);
		if (WorldDebugEvent.IsBound())
		{
			WorldDebugEvent.Broadcast();
		}
	}
}

void FImGuiContextProxy::BroadcastMultiContextDebug()
{
	FSimpleMulticastDelegate& MultiContextDebugEvent = FImGuiDelegatesContainer::Get().OnMultiContextDebug();
	if (MultiContextDebugEvent.IsBound())
	{
		MultiContextDebugEvent.Broadcast();
	}
}

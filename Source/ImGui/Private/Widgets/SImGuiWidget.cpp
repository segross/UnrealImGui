// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "SImGuiWidget.h"
#include "ImGuiPrivatePCH.h"

#include "SImGuiCanvasControl.h"

#include "ImGuiContextManager.h"
#include "ImGuiContextProxy.h"
#include "ImGuiImplementation.h"
#include "ImGuiInputHandler.h"
#include "ImGuiInputHandlerFactory.h"
#include "ImGuiInteroperability.h"
#include "ImGuiModuleManager.h"
#include "ImGuiModuleSettings.h"
#include "TextureManager.h"
#include "Utilities/Arrays.h"
#include "Utilities/ScopeGuards.h"

#include <Engine/Console.h>

#include <utility>


#if IMGUI_WIDGET_DEBUG

DEFINE_LOG_CATEGORY_STATIC(LogImGuiWidget, Warning, All);

#define IMGUI_WIDGET_LOG(Verbosity, Format, ...) UE_LOG(LogImGuiWidget, Verbosity, Format, __VA_ARGS__)

#define TEXT_INPUT_MODE(Val) (\
	(Val) == EInputMode::Full ? TEXT("Full") :\
	(Val) == EInputMode::MousePointerOnly ? TEXT("MousePointerOnly") :\
	TEXT("None"))

#define TEXT_BOOL(Val) ((Val) ? TEXT("true") : TEXT("false"))

#else

#define IMGUI_WIDGET_LOG(...)

#endif // IMGUI_WIDGET_DEBUG

#if IMGUI_WIDGET_DEBUG
namespace CVars
{
	TAutoConsoleVariable<int> DebugWidget(TEXT("ImGui.Debug.Widget"), 0,
		TEXT("Show debug for SImGuiWidget.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled"),
		ECVF_Default);

	TAutoConsoleVariable<int> DebugInput(TEXT("ImGui.Debug.Input"), 0,
		TEXT("Show debug for input state.\n")
		TEXT("0: disabled (default)\n")
		TEXT("1: enabled"),
		ECVF_Default);
}
#endif // IMGUI_WIDGET_DEBUG

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SImGuiWidget::Construct(const FArguments& InArgs)
{
	checkf(InArgs._ModuleManager, TEXT("Null Module Manager argument"));
	checkf(InArgs._GameViewport, TEXT("Null Game Viewport argument"));

	ModuleManager = InArgs._ModuleManager;
	GameViewport = InArgs._GameViewport;
	ContextIndex = InArgs._ContextIndex;

	// Disable mouse cursor over this widget as we will use ImGui to draw it.
	SetCursor(EMouseCursor::None);

	// Sync visibility with default input enabled state.
	UpdateVisibility();

	// Register to get post-update notifications, so we can clean frame updates.
	ModuleManager->OnPostImGuiUpdate().AddRaw(this, &SImGuiWidget::OnPostImGuiUpdate);

	// Bind this widget to its context proxy.
	auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	checkf(ContextProxy, TEXT("Missing context during widget construction: ContextIndex = %d"), ContextIndex);
#if IMGUI_WIDGET_DEBUG
	ContextProxy->OnDraw().AddRaw(this, &SImGuiWidget::OnDebugDraw);
#endif // IMGUI_WIDGET_DEBUG
	ContextProxy->SetInputState(&InputState);

	// Register for settings change.
	RegisterImGuiSettingsDelegates();

	const auto& Settings = ModuleManager->GetSettings();

	// Cache locally software cursor mode.
	SetUseSoftwareCursor(Settings.UseSoftwareCursor());

	// Create ImGui Input Handler.
	CreateInputHandler(Settings.GetImGuiInputHandlerClass());

	ChildSlot
	[
		SAssignNew(CanvasControlWidget, SImGuiCanvasControl).OnTransformChanged(this, &SImGuiWidget::SetImGuiTransform)
	];

	ImGuiTransform = CanvasControlWidget->GetTransform();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SImGuiWidget::~SImGuiWidget()
{
	// Stop listening for settings change.
	UnregisterImGuiSettingsDelegates();

	// Release ImGui Input Handler.
	ReleaseInputHandler();

	// Remove binding between this widget and its context proxy.
	if (auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex))
	{
#if IMGUI_WIDGET_DEBUG
		ContextProxy->OnDraw().RemoveAll(this);
#endif // IMGUI_WIDGET_DEBUG
		ContextProxy->RemoveInputState(&InputState);
	}

	// Unregister from post-update notifications.
	ModuleManager->OnPostImGuiUpdate().RemoveAll(this);
}

void SImGuiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UpdateMouseStatus();

	// Note: Moving that update to console variable sink or callback might seem like a better alternative but input
	// setup in this function is better handled here.
	UpdateInputEnabled();
}

namespace
{
	FReply ToSlateReply(const FImGuiInputResponse& HandlingResponse)
	{
		return HandlingResponse.HasConsumeRequest() ? FReply::Handled() : FReply::Unhandled();
	}
}

FReply SImGuiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent)
{
	const FImGuiInputResponse Response = InputHandler->OnKeyChar(CharacterEvent);
	if (Response.HasProcessingRequest())
	{
		InputState.AddCharacter(CharacterEvent.GetCharacter());
	}

	return ToSlateReply(Response);
}

FReply SImGuiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey().IsGamepadKey())
	{
		if (InputState.IsGamepadNavigationEnabled())
		{
			const FImGuiInputResponse Response = InputHandler->OnGamepadKeyDown(KeyEvent);
			if (Response.HasProcessingRequest())
			{
				InputState.SetGamepadNavigationKey(KeyEvent, true);
			}

			return ToSlateReply(Response);
		}
		else
		{
			return Super::OnKeyDown(MyGeometry, KeyEvent);
		}
	}
	else
	{
		UpdateCanvasControlMode(KeyEvent);

		const FImGuiInputResponse Response = InputHandler->OnKeyDown(KeyEvent);
		if (Response.HasProcessingRequest())
		{
			InputState.SetKeyDown(KeyEvent, true);
			CopyModifierKeys(KeyEvent);
		}

		return ToSlateReply(Response);
	}
}

FReply SImGuiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey().IsGamepadKey())
	{
		if (InputState.IsGamepadNavigationEnabled())
		{
			// Always handle key up events to protect from leaving accidental keys not cleared in ImGui input state.
			InputState.SetGamepadNavigationKey(KeyEvent, false);

			return ToSlateReply(InputHandler->OnGamepadKeyUp(KeyEvent));
		}
		else
		{
			return Super::OnKeyUp(MyGeometry, KeyEvent);
		}
	}
	else
	{
		UpdateCanvasControlMode(KeyEvent);

		// Always handle key up events to protect from leaving accidental keys not cleared in ImGui input state.
		InputState.SetKeyDown(KeyEvent, false);
		CopyModifierKeys(KeyEvent);

		return ToSlateReply(InputHandler->OnKeyUp(KeyEvent));
	}
}

FReply SImGuiWidget::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent)
{
	if (AnalogInputEvent.GetKey().IsGamepadKey() && InputState.IsGamepadNavigationEnabled())
	{
		const FImGuiInputResponse Response = InputHandler->OnGamepadAxis(AnalogInputEvent);
		if (Response.HasProcessingRequest())
		{
			InputState.SetGamepadNavigationAxis(AnalogInputEvent, AnalogInputEvent.GetAnalogValue());
		}

		return ToSlateReply(Response);
	}
	else
	{
		return Super::OnAnalogValueChanged(MyGeometry, AnalogInputEvent);
	}
}

FReply SImGuiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(MouseEvent, true);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(MouseEvent, true);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.SetMouseDown(MouseEvent, false);
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	InputState.AddMouseWheelDelta(MouseEvent.GetWheelDelta());
	CopyModifierKeys(MouseEvent);

	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FSlateRenderTransform ImGuiToScreen = ImGuiTransform.Concatenate(MyGeometry.GetAccumulatedRenderTransform());
	InputState.SetMousePosition(ImGuiToScreen.Inverse().TransformPoint(MouseEvent.GetScreenSpacePosition()));
	CopyModifierKeys(MouseEvent);

	// This event is called in every frame when we have a mouse, so we can use it to raise notifications.
	NotifyMouseEvent();

	return FReply::Handled();
}

FReply SImGuiWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent)
{
	Super::OnFocusReceived(MyGeometry, FocusEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Focus Received."), ContextIndex);

	// If widget has a keyboard focus we always maintain mouse input. Technically, if mouse is outside of the widget
	// area it won't generate events but we freeze its state until it either comes back or input is completely lost.
	UpdateInputMode(true, IsDirectlyHovered());

	FSlateApplication::Get().ResetToDefaultPointerInputSettings();
	return FReply::Handled();
}

void SImGuiWidget::OnFocusLost(const FFocusEvent& FocusEvent)
{
	Super::OnFocusLost(FocusEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Focus Lost."), ContextIndex);

	UpdateInputMode(false, IsDirectlyHovered());
}

void SImGuiWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::OnMouseEnter(MyGeometry, MouseEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Mouse Enter."), ContextIndex);

	// If mouse enters while input is active then we need to update mouse buttons because there is a chance that we
	// missed some events.
	if (InputMode != EInputMode::None)
	{
		for (const FKey& Button : { EKeys::LeftMouseButton, EKeys::MiddleMouseButton, EKeys::RightMouseButton, EKeys::ThumbMouseButton, EKeys::ThumbMouseButton2 })
		{
			InputState.SetMouseDown(Button, MouseEvent.IsMouseButtonDown(Button));
		}
	}

	UpdateInputMode(HasKeyboardFocus(), true);
}

void SImGuiWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::OnMouseLeave(MouseEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Mouse Leave."), ContextIndex);

	// We don't get any events when application loses focus, but often this is followed by OnMouseLeave, so we can use
	// this event to immediately disable keyboard input if application lost focus.
	UpdateInputMode(HasKeyboardFocus() && GameViewport->Viewport->IsForegroundWindow(), false);
}

FCursorReply SImGuiWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	EMouseCursor::Type MouseCursor = EMouseCursor::None;
	if (!bUseSoftwareCursor)
	{
		if (FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex))
		{
			MouseCursor = ContextProxy->GetMouseCursor();
		}
	}

	return FCursorReply::Cursor(MouseCursor);
}

void SImGuiWidget::CreateInputHandler(const FStringClassReference& HandlerClassReference)
{
	ReleaseInputHandler();

	if (!InputHandler.IsValid())
	{
		InputHandler = FImGuiInputHandlerFactory::NewHandler(HandlerClassReference, ModuleManager, GameViewport.Get(), ContextIndex);
	}
}

void SImGuiWidget::ReleaseInputHandler()
{
	if (InputHandler.IsValid())
	{
		FImGuiInputHandlerFactory::ReleaseHandler(InputHandler.Get());
		InputHandler.Reset();
	}
}

void SImGuiWidget::RegisterImGuiSettingsDelegates()
{
	auto& Settings = ModuleManager->GetSettings();

	if (!Settings.OnImGuiInputHandlerClassChanged.IsBoundToObject(this))
	{
		Settings.OnImGuiInputHandlerClassChanged.AddRaw(this, &SImGuiWidget::CreateInputHandler);
	}
	if (!Settings.OnUseSoftwareCursorChanged.IsBoundToObject(this))
	{
		Settings.OnUseSoftwareCursorChanged.AddRaw(this, &SImGuiWidget::SetUseSoftwareCursor);
	}
}

void SImGuiWidget::UnregisterImGuiSettingsDelegates()
{
	auto& Settings = ModuleManager->GetSettings();

	Settings.OnImGuiInputHandlerClassChanged.RemoveAll(this);
	Settings.OnUseSoftwareCursorChanged.RemoveAll(this);
}

void SImGuiWidget::CopyModifierKeys(const FInputEvent& InputEvent)
{
	InputState.SetControlDown(InputEvent.IsControlDown());
	InputState.SetShiftDown(InputEvent.IsShiftDown());
	InputState.SetAltDown(InputEvent.IsAltDown());
}

void SImGuiWidget::CopyModifierKeys(const FPointerEvent& MouseEvent)
{
	if (InputMode == EInputMode::MousePointerOnly)
	{
		CopyModifierKeys(static_cast<const FInputEvent&>(MouseEvent));
	}
}

bool SImGuiWidget::IsConsoleOpened() const
{
	return GameViewport->ViewportConsole && GameViewport->ViewportConsole->ConsoleState != NAME_None;
}

void SImGuiWidget::UpdateVisibility()
{
	// If we don't use input disable hit test to make this widget invisible for cursors hit detection.
	SetVisibility(bInputEnabled ? EVisibility::Visible : EVisibility::HitTestInvisible);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Visibility updated to '%s'."),
		ContextIndex, *GetVisibility().ToString());
}

ULocalPlayer* SImGuiWidget::GetLocalPlayer() const
{
	if (GameViewport.IsValid())
	{
		if (UWorld* World = GameViewport->GetWorld())
		{
			if (ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController())
			{
				return World->GetFirstLocalPlayerFromController();
			}
		}
	}

	return nullptr;
}

void SImGuiWidget::TakeFocus()
{
	auto& SlateApplication = FSlateApplication::Get();

	PreviousUserFocusedWidget = SlateApplication.GetUserFocusedWidget(SlateApplication.GetUserIndexForKeyboard());

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		TSharedRef<SWidget> FocusWidget = SharedThis(this);
		LocalPlayer->GetSlateOperations().CaptureMouse(FocusWidget);
		LocalPlayer->GetSlateOperations().SetUserFocus(FocusWidget);
	}
	else
	{
		SlateApplication.SetKeyboardFocus(SharedThis(this));
	}
}

void SImGuiWidget::ReturnFocus()
{
	if (HasKeyboardFocus())
	{
		auto FocusWidgetPtr = PreviousUserFocusedWidget.IsValid()
			? PreviousUserFocusedWidget.Pin()
			: GameViewport->GetGameViewportWidget();

		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			auto FocusWidgetRef = FocusWidgetPtr.ToSharedRef();
			LocalPlayer->GetSlateOperations().CaptureMouse(FocusWidgetRef);
			LocalPlayer->GetSlateOperations().SetUserFocus(FocusWidgetRef);
		}
		else
		{
			auto& SlateApplication = FSlateApplication::Get();
			SlateApplication.ResetToDefaultPointerInputSettings();
			SlateApplication.SetUserFocus(SlateApplication.GetUserIndexForKeyboard(), FocusWidgetPtr);
		}
	}

	PreviousUserFocusedWidget.Reset();
}

void SImGuiWidget::UpdateInputEnabled()
{
	const bool bEnabled = ModuleManager && ModuleManager->GetProperties().IsInputEnabled();
	if (bInputEnabled != bEnabled)
	{
		bInputEnabled = bEnabled;

		IMGUI_WIDGET_LOG(Log, TEXT("ImGui Widget %d - Input Enabled changed to '%s'."),
			ContextIndex, TEXT_BOOL(bInputEnabled));

		UpdateVisibility();

		if (!bInputEnabled)
		{
			ReturnFocus();
			UpdateInputMode(false, false);
		}
	}

	// Note: Some widgets, like console, can reset focus to viewport after we already grabbed it. If we detect that
	// viewport has a focus while input is enabled we will take it.
	if (bInputEnabled && !HasKeyboardFocus() && !IsConsoleOpened())
	{
		const auto& ViewportWidget = GameViewport->GetGameViewportWidget();
		if (ViewportWidget->HasKeyboardFocus() || ViewportWidget->HasFocusedDescendants())
		{
			TakeFocus();
		}
	}

	// We don't get any events when application loses focus (we get OnMouseLeave but not always) but we fix it with
	// this manual check. We still allow the above code to run, even if we need to suppress keyboard input right after
	// that.
	if (bInputEnabled && !GameViewport->Viewport->IsForegroundWindow() && InputMode == EInputMode::Full)
	{
		UpdateInputMode(false, IsDirectlyHovered());
	}

	if (bInputEnabled)
	{
		InputState.SetKeyboardNavigationEnabled(ModuleManager && ModuleManager->GetProperties().IsKeyboardNavigationEnabled());
		InputState.SetGamepadNavigationEnabled(ModuleManager && ModuleManager->GetProperties().IsGamepadNavigationEnabled());
		const auto& Application = FSlateApplication::Get().GetPlatformApplication();
		InputState.SetGamepad(Application.IsValid() && Application->IsGamepadAttached());
	}
}

void SImGuiWidget::UpdateInputMode(bool bHasKeyboardFocus, bool bHasMousePointer)
{
	const EInputMode NewInputMode =
		bHasKeyboardFocus ? EInputMode::Full :
		bHasMousePointer ? EInputMode::MousePointerOnly :
		EInputMode::None;

	if (InputMode != NewInputMode)
	{
		IMGUI_WIDGET_LOG(Verbose, TEXT("ImGui Widget %d - Input Mode changed from '%s' to '%s'."),
			ContextIndex, TEXT_INPUT_MODE(InputMode), TEXT_INPUT_MODE(NewInputMode));

		// We need to reset input components if we are either fully shutting down or we are downgrading from full to
		// mouse-only input mode.
		if (NewInputMode == EInputMode::None)
		{
			InputState.ResetState();
		}
		else if (InputMode == EInputMode::Full)
		{
			InputState.ResetKeyboardState();
			InputState.ResetNavigationState();
		}

		InputMode = NewInputMode;

		ClearMouseEventNotification();
	}

	InputState.SetMousePointer(bUseSoftwareCursor && bHasMousePointer);
}

void SImGuiWidget::UpdateMouseStatus()
{
	// Note: Mouse leave events can get lost if other viewport takes mouse capture (for instance console is opened by
	// different viewport when this widget is hovered). With that we lose a chance to cleanup and hide ImGui pointer.
	// We could either update ImGui pointer in every frame or like below, use mouse events to catch when mouse is lost.

	if (InputMode == EInputMode::MousePointerOnly)
	{
		if (!HasMouseEventNotification())
		{
			UpdateInputMode(false, IsDirectlyHovered());
		}
		ClearMouseEventNotification();
	}
}

void SImGuiWidget::UpdateCanvasControlMode(const FInputEvent& InputEvent)
{
	CanvasControlWidget->SetActive(InputEvent.IsLeftAltDown() && InputEvent.IsLeftShiftDown());
}

void SImGuiWidget::OnPostImGuiUpdate()
{
	if (InputMode != EInputMode::None)
	{
		InputState.ClearUpdateState();
	}

	ImGuiRenderTransform = ImGuiTransform;
}

namespace
{
	FORCEINLINE FSlateRenderTransform RoundTranslation(const FSlateRenderTransform& Transform)
	{
		const FVector2D& Translation = Transform.GetTranslation();
		return FSlateRenderTransform{ Transform.GetMatrix(),
			FVector2D{ FMath::RoundToFloat(Translation.X), FMath::RoundToFloat(Translation.Y) } };
	}
}

int32 SImGuiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const
{
	if (FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex))
	{
		// Manually update ImGui context to minimise lag between creating and rendering ImGui output. This will also
		// keep frame tearing at minimum because it is executed at the very end of the frame.
		ContextProxy->Tick(FSlateApplication::Get().GetDeltaTime());

		// Calculate transform from ImGui to screen space. Rounding translation is necessary to keep it pixel-perfect
		// in older engine versions.
		const FSlateRenderTransform& WidgetToScreen = AllottedGeometry.GetAccumulatedRenderTransform();
		const FSlateRenderTransform ImGuiToScreen = RoundTranslation(ImGuiRenderTransform.Concatenate(WidgetToScreen));

#if ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
		// Convert clipping rectangle to format required by Slate vertex.
		const FSlateRotatedRect VertexClippingRect{ MyClippingRect };
#endif // ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API

		for (const auto& DrawList : ContextProxy->GetDrawData())
		{
#if ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
			DrawList.CopyVertexData(VertexBuffer, ImGuiToScreen, VertexClippingRect);

			// Get access to the Slate scissor rectangle defined in Slate Core API, so we can customize elements drawing.
			extern SLATECORE_API TOptional<FShortRect> GSlateScissorRect;
			auto GSlateScissorRectSaver = ScopeGuards::MakeStateSaver(GSlateScissorRect);
#else
			DrawList.CopyVertexData(VertexBuffer, ImGuiToScreen);
#endif // ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API

			int IndexBufferOffset = 0;
			for (int CommandNb = 0; CommandNb < DrawList.NumCommands(); CommandNb++)
			{
				const auto& DrawCommand = DrawList.GetCommand(CommandNb, ImGuiToScreen);

				DrawList.CopyIndexData(IndexBuffer, IndexBufferOffset, DrawCommand.NumElements);

				// Advance offset by number of copied elements to position it for the next command.
				IndexBufferOffset += DrawCommand.NumElements;

				// Get texture resource handle for this draw command (null index will be also mapped to a valid texture).
				const FSlateResourceHandle& Handle = ModuleManager->GetTextureManager().GetTextureHandle(DrawCommand.TextureId);

				// Transform clipping rectangle to screen space and apply to elements that we draw.
				const FSlateRect ClippingRect = DrawCommand.ClippingRect.IntersectionWith(MyClippingRect);

#if ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
				GSlateScissorRect = FShortRect{ ClippingRect };
#else
				OutDrawElements.PushClip(FSlateClippingZone{ ClippingRect });
#endif // ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API

				// Add elements to the list.
				FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, Handle, VertexBuffer, IndexBuffer, nullptr, 0, 0);

#if !ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
				OutDrawElements.PopClip();
#endif // ENGINE_COMPATIBILITY_LEGACY_CLIPPING_API
			}
		}
	}

	return Super::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, WidgetStyle, bParentEnabled);
}

FVector2D SImGuiWidget::ComputeDesiredSize(float Scale) const
{
	return FVector2D{ 3840.f, 2160.f } * Scale;
}

#if IMGUI_WIDGET_DEBUG

static TArray<FKey> GetImGuiMappedKeys()
{
	TArray<FKey> Keys;
	Keys.Reserve(Utilities::ArraySize<ImGuiInterops::ImGuiTypes::FKeyMap>::value + 8);

	// ImGui IO key map.
	Keys.Emplace(EKeys::Tab);
	Keys.Emplace(EKeys::Left);
	Keys.Emplace(EKeys::Right);
	Keys.Emplace(EKeys::Up);
	Keys.Emplace(EKeys::Down);
	Keys.Emplace(EKeys::PageUp);
	Keys.Emplace(EKeys::PageDown);
	Keys.Emplace(EKeys::Home);
	Keys.Emplace(EKeys::End);
	Keys.Emplace(EKeys::Delete);
	Keys.Emplace(EKeys::BackSpace);
	Keys.Emplace(EKeys::Enter);
	Keys.Emplace(EKeys::Escape);
	Keys.Emplace(EKeys::A);
	Keys.Emplace(EKeys::C);
	Keys.Emplace(EKeys::V);
	Keys.Emplace(EKeys::X);
	Keys.Emplace(EKeys::Y);
	Keys.Emplace(EKeys::Z);

	// Modifier keys.
	Keys.Emplace(EKeys::LeftShift);
	Keys.Emplace(EKeys::RightShift);
	Keys.Emplace(EKeys::LeftControl);
	Keys.Emplace(EKeys::RightControl);
	Keys.Emplace(EKeys::LeftAlt);
	Keys.Emplace(EKeys::RightAlt);
	Keys.Emplace(EKeys::LeftCommand);
	Keys.Emplace(EKeys::RightCommand);

	return Keys;
}

// Column layout utilities.
namespace Columns
{
	template<typename FunctorType>
	static void CollapsingGroup(const char* Name, int Columns, FunctorType&& DrawContent)
	{
		if (ImGui::CollapsingHeader(Name, ImGuiTreeNodeFlags_DefaultOpen))
		{
			const int LastColumns = ImGui::GetColumnsCount();
			ImGui::Columns(Columns, nullptr, false);
			DrawContent();
			ImGui::Columns(LastColumns);
		}
	}
}

// Controls tweaked for 2-columns layout.
namespace TwoColumns
{
	template<typename FunctorType>
	static inline void CollapsingGroup(const char* Name, FunctorType&& DrawContent)
	{
		Columns::CollapsingGroup(Name, 2, std::forward<FunctorType>(DrawContent));
	}

	namespace
	{
		void LabelText(const char* Label)
		{
			ImGui::Text("%s:", Label);
		}

		void LabelText(const wchar_t* Label)
		{
			ImGui::Text("%ls:", Label);
		}
	}

	template<typename LabelType>
	static void Value(LabelType&& Label, int32 Value)
	{
		LabelText(Label); ImGui::NextColumn();
		ImGui::Text("%d", Value); ImGui::NextColumn();
	}

	template<typename LabelType>
	static void Value(LabelType&& Label, uint32 Value)
	{
		LabelText(Label); ImGui::NextColumn();
		ImGui::Text("%u", Value); ImGui::NextColumn();
	}

	template<typename LabelType>
	static void Value(LabelType&& Label, float Value)
	{
		LabelText(Label); ImGui::NextColumn();
		ImGui::Text("%f", Value); ImGui::NextColumn();
	}

	template<typename LabelType>
	static void Value(LabelType&& Label, bool bValue)
	{
		LabelText(Label); ImGui::NextColumn();
		ImGui::Text("%ls", TEXT_BOOL(bValue)); ImGui::NextColumn();
	}

	template<typename LabelType>
	static void Value(LabelType&& Label, const TCHAR* Value)
	{
		LabelText(Label); ImGui::NextColumn();
		ImGui::Text("%ls", Value); ImGui::NextColumn();
	}
}

namespace Styles
{
	template<typename FunctorType>
	static void TextHighlight(bool bHighlight, FunctorType&& DrawContent)
	{
		if (bHighlight)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, { 1.f, 1.f, 0.5f, 1.f });
		}
		DrawContent();
		if (bHighlight)
		{
			ImGui::PopStyleColor();
		}
	}
}

void SImGuiWidget::OnDebugDraw()
{
	if (CVars::DebugWidget.GetValueOnGameThread() > 0)
	{
		bool bDebug = true;
		ImGui::SetNextWindowSize(ImVec2(380, 480), ImGuiSetCond_Once);
		if (ImGui::Begin("ImGui Widget Debug", &bDebug))
		{
			ImGui::Spacing();

			TwoColumns::CollapsingGroup("Context", [&]()
			{
				TwoColumns::Value("Context Index", ContextIndex);
				FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
				TwoColumns::Value("Context Name", ContextProxy ? *ContextProxy->GetName() : TEXT("< Null >"));
				TwoColumns::Value("Game Viewport", *GameViewport->GetName());
			});

			TwoColumns::CollapsingGroup("Input Mode", [&]()
			{
				TwoColumns::Value("Input Enabled", bInputEnabled);
				TwoColumns::Value("Input Mode", TEXT_INPUT_MODE(InputMode));
				TwoColumns::Value("Input Has Mouse Pointer", InputState.HasMousePointer());
			});

			TwoColumns::CollapsingGroup("Widget", [&]()
			{
				TwoColumns::Value("Visibility", *GetVisibility().ToString());
				TwoColumns::Value("Is Hovered", IsHovered());
				TwoColumns::Value("Is Directly Hovered", IsDirectlyHovered());
				TwoColumns::Value("Has Keyboard Input", HasKeyboardFocus());
			});

			TwoColumns::CollapsingGroup("Viewport", [&]()
			{
				const auto& ViewportWidget = GameViewport->GetGameViewportWidget();
				TwoColumns::Value("Is Foreground Window", GameViewport->Viewport->IsForegroundWindow());
				TwoColumns::Value("Is Hovered", ViewportWidget->IsHovered());
				TwoColumns::Value("Is Directly Hovered", ViewportWidget->IsDirectlyHovered());
				TwoColumns::Value("Has Mouse Capture", ViewportWidget->HasMouseCapture());
				TwoColumns::Value("Has Keyboard Input", ViewportWidget->HasKeyboardFocus());
				TwoColumns::Value("Has Focused Descendants", ViewportWidget->HasFocusedDescendants());
				auto Widget = PreviousUserFocusedWidget.Pin();
				TwoColumns::Value("Previous User Focused", Widget.IsValid() ? *Widget->GetTypeAsString() : TEXT("None"));
			});
		}
		ImGui::End();

		if (!bDebug)
		{
			CVars::DebugWidget->Set(0, ECVF_SetByConsole);
		}
	}

	if (CVars::DebugInput.GetValueOnGameThread() > 0)
	{
		bool bDebug = true;
		ImGui::SetNextWindowSize(ImVec2(460, 480), ImGuiSetCond_Once);
		if (ImGui::Begin("ImGui Input State", &bDebug))
		{
			const ImVec4 HiglightColor{ 1.f, 1.f, 0.5f, 1.f };
			Columns::CollapsingGroup("Mapped Keys", 4, [&]()
			{
				static const auto& Keys = GetImGuiMappedKeys();

				const int32 Num = Keys.Num();

				// Simplified when slicing for two 2.
				const int32 RowsNum = (Num + 1) / 2;

				for (int32 Row = 0; Row < RowsNum; Row++)
				{
					for (int32 Col = 0; Col < 2; Col++)
					{
						const int32 Idx = Row + Col * RowsNum;
						if (Idx < Num)
						{
							const FKey& Key = Keys[Idx];
							const uint32 KeyIndex = ImGuiInterops::GetKeyIndex(Key);
							Styles::TextHighlight(InputState.GetKeys()[KeyIndex], [&]()
							{
								TwoColumns::Value(*Key.GetDisplayName().ToString(), KeyIndex);
							});
						}
						else
						{
							ImGui::NextColumn(); ImGui::NextColumn();
						}
					}
				}
			});

			Columns::CollapsingGroup("Modifier Keys", 4, [&]()
			{
				Styles::TextHighlight(InputState.IsShiftDown(), [&]() { ImGui::Text("Shift"); }); ImGui::NextColumn();
				Styles::TextHighlight(InputState.IsControlDown(), [&]() { ImGui::Text("Control"); }); ImGui::NextColumn();
				Styles::TextHighlight(InputState.IsAltDown(), [&]() { ImGui::Text("Alt"); }); ImGui::NextColumn();
				ImGui::NextColumn();
			});

			Columns::CollapsingGroup("Mouse Buttons", 4, [&]()
			{
				static const FKey Buttons[] = { EKeys::LeftMouseButton, EKeys::RightMouseButton,
					EKeys::MiddleMouseButton, EKeys::ThumbMouseButton, EKeys::ThumbMouseButton2 };

				const int32 Num = Utilities::GetArraySize(Buttons);

				// Simplified when slicing for two 2.
				const int32 RowsNum = (Num + 1) / 2;

				for (int32 Row = 0; Row < RowsNum; Row++)
				{
					for (int32 Col = 0; Col < 2; Col++)
					{
						const int32 Idx = Row + Col * RowsNum;
						if (Idx < Num)
						{
							const FKey& Button = Buttons[Idx];
							const uint32 MouseIndex = ImGuiInterops::GetMouseIndex(Button);
							Styles::TextHighlight(InputState.GetMouseButtons()[MouseIndex], [&]()
							{
								TwoColumns::Value(*Button.GetDisplayName().ToString(), MouseIndex);
							});
						}
						else
						{
							ImGui::NextColumn(); ImGui::NextColumn();
						}
					}
				}
			});

			Columns::CollapsingGroup("Mouse Axes", 4, [&]()
			{
				TwoColumns::Value("Position X", InputState.GetMousePosition().X);
				TwoColumns::Value("Position Y", InputState.GetMousePosition().Y);
				TwoColumns::Value("Wheel Delta", InputState.GetMouseWheelDelta());
				ImGui::NextColumn(); ImGui::NextColumn();
			});

			if (!bDebug)
			{
				CVars::DebugInput->Set(0, ECVF_SetByConsole);
			}
		}
		ImGui::End();
	}
}

#undef TEXT_INPUT_MODE
#undef TEXT_BOOL

#endif // IMGUI_WIDGET_DEBUG

#undef IMGUI_WIDGET_LOG

// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "SImGuiWidget.h"
#include "SImGuiCanvasControl.h"

#include "ImGuiContextManager.h"
#include "ImGuiContextProxy.h"
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

	// Register to get post-update notifications.
	ModuleManager->OnPostImGuiUpdate().AddRaw(this, &SImGuiWidget::OnPostImGuiUpdate);

	// Register debug delegate.
	auto* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
	checkf(ContextProxy, TEXT("Missing context during widget construction: ContextIndex = %d"), ContextIndex);
#if IMGUI_WIDGET_DEBUG
	ContextProxy->OnDraw().AddRaw(this, &SImGuiWidget::OnDebugDraw);
#endif // IMGUI_WIDGET_DEBUG

	// Register for settings change.
	RegisterImGuiSettingsDelegates();

	// Get initial settings.
	const auto& Settings = ModuleManager->GetSettings();
	SetHideMouseCursor(Settings.UseSoftwareCursor());
	CreateInputHandler(Settings.GetImGuiInputHandlerClass());

	// Initialize state.
	UpdateVisibility();
	UpdateMouseCursor();

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
	}

	// Unregister from post-update notifications.
	ModuleManager->OnPostImGuiUpdate().RemoveAll(this);
}

void SImGuiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	Super::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UpdateInputState();
	HandleWindowFocusLost();
}

FReply SImGuiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent)
{
	return InputHandler->OnKeyChar(CharacterEvent);
}

FReply SImGuiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	UpdateCanvasControlMode(KeyEvent);
	return InputHandler->OnKeyDown(KeyEvent);
}

FReply SImGuiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	UpdateCanvasControlMode(KeyEvent);
	return InputHandler->OnKeyUp(KeyEvent);
}

FReply SImGuiWidget::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& AnalogInputEvent)
{
	return InputHandler->OnAnalogValueChanged(AnalogInputEvent);
}

FReply SImGuiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return InputHandler->OnMouseButtonDown(MouseEvent).LockMouseToWidget(SharedThis(this));
}

FReply SImGuiWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return InputHandler->OnMouseButtonDoubleClick(MouseEvent).LockMouseToWidget(SharedThis(this));
}

namespace
{
	bool NeedMouseLock(const FPointerEvent& MouseEvent)
	{
#if FROM_ENGINE_VERSION(4, 20)
		return FSlateApplication::Get().GetPressedMouseButtons().Num() > 0;
#else
		return MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton)
			|| MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
#endif
	}
}

FReply SImGuiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = InputHandler->OnMouseButtonUp(MouseEvent);
	if (!NeedMouseLock(MouseEvent))
	{
		Reply.ReleaseMouseLock();
	}
	return Reply;
}

FReply SImGuiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return InputHandler->OnMouseWheel(MouseEvent);
}

FReply SImGuiWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FSlateRenderTransform ImGuiToScreen = ImGuiTransform.Concatenate(MyGeometry.GetAccumulatedRenderTransform());
	return InputHandler->OnMouseMove(ImGuiToScreen.Inverse().TransformPoint(MouseEvent.GetScreenSpacePosition()));
}

FReply SImGuiWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& FocusEvent)
{
	Super::OnFocusReceived(MyGeometry, FocusEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Focus Received."), ContextIndex);

	bForegroundWindow = GameViewport->Viewport->IsForegroundWindow();
	InputHandler->OnKeyboardInputEnabled();
	InputHandler->OnGamepadInputEnabled();

	FSlateApplication::Get().ResetToDefaultPointerInputSettings();
	return FReply::Handled();
}

void SImGuiWidget::OnFocusLost(const FFocusEvent& FocusEvent)
{
	Super::OnFocusLost(FocusEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Focus Lost."), ContextIndex);

	InputHandler->OnKeyboardInputDisabled();
	InputHandler->OnGamepadInputDisabled();
}

void SImGuiWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Super::OnMouseEnter(MyGeometry, MouseEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Mouse Enter."), ContextIndex);

	InputHandler->OnMouseInputEnabled();
}

void SImGuiWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	Super::OnMouseLeave(MouseEvent);

	IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Mouse Leave."), ContextIndex);

	InputHandler->OnMouseInputDisabled();
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
		Settings.OnUseSoftwareCursorChanged.AddRaw(this, &SImGuiWidget::SetHideMouseCursor);
	}
}

void SImGuiWidget::UnregisterImGuiSettingsDelegates()
{
	auto& Settings = ModuleManager->GetSettings();

	Settings.OnImGuiInputHandlerClassChanged.RemoveAll(this);
	Settings.OnUseSoftwareCursorChanged.RemoveAll(this);
}

void SImGuiWidget::SetHideMouseCursor(bool bHide)
{
	if (bHideMouseCursor != bHide)
	{
		bHideMouseCursor = bHide;
		UpdateMouseCursor();
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

void SImGuiWidget::UpdateMouseCursor()
{
	if (!bHideMouseCursor)
	{
		const FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);
		SetCursor(ContextProxy ? ContextProxy->GetMouseCursor() : EMouseCursor::Default);
	}
	else
	{
		SetCursor(EMouseCursor::None);
	}
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

void SImGuiWidget::UpdateInputState()
{
	const bool bEnabled = ModuleManager && ModuleManager->GetProperties().IsInputEnabled();
	if (bInputEnabled != bEnabled)
	{
		IMGUI_WIDGET_LOG(Log, TEXT("ImGui Widget %d - Input Enabled changed to '%s'."),
			ContextIndex, TEXT_BOOL(bEnabled));

		bInputEnabled = bEnabled;

		UpdateVisibility();
		UpdateMouseCursor();

		if (bInputEnabled)
		{
			// We won't get mouse enter, if viewport is already hovered.
			if (GameViewport->GetGameViewportWidget()->IsHovered())
			{
				InputHandler->OnMouseInputEnabled();
			}

			// Focus is handled later as it can depend on additional factors.
		}
		else
		{
			ReturnFocus();
		}
	}

	// We should request a focus, if we are in the input mode and don't have one. But we should wait, if this is not
	// a foreground window (application), if viewport doesn't have a focus or if console is opened. Note that this
	// will keep this widget from releasing focus to viewport or other widgets as long as we are in the input mode.
	if (bInputEnabled && GameViewport->Viewport->IsForegroundWindow() && !HasKeyboardFocus() && !IsConsoleOpened())
	{
		const auto& ViewportWidget = GameViewport->GetGameViewportWidget();
		if (ViewportWidget->HasKeyboardFocus() || ViewportWidget->HasFocusedDescendants())
		{
			TakeFocus();
		}
	}
}

void SImGuiWidget::HandleWindowFocusLost()
{
	// We can use window foreground status to notify about application losing or receiving focus. In some situations
	// we get mouse leave or enter events, but they are only sent if mouse pointer is inside of the viewport.
	if (bInputEnabled && HasKeyboardFocus())
	{
		if (bForegroundWindow != GameViewport->Viewport->IsForegroundWindow())
		{
			bForegroundWindow = !bForegroundWindow;

			IMGUI_WIDGET_LOG(VeryVerbose, TEXT("ImGui Widget %d - Updating input after %s foreground window status."),
				ContextIndex, bForegroundWindow ? TEXT("getting") : TEXT("losing"));

			if (bForegroundWindow)
			{
				InputHandler->OnKeyboardInputEnabled();
				InputHandler->OnGamepadInputEnabled();
			}
			else
			{
				InputHandler->OnKeyboardInputDisabled();
				InputHandler->OnGamepadInputDisabled();
			}
		}
	}
}

void SImGuiWidget::UpdateCanvasControlMode(const FInputEvent& InputEvent)
{
	CanvasControlWidget->SetActive(InputEvent.IsLeftAltDown() && InputEvent.IsLeftShiftDown());
}

void SImGuiWidget::OnPostImGuiUpdate()
{
	ImGuiRenderTransform = ImGuiTransform;
	UpdateMouseCursor();
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
	FImGuiContextProxy* ContextProxy = ModuleManager->GetContextManager().GetContextProxy(ContextIndex);

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
				TwoColumns::Value("Context Name", ContextProxy ? *ContextProxy->GetName() : TEXT("< Null >"));
				TwoColumns::Value("Game Viewport", *GameViewport->GetName());
			});

			TwoColumns::CollapsingGroup("Input Mode", [&]()
			{
				TwoColumns::Value("Input Enabled", bInputEnabled);
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

	if (ContextProxy && CVars::DebugInput.GetValueOnGameThread() > 0)
	{
		FImGuiInputState& InputState = ContextProxy->GetInputState();

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

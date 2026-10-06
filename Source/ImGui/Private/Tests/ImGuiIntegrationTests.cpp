// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ImGuiContextProxy.h"
#include "ImGuiDrawData.h"
#include "ImGuiInputState.h"
#include "ImGuiTextureHandle.h"
#include "ImGuiModule.h"
#include "Misc/ScopeExit.h"
#include "UObject/GarbageCollection.h"
#include "Framework/Application/SlateApplication.h"
#include "RenderingThread.h"
#include "DynamicRHI.h"
#include "RHICommandList.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include <limits>

namespace
{
	constexpr auto TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FRestoreImGuiContext
	{
		ImGuiContext* Previous = ImGui::GetCurrentContext();
		~FRestoreImGuiContext() { ImGui::SetCurrentContext(Previous); }
	};

	struct FTestImGuiContext : FRestoreImGuiContext
	{
		ImGuiContext* Context = ImGui::CreateContext();
		FTestImGuiContext()
		{
			ImGui::SetCurrentContext(Context);
			ImGui::GetIO().IniFilename = nullptr;
			ImGui::GetIO().DisplaySize = ImVec2(640.f, 480.f);
			ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
		}
		~FTestImGuiContext() { ImGui::DestroyContext(Context); }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiKeyMappingTest, "ImGui.Integration.KeyMappingAndTextureHandles", TestFlags)
bool FImGuiKeyMappingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Named letter"), ImGuiInterops::ToImGuiKey(EKeys::Z), ImGuiKey_Z);
	TestEqual(TEXT("Navigation"), ImGuiInterops::ToImGuiKey(EKeys::PageDown), ImGuiKey_PageDown);
	TestEqual(TEXT("Function key"), ImGuiInterops::ToImGuiKey(EKeys::F12), ImGuiKey_F12);
	TestEqual(TEXT("Keypad"), ImGuiInterops::ToImGuiKey(EKeys::NumPadNine), ImGuiKey_Keypad9);
	TestEqual(TEXT("Unknown key"), ImGuiInterops::ToImGuiKey(FKey()), ImGuiKey_None);
	const TextureIndex TextureIndices[] = { INDEX_NONE, 0, 1, 1234 };
	for (TextureIndex Index : TextureIndices)
	{
		TestEqual(TEXT("Texture ID round trip"), ImGuiInterops::ToTextureIndex(ImGuiInterops::ToImTextureID(Index)), Index);
	}
	TestTrue(TEXT("Texture slot zero has a valid ImGui ID"), ImGuiInterops::ToImTextureID(0) != ImTextureID_Invalid);
	FImGuiTextureHandle Handle;
	TestEqual(TEXT("Null handle"), Handle.GetTextureId(), ImTextureID_Invalid);
	FTestImGuiContext Context;
	ImGui::NewFrame();
	ImGui::Begin("Texture handle API");
	ImGui::Image(Handle, ImVec2(16.f, 16.f)); // Verify the existing convenience API accepts ImTextureRef.
	ImGui::End();
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiInputEventsTest, "ImGui.Integration.FastInputAndReset", TestFlags)
bool FImGuiInputEventsTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	FImGuiInputState Input;
	Input.SetKeyDown(EKeys::A, true);
	Input.SetKeyDown(EKeys::A, false);
	Input.SetMouseDown(EKeys::LeftMouseButton, true);
	Input.SetMouseDown(EKeys::LeftMouseButton, false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Fast key press retained"), ImGui::IsKeyPressed(ImGuiKey_A, false));
	ImGui::EndFrame();
	bool bKeyReleased = false;
	bool bMouseClicked = false;
	bool bMouseReleased = false;
	for (int Frame = 0; Frame < 4; ++Frame)
	{
		ImGui::NewFrame();
		bKeyReleased |= ImGui::IsKeyReleased(ImGuiKey_A);
		bMouseClicked |= ImGui::IsMouseClicked(0);
		bMouseReleased |= ImGui::IsMouseReleased(0);
		ImGui::EndFrame();
	}
	TestTrue(TEXT("Fast key release retained"), bKeyReleased);
	TestTrue(TEXT("Fast mouse press retained"), bMouseClicked);
	TestTrue(TEXT("Fast mouse release retained"), bMouseReleased);
	Input.SetControlDown(true);
	Input.SetKeyDown(EKeys::LeftControl, true);
	Input.SetKeyDown(EKeys::B, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Control modifier"), ImGui::GetIO().KeyCtrl);
	TestTrue(TEXT("Held letter"), ImGui::IsKeyDown(ImGuiKey_B));
	ImGui::EndFrame();
	Input.ResetKeyboard();
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	ImGui::NewFrame();
	TestFalse(TEXT("Reset releases modifier"), ImGui::GetIO().KeyCtrl);
	TestFalse(TEXT("Reset releases held key"), ImGui::IsKeyDown(ImGuiKey_B));
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiGamepadTest, "ImGui.Integration.GamepadMappingAndDisconnect", TestFlags)
bool FImGuiGamepadTest::RunTest(const FString& Parameters)
{
	ImGuiInterops::ImGuiTypes::FNavInputArray Inputs = {};
	ImGuiInterops::SetGamepadNavigationKey(Inputs, EKeys::Gamepad_FaceButton_Bottom, true);
	TestEqual(TEXT("Activate button"), Inputs[ImGuiKey_GamepadFaceDown - ImGuiKey_GamepadStart], 1.f);
	ImGuiInterops::SetGamepadNavigationAxis(Inputs, EKeys::Gamepad_LeftX, 1.f);
	TestEqual(TEXT("Full stick tilt"), Inputs[ImGuiKey_GamepadLStickRight - ImGuiKey_GamepadStart], 1.f);
	ImGuiInterops::SetGamepadNavigationAxis(Inputs, EKeys::Gamepad_LeftX, 0.1f);
	TestEqual(TEXT("Stick dead zone"), Inputs[ImGuiKey_GamepadLStickRight - ImGuiKey_GamepadStart], 0.f);
	FTestImGuiContext Context;
	ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_HasGamepad;
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui::GetIO().AddKeyAnalogEvent(ImGuiKey_GamepadFaceDown, true, 1.f);
	ImGui::NewFrame();
	TestTrue(TEXT("Gamepad key held"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	FImGuiInputState DisconnectedInput;
	ImGuiInterops::CopyInput(ImGui::GetIO(), DisconnectedInput);
	ImGui::NewFrame();
	TestFalse(TEXT("Disconnect releases gamepad keys"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiDrawOffsetsTest, "ImGui.Integration.DrawOffsetsAndCallbacks", TestFlags)
bool FImGuiDrawOffsetsTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	ImGui::NewFrame();
	ImDrawList Source(ImGui::GetDrawListSharedData());
	Source.VtxBuffer.resize(70004);
	FMemory::Memzero(Source.VtxBuffer.Data, Source.VtxBuffer.size_in_bytes());
	Source.VtxBuffer[0] = { ImVec2(20.f, 30.f), ImVec2(0.25f, 0.75f), IM_COL32(200, 100, 50, 255) };
	const ImDrawIdx SourceIndices[] = { 2, 1, 0, 0, 1, 2 };
	for (ImDrawIdx Index : SourceIndices) Source.IdxBuffer.push_back(Index);
	ImDrawCmd Command;
	const ImTextureID CommandTextureId = ImGuiInterops::ToImTextureID(42, 1234);
	Command.TexRef = ImTextureRef(CommandTextureId);
	Command.ElemCount = 3;
	Source.CmdBuffer.push_back(Command);
	int CallbackCount = 0;
	Command.ElemCount = 0;
	Command.UserCallbackData = &CallbackCount;
	Command.UserCallback = [](const ImDrawList*, const ImDrawCmd* DrawCommand) { ++*static_cast<int*>(DrawCommand->UserCallbackData); };
	Source.CmdBuffer.push_back(Command);
	Command.UserCallback = nullptr;
	Command.ElemCount = 3;
	Command.IdxOffset = 3;
	Command.VtxOffset = 70000;
	Source.CmdBuffer.push_back(Command);
	FImGuiDrawList Staged;
	Staged.TransferDrawData(Source);
	const auto FinalCommand = Staged.GetCommand(2, FTransform2D());
	TestEqual(TEXT("Callback invoked once"), CallbackCount, 1);
	TestEqual(TEXT("Full texture identity preserved"), FinalCommand.TextureId, CommandTextureId);
	TestEqual(TEXT("Explicit index offset"), FinalCommand.IndexOffset, 3u);
	TArray<FSlateVertex> Vertices;
	Staged.CopyVertexData(Vertices, FTransform2D());
	TestEqual(TEXT("Slate vertex position"), Vertices[0].Position, FVector2f(20.f, 30.f));
	TestEqual(TEXT("Slate material UV initialized"), Vertices[0].MaterialTexCoords, FVector2f(0.25f, 0.75f));
	TestEqual(TEXT("Slate secondary color initialized"), Vertices[0].SecondaryColor, FColor::Transparent);
	TArray<SlateIndex> Indices;
	Staged.CopyIndexData(Indices, FinalCommand.IndexOffset, FinalCommand.NumElements, FinalCommand.VertexOffset);
	if (sizeof(SlateIndex) >= sizeof(uint32)) TestEqual(TEXT("Large vertex offset preserved"), static_cast<uint32>(Indices[2]), 70002u);
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiTextureLifecycleTest, "ImGui.Integration.DynamicTextureLifecycle", TestFlags)
bool FImGuiTextureLifecycleTest::RunTest(const FString& Parameters)
{
	if (!FSlateApplication::IsInitialized()) { AddError(TEXT("Slate is required for texture validation.")); return false; }
	FTextureManager Manager;
	ImTextureData Texture;
	Texture.Create(ImTextureFormat_RGBA32, 4, 4);
	for (int Pixel = 0; Pixel < 16; ++Pixel)
	{
		Texture.Pixels[Pixel * 4 + 0] = 200;
		Texture.Pixels[Pixel * 4 + 1] = 80;
		Texture.Pixels[Pixel * 4 + 2] = 20;
		Texture.Pixels[Pixel * 4 + 3] = 255;
	}
	ImVector<ImTextureData*> Textures;
	Textures.push_back(&Texture);
	ImDrawData DrawData;
	DrawData.Textures = &Textures;
	Manager.UpdateImGuiTextures(DrawData);
	TestEqual(TEXT("Texture created"), Texture.Status, ImTextureStatus_OK);
	const ImTextureID TextureId = Texture.GetTexID();
	TestTrue(TEXT("Nonzero texture ID"), TextureId != ImTextureID_Invalid);
	const bool bReadGPU = GDynamicRHI && !FString(GDynamicRHI->GetName()).StartsWith(TEXT("Null"));
	const auto ReadPixels = [&Texture]()
	{
		FlushRenderingCommands();
		const FTextureRHIRef Resource = static_cast<UTexture2D*>(Texture.BackendUserData)->GetResource()->TextureRHI;
		TArray<FColor> Pixels;
		ENQUEUE_RENDER_COMMAND(ImGuiTestReadback)([Resource, &Pixels](FRHICommandListImmediate& Commands)
		{
			FReadSurfaceDataFlags Flags(RCM_UNorm);
			Flags.SetLinearToGamma(false);
			Commands.ReadSurfaceData(Resource, FIntRect(0, 0, 4, 4), Pixels, Flags);
		});
		FlushRenderingCommands();
		return Pixels;
	};
	// Deliberately change ImGui's CPU data before the queued upload executes.
	FMemory::Memzero(Texture.Pixels, Texture.GetSizeInBytes());
	if (bReadGPU)
	{
		const TArray<FColor> Pixels = ReadPixels();
		if (TestEqual(TEXT("GPU texture pixel count"), Pixels.Num(), 16))
		{
			TestEqual(TEXT("Creation snapshots RGBA pixels correctly"), Pixels[0], FColor(200, 80, 20, 255));
		}
	}
	Texture.UpdateRect = { 1, 1, 2, 2 };
	for (int Y = 1; Y < 3; ++Y)
	{
		for (int X = 1; X < 3; ++X)
		{
			uint8* Pixel = Texture.Pixels + (X + Y * Texture.Width) * 4;
			Pixel[0] = 40; Pixel[1] = 100; Pixel[2] = 220; Pixel[3] = 255;
		}
	}
	Texture.SetStatus(ImTextureStatus_WantUpdates);
	Manager.UpdateImGuiTextures(DrawData);
	TestEqual(TEXT("Texture updated"), Texture.Status, ImTextureStatus_OK);
	TestEqual(TEXT("Update preserves texture ID"), Texture.GetTexID(), TextureId);
	FMemory::Memzero(Texture.Pixels, Texture.GetSizeInBytes());
	if (bReadGPU)
	{
		const TArray<FColor> Pixels = ReadPixels();
		if (TestEqual(TEXT("Updated GPU texture pixel count"), Pixels.Num(), 16))
		{
			TestEqual(TEXT("Partial update preserves other pixels"), Pixels[0], FColor(200, 80, 20, 255));
			TestEqual(TEXT("Partial update snapshots RGBA pixels correctly"), Pixels[5], FColor(40, 100, 220, 255));
		}
	}
	else AddInfo(TEXT("GPU pixel readback skipped with NullRHI."));
	Texture.WantDestroyNextFrame = true;
	Texture.SetStatus(ImTextureStatus_WantDestroy);
	Texture.UnusedFrames = 1;
	Manager.UpdateImGuiTextures(DrawData);
	TestEqual(TEXT("Staged draws retain texture"), Texture.Status, ImTextureStatus_WantDestroy);
	Texture.UnusedFrames = 3;
	Manager.UpdateImGuiTextures(DrawData);
	TestEqual(TEXT("Texture destroyed"), Texture.Status, ImTextureStatus_Destroyed);
	TestEqual(TEXT("ID invalidated"), Texture.GetTexID(), ImTextureID_Invalid);
	FlushRenderingCommands();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiSharedAtlasTest, "ImGui.Integration.SharedAtlasAndDPI", TestFlags)
bool FImGuiSharedAtlasTest::RunTest(const FString& Parameters)
{
	FRestoreImGuiContext Restore;
	FTextureManager Manager;
	ImFontAtlas Atlas;
	Atlas.AddFontDefault();
	{
		FImGuiContextProxy First(TEXT("ImGuiTest_First"), -1, &Atlas, 1.f, Manager);
		FImGuiContextProxy Second(TEXT("ImGuiTest_Second"), -1, &Atlas, 2.f, Manager);
		TestEqual(TEXT("Shared atlas users"), Atlas.RefCount, 2);
		First.SetAsCurrent();
		ImGui::GetForegroundDrawList()->AddText(ImVec2(10.f, 10.f), IM_COL32_WHITE, "First shared atlas");
		ImGui::Begin("First context"); ImGui::TextUnformatted("Shared fonts at 1x"); ImGui::End();
		First.Tick(1.f / 60.f);
		Second.SetAsCurrent();
		TestEqual(TEXT("Second context DPI"), ImGui::GetStyle().FontScaleDpi, 2.f);
		ImGui::Begin("Second context");
		ImGui::PushFont(nullptr, 32.f);
		ImGui::TextUnformatted("Dynamic fonts at 2x");
		ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(10.f, 10.f), IM_COL32_WHITE, "Second shared atlas");
		ImGui::PopFont();
		ImGui::End();
		Second.Tick(1.f / 60.f);
		TestTrue(TEXT("First context produced draw data"), !First.GetDrawData().IsEmpty());
		TestTrue(TEXT("Second context produced draw data"), !Second.GetDrawData().IsEmpty());
		TestTrue(TEXT("Shared atlas uploaded"), Atlas.TexRef.GetTexID() != ImTextureID_Invalid);
	}
	TestEqual(TEXT("Contexts release shared atlas"), Atlas.RefCount, 0);
	FlushRenderingCommands();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiOrderedInputTest, "ImGui.Integration.OrderedPointerAndText", TestFlags)
bool FImGuiOrderedInputTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	FImGuiInputState Input;
	Input.SetMousePosition(FVector2D(10.f, 20.f));
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame(); ImGui::EndFrame();
	Input.SetMousePosition(FVector2D(110.f, 120.f));
	Input.SetMouseDown(EKeys::LeftMouseButton, true);
	Input.SetMousePosition(FVector2D(210.f, 220.f));
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Click is delivered"), ImGui::IsMouseClicked(0));
	TestEqual(TEXT("Click uses the position at press time"), ImGui::GetIO().MouseClickedPos[0].x, 110.f);
	TestEqual(TEXT("Click uses the position at press time Y"), ImGui::GetIO().MouseClickedPos[0].y, 120.f);
	ImGui::EndFrame();
	ImGui::NewFrame();
	TestEqual(TEXT("Later pointer motion stays after the click"), ImGui::GetIO().MousePos.x, 210.f);
	ImGui::EndFrame();
	Input.ResetMouse();
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	char Text[64] = "ab";
	for (int Frame = 0; Frame < 3; ++Frame)
	{
		ImGui::NewFrame();
		ImGui::Begin("Ordered text input");
		if (Frame == 0) ImGui::SetKeyboardFocusHere();
		ImGui::InputText("Text", Text, sizeof(Text));
		ImGui::End(); ImGui::EndFrame();
	}
	TestTrue(TEXT("A real text widget has keyboard focus"), ImGui::GetIO().WantTextInput);
	Input.AddCharacter(TEXT('x'));
	Input.SetKeyDown(EKeys::Left, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestEqual(TEXT("Text precedes navigation"), ImGui::GetIO().InputQueueCharacters.Size, 1);
	TestFalse(TEXT("Navigation waits for earlier text"), ImGui::IsKeyDown(ImGuiKey_LeftArrow));
	ImGui::Begin("Ordered text input"); ImGui::InputText("Text", Text, sizeof(Text)); ImGui::End();
	ImGui::EndFrame();
	ImGui::NewFrame();
	TestTrue(TEXT("Navigation is retained for the next frame"), ImGui::IsKeyPressed(ImGuiKey_LeftArrow, false));
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiPendingInputResetTest, "ImGui.Integration.PendingInputCancellation", TestFlags)
bool FImGuiPendingInputResetTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	FImGuiInputState Input;
	Input.SetKeyDown(EKeys::A, true); Input.SetKeyDown(EKeys::A, false);
	Input.AddCharacter(TEXT('a'));
	Input.SetMouseDown(EKeys::LeftMouseButton, true); Input.SetMouseDown(EKeys::LeftMouseButton, false);
	Input.ResetKeyboard(); Input.ResetMouse();
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	for (int Frame = 0; Frame < 3; ++Frame)
	{
		ImGui::NewFrame();
		TestFalse(TEXT("Locally queued press is cancelled"), ImGui::IsKeyPressed(ImGuiKey_A, false));
		TestFalse(TEXT("Locally queued click is cancelled"), ImGui::IsMouseClicked(0));
		TestEqual(TEXT("Locally queued text is cancelled"), ImGui::GetIO().InputQueueCharacters.Size, 0);
		ImGui::EndFrame();
	}
	Input.SetKeyDown(EKeys::B, true); Input.SetKeyDown(EKeys::B, false); Input.SetKeyDown(EKeys::B, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("First trickled press is consumed"), ImGui::IsKeyDown(ImGuiKey_B));
	ImGui::EndFrame();
	Input.SetMousePosition(FVector2D(30.f, 40.f)); Input.SetMouseDown(EKeys::LeftMouseButton, true);
	Input.ResetKeyboard();
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Reset clears consumed keys"), ImGui::IsKeyDown(ImGuiKey_B));
	TestTrue(TEXT("Keyboard reset preserves mouse events"), ImGui::IsMouseClicked(0));
	ImGui::EndFrame();
	ImGui::NewFrame();
	TestFalse(TEXT("Already queued later press cannot reappear"), ImGui::IsKeyPressed(ImGuiKey_B, false));
	ImGui::EndFrame();
	Input.SetMouseDown(EKeys::LeftMouseButton, false); Input.SetMouseDown(EKeys::LeftMouseButton, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame(); ImGui::EndFrame();
	Input.ResetMouse(); Input.SetKeyDown(EKeys::C, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Mouse reset clears pending click"), ImGui::IsMouseClicked(0));
	TestTrue(TEXT("Mouse reset preserves keyboard events"), ImGui::IsKeyDown(ImGuiKey_C));
	ImGui::EndFrame();
	Input.SetKeyDown(EKeys::D, true);
	Input.SetFocused(false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Focus loss is forwarded"), ImGui::GetIO().AppFocusLost);
	TestFalse(TEXT("Focus loss clears held keys"), ImGui::IsKeyDown(ImGuiKey_C));
	TestFalse(TEXT("Focus loss cancels pending keys"), ImGui::IsKeyPressed(ImGuiKey_D, false));
	ImGui::EndFrame();
	Input.SetFocused(true); Input.SetKeyDown(EKeys::D, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Focus is recovered"), ImGui::GetIO().AppFocusLost);
	TestTrue(TEXT("New input works after focus recovery"), ImGui::IsKeyPressed(ImGuiKey_D, false));
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiFastGamepadAndTouchTest, "ImGui.Integration.FastGamepadAndTouch", TestFlags)
bool FImGuiFastGamepadAndTouchTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	FImGuiInputState Input;
	Input.SetGamepad(true); Input.SetGamepadNavigationEnabled(true);
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Bottom, true);
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Bottom, false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Input state retains fast gamepad press"), ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false));
	ImGui::EndFrame();
	ImGui::NewFrame();
	TestTrue(TEXT("Input state retains fast gamepad release"), ImGui::IsKeyReleased(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	Input.SetGamepadNavigationAxis(EKeys::Gamepad_LeftX, 1.f);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Input state submits analog stick"), ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight));
	ImGui::EndFrame();
	Input.SetGamepad(false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Disconnect clears analog stick"), ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight));
	ImGui::EndFrame();
	Input.SetTouchPosition(FVector2D(50.f, 60.f)); Input.SetTouchDown(true);
	Input.SetTouchDown(false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input);
	Input.ClearUpdateState();
	bool bClicked = false, bReleased = false;
	for (int Frame = 0; Frame < 4; ++Frame)
	{
		ImGui::NewFrame();
		if (ImGui::IsMouseClicked(0))
		{
			bClicked = true;
			TestEqual(TEXT("Touch click X"), ImGui::GetIO().MouseClickedPos[0].x, 50.f);
			TestEqual(TEXT("Touch click Y"), ImGui::GetIO().MouseClickedPos[0].y, 60.f);
			TestEqual(TEXT("Touch source retained"), ImGui::GetIO().MouseSource, ImGuiMouseSource_TouchScreen);
		}
		bReleased |= ImGui::IsMouseReleased(0);
		ImGui::EndFrame();
	}
	TestTrue(TEXT("Fast touch press retained"), bClicked);
	TestTrue(TEXT("Fast touch release retained"), bReleased);
	Input.SetMousePosition(FVector2D(140.f, 150.f));
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame(); ImGui::EndFrame();
	Input.SetTouchPosition(FVector2D(90.f, 100.f)); Input.SetTouchDown(true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	for (int Frame = 0; Frame < 2; ++Frame) { ImGui::NewFrame(); ImGui::EndFrame(); }
	Input.SetTouchDown(false);
	Input.SetMousePosition(FVector2D(140.f, 150.f)); // Filtered while the touch release is pending.
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame(); ImGui::EndFrame();
	Input.SetMousePosition(FVector2D(140.f, 150.f)); // Slate supplies this before a click, without an idle frame.
	Input.SetMouseDown(EKeys::LeftMouseButton, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Immediate mouse click after touch is retained"), ImGui::IsMouseClicked(0));
	TestEqual(TEXT("Mouse handoff click uses physical X"), ImGui::GetIO().MouseClickedPos[0].x, 140.f);
	TestEqual(TEXT("Mouse handoff click uses physical Y"), ImGui::GetIO().MouseClickedPos[0].y, 150.f);
	TestEqual(TEXT("Mouse handoff restores its source"), ImGui::GetIO().MouseSource, ImGuiMouseSource_Mouse);
	ImGui::EndFrame();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiTextureIdentityTest, "ImGui.Integration.TextureIdentityAndGC", TestFlags)
bool FImGuiTextureIdentityTest::RunTest(const FString& Parameters)
{
	FTextureManager Manager;
	Manager.InitializeErrorTexture(FColor::Magenta);
	const TextureIndex First = Manager.CreatePlainTexture(TEXT("Identity_A"), 2, 2, FColor::Red);
	const ImTextureID OldId = Manager.GetTextureId(First);
	Manager.ReleaseTextureResources(First);
	const TextureIndex Second = Manager.CreatePlainTexture(TEXT("Identity_B"), 2, 2, FColor::Blue);
	TestEqual(TEXT("Released slot is reused"), Second, First);
	TestFalse(TEXT("Stale ID cannot resolve another texture"), Manager.IsValidTextureId(OldId));
	TestTrue(TEXT("Reused slot has a different identity"), Manager.GetTextureId(Second) != OldId);
	Manager.ReleaseTextureResources(Second);
	const TextureIndex Recreated = Manager.CreatePlainTexture(TEXT("Identity_A"), 2, 2, FColor::Green);
	TestFalse(TEXT("Same name cannot resurrect released ID"), Manager.IsValidTextureId(OldId));
	const ImTextureID CurrentId = Manager.GetTextureId(Recreated);
	Manager.CreatePlainTexture(TEXT("Identity_A"), 2, 2, FColor::White);
	TestEqual(TEXT("Updating a live registration preserves identity"), Manager.GetTextureId(Recreated), CurrentId);

	FImGuiModule& Module = FImGuiModule::Get();
	const FName ExternalName(TEXT("ImGuiTest_ExternalGC"));
	UTexture2D* External = UTexture2D::CreateTransient(2, 2);
	External->UpdateResource();
	TWeakObjectPtr<UTexture2D> WeakExternal(External);
	FImGuiTextureHandle ExternalHandle = Module.RegisterTexture(ExternalName, External);
	TestTrue(TEXT("External texture is valid before GC"), ExternalHandle.IsValid());
	TestFalse(TEXT("Registration leaves external ownership with caller"), External->IsRooted());
	FlushRenderingCommands();
	External = nullptr;
	CollectGarbage(RF_NoFlags);
	TestFalse(TEXT("Unowned external texture is collected"), WeakExternal.IsValid());
	TestFalse(TEXT("Collected texture invalidates handle"), ExternalHandle.IsValid());
	TestTrue(TEXT("Collected texture lookup returns a null handle"), Module.FindTextureHandle(ExternalName).IsNull());
	Module.ReleaseTexture(ExternalHandle);
	UTexture2D* Replacement = UTexture2D::CreateTransient(2, 2);
	Replacement->AddToRoot(); Replacement->UpdateResource();
	const FImGuiTextureHandle ReplacementHandle = Module.RegisterTexture(ExternalName, Replacement);
	TestTrue(TEXT("Collected registration can be released and recreated"), ReplacementHandle.IsValid());
	TestFalse(TEXT("Old public handle remains invalid after same-name recreation"), ExternalHandle.IsValid());
	Module.ReleaseTexture(ExternalHandle);
	TestTrue(TEXT("Stale release cannot remove replacement"), ReplacementHandle.IsValid());
	Module.ReleaseTexture(ReplacementHandle); Replacement->RemoveFromRoot();
	FlushRenderingCommands();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiRuntimeDPITest, "ImGui.Integration.RuntimeDPIAndTheme", TestFlags)
bool FImGuiRuntimeDPITest::RunTest(const FString& Parameters)
{
	FRestoreImGuiContext Restore;
	FTextureManager Manager;
	ImFontAtlas Atlas;
	FImGuiContextProxy Context(TEXT("ImGuiTest_RuntimeDPI"), -1, &Atlas, 1.f, Manager);
	Context.SetAsCurrent();
	ImGuiStyle& Style = ImGui::GetStyle();
	Style.WindowPadding = ImVec2(7.25f, 9.5f);
	Style.FrameRounding = 3.75f;
	Style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.2f, 0.3f, 0.9f);
	Style.FontScaleMain = 1.3f;
	Style.Alpha = 0.8f;
	for (int Cycle = 0; Cycle < 8; ++Cycle)
	{
		for (float Scale : { 1.5f, 2.f, 1.f })
		{
			Context.SetDPIScale(Scale);
			TestEqual(TEXT("Runtime font DPI follows scale"), Style.FontScaleDpi, Scale);
			TestEqual(TEXT("Fractional padding scales without drift"), Style.WindowPadding.x, 7.25f * Scale);
			TestEqual(TEXT("Rounding scales without drift"), Style.FrameRounding, 3.75f * Scale);
			TestEqual(TEXT("Theme survives DPI switch"), Style.Colors[ImGuiCol_WindowBg].z, 0.3f);
			TestEqual(TEXT("User font scale survives DPI switch"), Style.FontScaleMain, 1.3f);
			TestEqual(TEXT("Opacity survives DPI switch"), Style.Alpha, 0.8f);
		}
	}
	Context.SetDPIScale(2.f);
	Style.WindowPadding.y = 21.f;
	Style.FrameRounding = 9.f;
	Context.SetDPIScale(1.f);
	TestEqual(TEXT("Theme edits at scaled DPI become the new baseline"), Style.WindowPadding.y, 10.5f);
	TestEqual(TEXT("Scaled rounding edit is preserved"), Style.FrameRounding, 4.5f);
	Context.SetDPIScale(0.f);
	TestEqual(TEXT("Nonpositive DPI is normalized"), Context.GetDPIScale(), 1.f);
	Context.SetDPIScale(std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("Nonfinite DPI is normalized"), Context.GetDPIScale(), 1.f);
	TestTrue(TEXT("Reset-render-state callback is advertised"), ImGui::GetPlatformIO().DrawCallback_ResetRenderState != nullptr);
	if (ImDrawCallback Reset = ImGui::GetPlatformIO().DrawCallback_ResetRenderState)
	{
		ImGui::GetForegroundDrawList()->AddCallback(Reset, nullptr);
	}
	ImGui::GetForegroundDrawList()->AddText(ImVec2(10.f, 10.f), IM_COL32_WHITE, "Runtime DPI theme");
	Context.Tick(1.f / 60.f);
	TestTrue(TEXT("Context still renders after DPI changes"), !Context.GetDrawData().IsEmpty());
	FlushRenderingCommands();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiAtlasGrowthTest, "ImGui.Integration.AtlasGrowthAndContextLifetime", TestFlags)
bool FImGuiAtlasGrowthTest::RunTest(const FString& Parameters)
{
	FRestoreImGuiContext Restore;
	const auto SavedFrameNumber = GFrameNumber;
	ON_SCOPE_EXIT { GFrameNumber = SavedFrameNumber; };
	FTextureManager Manager;
	ImFontAtlas Atlas;
	Atlas.TexMinWidth = Atlas.TexMinHeight = 128;
	Atlas.AddFontDefault();
	const bool bReadGPU = GDynamicRHI && !FString(GDynamicRHI->GetName()).StartsWith(TEXT("Null"));
	const auto CheckWhitePixel = [this, bReadGPU](UTexture2D* Texture, const ImVec2& UV, int Width, int Height)
	{
		if (!bReadGPU) return;
		FlushRenderingCommands();
		if (!TestNotNull(TEXT("Atlas UObject survives until readback"), Texture)) return;
		const FTextureRHIRef Resource = Texture->GetResource()->TextureRHI;
		const int X = static_cast<int>(UV.x * Width), Y = static_cast<int>(UV.y * Height);
		TArray<FColor> Pixels;
		ENQUEUE_RENDER_COMMAND(ImGuiAtlasReadback)([Resource, X, Y, &Pixels](FRHICommandListImmediate& Commands)
		{
			FReadSurfaceDataFlags Flags(RCM_UNorm);
			Flags.SetLinearToGamma(false);
			Commands.ReadSurfaceData(Resource, FIntRect(X, Y, X + 1, Y + 1), Pixels, Flags);
		});
		FlushRenderingCommands();
		if (TestEqual(TEXT("Atlas white-pixel readback count"), Pixels.Num(), 1))
		{
			TestEqual(TEXT("Atlas upload preserves its solid white texel"), Pixels[0], FColor::White);
		}
	};
	{
		FImGuiContextProxy First(TEXT("ImGuiTest_GrowingFirst"), -1, &Atlas, 1.f, Manager);
		FImGuiContextProxy Second(TEXT("ImGuiTest_GrowingSecond"), -1, &Atlas, 1.f, Manager);
		const auto DrawText = [](FImGuiContextProxy& Context, float Size)
		{
			Context.SetAsCurrent();
			ImGui::GetForegroundDrawList()->AddText(ImGui::GetFont(), Size, ImVec2(10.f, 10.f), IM_COL32_WHITE,
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789");
		};
		DrawText(First, 13.f); First.Tick(1.f / 60.f);
		DrawText(Second, 13.f); Second.Tick(1.f / 60.f);
		const ImTextureID OldId = Atlas.TexRef.GetTexID();
		TestTrue(TEXT("Initial atlas has a valid resource"), Manager.IsValidTextureId(OldId));
		TestFalse(TEXT("Second context has staged data"), Second.GetDrawData().IsEmpty());
		const ImTextureID StagedId = Second.GetDrawData()[0].GetCommand(0, FTransform2D()).TextureId;
		TestEqual(TEXT("Second context staged the original atlas"), StagedId, OldId);
		const int InitialWidth = Atlas.TexData->Width, InitialHeight = Atlas.TexData->Height;
		const ImVec2 OldWhitePixelUV = Atlas.TexUvWhitePixel;
		const TWeakObjectPtr<UTexture2D> OldTexture(static_cast<UTexture2D*>(Atlas.TexData->BackendUserData));
		++GFrameNumber;
		DrawText(First, 96.f); DrawText(First, 192.f); First.Tick(1.f / 60.f);
		TestTrue(TEXT("Dynamic fonts trigger real atlas growth"), Atlas.TexData->Width > InitialWidth || Atlas.TexData->Height > InitialHeight);
		TestTrue(TEXT("Growth creates a new texture identity"), Atlas.TexRef.GetTexID() != OldId);
		TestTrue(TEXT("Old staged context retains its resource during handoff"), Manager.IsValidTextureId(StagedId));
		FlushRenderingCommands(); CollectGarbage(RF_NoFlags);
		TestTrue(TEXT("Pending atlas resource survives GC"), Manager.IsValidTextureId(StagedId));
		CheckWhitePixel(OldTexture.Get(), OldWhitePixelUV, InitialWidth, InitialHeight);
		CheckWhitePixel(static_cast<UTexture2D*>(Atlas.TexData->BackendUserData), Atlas.TexUvWhitePixel, Atlas.TexData->Width, Atlas.TexData->Height);
		for (int Frame = 0; Frame < 4; ++Frame)
		{
			++GFrameNumber;
			DrawText(First, 13.f); First.Tick(1.f / 60.f);
			if (Frame < 2) TestTrue(TEXT("Two handoff frames retain the old atlas"), Manager.IsValidTextureId(StagedId));
		}
		TestFalse(TEXT("Unused atlas is eventually released by actual frame aging"), Manager.IsValidTextureId(StagedId));
		TestEqual(TEXT("Staged command remains a resolved ID after atlas metadata expires"), Second.GetDrawData()[0].GetCommand(0, FTransform2D()).TextureId, StagedId);
		DrawText(Second, 13.f); Second.Tick(1.f / 60.f);
		TestTrue(TEXT("Slower context recovers with current atlas"), Manager.IsValidTextureId(Second.GetDrawData()[0].GetCommand(0, FTransform2D()).TextureId));
	}
	TestEqual(TEXT("Growing contexts release shared atlas references"), Atlas.RefCount, 0);
	FlushRenderingCommands(); CollectGarbage(RF_NoFlags);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FImGuiGamepadConnectionTest, "ImGui.Integration.GamepadConnectionTransitions", TestFlags)
bool FImGuiGamepadConnectionTest::RunTest(const FString& Parameters)
{
	FTestImGuiContext Context;
	FImGuiInputState Input;
	Input.SetGamepadNavigationEnabled(true);
	// Simulate a late connection confirmation after an earlier copy saw no device.
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Bottom, true);
	Input.SetGamepadNavigationAxis(EKeys::Gamepad_LeftX, 1.f);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Unconfirmed device does not submit navigation"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	Input.SetGamepad(true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("Connection restores its first held button"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	TestTrue(TEXT("Connection restores its first held axis"), ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight));
	ImGui::EndFrame();
	Input.SetGamepad(false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Disconnect clears restored held button"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	// The production pre-update refresh confirms attachment after input arrival but before consumption.
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Left, true);
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Left, false);
	Input.SetGamepad(true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestTrue(TEXT("First-frame connected fast tap press is retained"), ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft, false));
	ImGui::EndFrame();
	ImGui::NewFrame();
	TestTrue(TEXT("First-frame connected fast tap release is retained"), ImGui::IsKeyReleased(ImGuiKey_GamepadFaceLeft));
	ImGui::EndFrame();
	Input.SetGamepadNavigationKey(EKeys::Gamepad_FaceButton_Bottom, true);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame(); ImGui::EndFrame();
	Input.SetGamepadNavigationEnabled(false);
	ImGuiInterops::CopyInput(ImGui::GetIO(), Input); Input.ClearUpdateState();
	ImGui::NewFrame();
	TestFalse(TEXT("Disabling navigation clears held inputs"), ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown));
	ImGui::EndFrame();
	return true;
}
#endif

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#if WITH_IMGUI
#include "ImGuiModule.h"
#include "ImGuiDelegates.h"
#include "UnrealClient.h"
#include "imgui.h"

class FImGuiValidationModule : public FDefaultGameModuleImpl
{
public:
	void StartupModule() override
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("ImGuiValidationSmoke")))
		{
			DrawHandle = FImGuiDelegates::OnMultiContextDebug().AddRaw(this, &FImGuiValidationModule::Draw);
			TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FImGuiValidationModule::Tick));
		}
	}
	void ShutdownModule() override
	{
		if (DrawHandle.IsValid() && FImGuiModule::IsAvailable()) FImGuiDelegates::OnMultiContextDebug().Remove(DrawHandle);
		if (TickHandle.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		if (UTexture2D* Texture = Checker.Get()) Texture->RemoveFromRoot();
	}
private:
	void Draw()
	{
		ImGui::SetNextWindowPos(ImVec2(30.f, 30.f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(560.f, 600.f), ImGuiCond_Always);
		ImGui::Begin("UnrealImGui5 - UE 5.8 validation", nullptr, ImGuiWindowFlags_NoSavedSettings);
		ImGui::Text("Dear ImGui %s", ImGui::GetVersion());
		ImGui::TextUnformatted("Unreal Engine 5.8 / Slate / D3D12");
		ImGui::Separator();
		for (float Size : { 13.f, 20.f, 32.f })
		{
			ImGui::PushFont(nullptr, Size);
			ImGui::Text("Dynamic font size %.0f px", Size);
			ImGui::PopFont();
		}
		static char Buffer[128] = "Keyboard input and clipboard";
		ImGui::InputText("Input", Buffer, sizeof(Buffer));
		static float Value = 0.5f;
		ImGui::SliderFloat("Slider", &Value, 0.f, 1.f);
		static bool bChecked = true;
		ImGui::Checkbox("Checkbox", &bChecked);
		if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_Borders))
		{
			for (int Row = 0; Row < 3; ++Row)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn(); ImGui::Text("Row %d", Row);
				ImGui::TableNextColumn(); ImGui::TextUnformatted("Table rendering");
			}
			ImGui::EndTable();
		}
		if (Checker.IsValid())
		{
			ImGui::TextUnformatted("Registered Unreal texture:");
			ImGui::Image(TextureHandle, ImVec2(96.f, 96.f));
		}
		ImGui::End();
		ImGui::SetNextWindowPos(ImVec2(620.f, 30.f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(600.f, 620.f), ImGuiCond_Always);
		ImGui::ShowDemoWindow();
		++DrawCount;
	}
	bool Tick(float DeltaTime)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (!Checker.IsValid())
			{
				Checker = UTexture2D::CreateTransient(16, 16);
				Checker->AddToRoot();
				FColor* Pixels = static_cast<FColor*>(Checker->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
				for (int Y = 0; Y < 16; ++Y) for (int X = 0; X < 16; ++X)
					Pixels[X + Y * 16] = ((X / 4 + Y / 4) % 2) ? FColor(40, 100, 220) : FColor(220, 100, 40);
				Checker->GetPlatformData()->Mips[0].BulkData.Unlock();
				Checker->UpdateResource();
				TextureHandle = FImGuiModule::Get().RegisterTexture(TEXT("ImGuiValidationChecker"), Checker.Get());
			}
			++ViewportTicks;
			if (ViewportTicks == 120)
			{
				const FString Screenshot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Validation/UE58-ImGui-Slate.png"));
				FScreenshotRequest::RequestScreenshot(Screenshot, true, false);
				UE_LOG(LogTemp, Display, TEXT("ImGui validation screenshot requested; DrawCount=%d"), DrawCount);
			}
			if (ViewportTicks > 160)
			{
				UE_LOG(LogTemp, Display, TEXT("ImGui validation finished; ViewportTicks=%d DrawCount=%d"), ViewportTicks, DrawCount);
				FPlatformMisc::RequestExit(false);
				return false;
			}
		}
		return true;
	}
	FDelegateHandle DrawHandle;
	FTSTicker::FDelegateHandle TickHandle;
	TWeakObjectPtr<UTexture2D> Checker;
	FImGuiTextureHandle TextureHandle;
	int ViewportTicks = 0;
	int DrawCount = 0;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FImGuiValidationModule, ImGuiValidation, "ImGuiValidation");
#else
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ImGuiValidation, "ImGuiValidation");
#endif

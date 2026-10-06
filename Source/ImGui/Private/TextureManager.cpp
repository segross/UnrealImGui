// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "TextureManager.h"
#include "ImGuiInteroperability.h"

#include <Engine/Texture2D.h>
#include <Framework/Application/SlateApplication.h>

#include <algorithm>

namespace
{
	uint32 AllocateTextureGeneration()
	{
		static uint32 NextGeneration = 1;
		const uint32 Generation = NextGeneration++;
		checkf(Generation != 0, TEXT("ImGui texture generation counter exhausted."));
		return Generation;
	}
	// The render thread owns this snapshot; ImGui may resize or free its pixels in the next frame.
	uint8* CopyImGuiPixels(const ImTextureData& Data, int32 X, int32 Y, int32 Width, int32 Height)
	{
		uint8* Pixels = new uint8[static_cast<SIZE_T>(Width) * Height * 4];
		for (int32 Row = 0; Row < Height; ++Row)
		{
			const uint8* Source = Data.Pixels + ((Y + Row) * Data.Width + X) * Data.BytesPerPixel;
			uint8* Destination = Pixels + Row * Width * 4;
			for (int32 Column = 0; Column < Width; ++Column)
			{
				// Unreal's transient textures use BGRA, while Dear ImGui provides RGBA or alpha.
				Destination[0] = Data.Format == ImTextureFormat_RGBA32 ? Source[2] : 255;
				Destination[1] = Data.Format == ImTextureFormat_RGBA32 ? Source[1] : 255;
				Destination[2] = Data.Format == ImTextureFormat_RGBA32 ? Source[0] : 255;
				Destination[3] = Data.Format == ImTextureFormat_RGBA32 ? Source[3] : Source[0];
				Source += Data.BytesPerPixel;
				Destination += 4;
			}
		}
		return Pixels;
	}
}

void FTextureManager::UpdateImGuiTextures(ImDrawData& DrawData)
{
	if (DrawData.Textures && FSlateApplication::IsInitialized())
	{
		for (ImTextureData* TextureData : *DrawData.Textures)
		{
			UpdateImGuiTexture(*TextureData);
		}
	}
}

void FTextureManager::UpdateImGuiTexture(ImTextureData& Data)
{
	if (Data.Status == ImTextureStatus_WantCreate)
	{
		const FName Name(*FString::Printf(TEXT("ImGui_Atlas_%p"), &Data));
		uint8* Pixels = CopyImGuiPixels(Data, 0, 0, Data.Width, Data.Height);
		const TextureIndex Index = CreateTexture(Name, Data.Width, Data.Height, 4, Pixels, [](uint8* Buffer) { delete[] Buffer; });
		Data.SetTexID(GetTextureId(Index));
		Data.BackendUserData = TextureResources[Index].GetTexture();
		Data.SetStatus(ImTextureStatus_OK);
	}
	else if (Data.Status == ImTextureStatus_WantUpdates)
	{
		const TextureIndex Index = ImGuiInterops::ToTextureIndex(Data.GetTexID());
		check(IsValidTextureId(Data.GetTexID()));
		const ImTextureRect& Rect = Data.UpdateRect;
		if (Rect.w > 0 && Rect.h > 0)
		{
			uint8* Pixels = CopyImGuiPixels(Data, Rect.x, Rect.y, Rect.w, Rect.h);
			FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(Rect.x, Rect.y, 0, 0, Rect.w, Rect.h);
			TextureResources[Index].GetTexture()->UpdateTextureRegions(0, 1, Region, Rect.w * 4, 4, Pixels,
				[](uint8* Buffer, const FUpdateTextureRegion2D* UpdateRegion) { delete[] Buffer; delete UpdateRegion; });
		}
		Data.SetStatus(ImTextureStatus_OK);
	}
	else if (Data.Status == ImTextureStatus_WantDestroy && Data.UnusedFrames > 2)
	{
		// Slate may still hold staged draw commands from another PIE context.
		ReleaseImGuiTexture(Data);
	}
}

void FTextureManager::ReleaseImGuiTexture(ImTextureData& Data)
{
	const TextureIndex Index = ImGuiInterops::ToTextureIndex(Data.GetTexID());
	if (IsRegisteredTextureId(Data.GetTexID()))
	{
		ReleaseTextureResources(Index);
	}
	Data.SetTexID(ImTextureID_Invalid);
	Data.BackendUserData = nullptr;
	Data.SetStatus(ImTextureStatus_Destroyed);
}


ImTextureID FTextureManager::GetTextureId(TextureIndex Index) const
{
	return IsValidTexture(Index) ? ImGuiInterops::ToImTextureID(Index, TextureResources[Index].GetGeneration()) : ImTextureID_Invalid;
}

bool FTextureManager::IsRegisteredTextureId(ImTextureID Id) const
{
	const TextureIndex Index = ImGuiInterops::ToTextureIndex(Id);
	return IsInRange(Index) && TextureResources[Index].GetName() != NAME_None && TextureResources[Index].GetGeneration() == ImGuiInterops::ToTextureGeneration(Id);
}

bool FTextureManager::IsValidTextureId(ImTextureID Id) const
{
	return IsRegisteredTextureId(Id) && IsValidTexture(ImGuiInterops::ToTextureIndex(Id));
}

void FTextureManager::InitializeErrorTexture(const FColor& Color)
{
	CreatePlainTextureInternal(NAME_ErrorTexture, 2, 2, Color);
}

TextureIndex FTextureManager::CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup)
{
	checkf(Name != NAME_None, TEXT("Trying to create a texture with a name 'NAME_None' is not allowed."));

	return CreateTextureInternal(Name, Width, Height, SrcBpp, SrcData, SrcDataCleanup);
}

TextureIndex FTextureManager::CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color)
{
	checkf(Name != NAME_None, TEXT("Trying to create a texture with a name 'NAME_None' is not allowed."));

	return CreatePlainTextureInternal(Name, Width, Height, Color);
}

TextureIndex FTextureManager::CreateTextureResources(const FName& Name, UTexture2D* Texture)
{
	checkf(Name != NAME_None, TEXT("Trying to create texture resources with a name 'NAME_None' is not allowed."));
	checkf(Texture, TEXT("Null Texture."));

	// Create an entry for the texture.
	return AddTextureEntry(Name, Texture, false);
}

void FTextureManager::ReleaseTextureResources(TextureIndex Index)
{
	checkf(IsInRange(Index), TEXT("Invalid texture index %d. Texture resources array has %d entries total."), Index, TextureResources.Num());

	TextureResources[Index] = {};
}

TextureIndex FTextureManager::CreateTextureInternal(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup)
{
	// Create a texture.
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height);

	// Create a new resource for that texture.
	Texture->UpdateResource();

	// Update texture data.
	FUpdateTextureRegion2D* TextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, Width, Height);
	auto DataCleanup = [SrcDataCleanup](uint8* Data, const FUpdateTextureRegion2D* UpdateRegion)
	{
		SrcDataCleanup(Data);
		delete UpdateRegion;
	};
	Texture->UpdateTextureRegions(0, 1u, TextureRegion, SrcBpp * Width, SrcBpp, SrcData, DataCleanup);

	// Create an entry for the texture.
	if (Name == NAME_ErrorTexture)
	{
		ErrorTexture = { Name, Texture, true };
		return INDEX_ErrorTexture;
	}
	else
	{
		return AddTextureEntry(Name, Texture, true);
	}
}

TextureIndex FTextureManager::CreatePlainTextureInternal(const FName& Name, int32 Width, int32 Height, const FColor& Color)
{
	// Create buffer with raw data.
	const uint32 ColorPacked = Color.ToPackedARGB();
	const uint32 Bpp = sizeof(ColorPacked);
	const uint32 SizeInPixels = Width * Height;
	const uint32 SizeInBytes = SizeInPixels * Bpp;
	uint8* SrcData = new uint8[SizeInBytes];
	std::fill(reinterpret_cast<uint32*>(SrcData), reinterpret_cast<uint32*>(SrcData) + SizeInPixels, ColorPacked);
	auto SrcDataCleanup = [](uint8* Data) { delete[] Data; };

	// Create new texture from raw data.
	return CreateTextureInternal(Name, Width, Height, Bpp, SrcData, SrcDataCleanup);
}

TextureIndex FTextureManager::AddTextureEntry(const FName& Name, UTexture2D* Texture, bool bAddToRoot)
{
	// Try to find an entry with that name.
	TextureIndex Index = FindTextureIndex(Name);
	// Updating an existing named registration preserves its identity; release/reuse creates a new one.
	const uint32 Generation = Index != INDEX_NONE ? TextureResources[Index].GetGeneration() : AllocateTextureGeneration();

	// If this is a new name, try to find an entry to reuse.
	if (Index == INDEX_NONE)
	{
		Index = FindTextureIndex(NAME_None);
	}

	// Either update/reuse an entry or add a new one.
	if (Index != INDEX_NONE)
	{
		TextureResources[Index] = { Name, Texture, bAddToRoot, Generation };
		return Index;
	}
	else
	{
		return TextureResources.Emplace(Name, Texture, bAddToRoot, Generation);
	}
}

FTextureManager::FTextureEntry::FTextureEntry(const FName& InName, UTexture2D* InTexture, bool bAddToRoot, uint32 InGeneration)
	: Name(InName)
	, Generation(InGeneration)
	, bOwnsTexture(bAddToRoot)
	, Texture(InTexture)
{
	checkf(InTexture, TEXT("Null texture."));

	if (bAddToRoot)
	{
		// Add texture to the root to prevent garbage collection.
		InTexture->AddToRoot();
	}

	// Create brush and resource handle for input texture.
	Brush.SetResourceObject(InTexture);
	CachedResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
}

FTextureManager::FTextureEntry::~FTextureEntry()
{
	Reset(true);
}

FTextureManager::FTextureEntry& FTextureManager::FTextureEntry::operator=(FTextureEntry&& Other)
{
	// Release old resources if allocated.
	Reset(true);

	// Move data and ownership to this instance.
	Name = MoveTemp(Other.Name);
	Generation = Other.Generation;
	bOwnsTexture = Other.bOwnsTexture;
	Texture = MoveTemp(Other.Texture);
	Brush = MoveTemp(Other.Brush);
	CachedResourceHandle = MoveTemp(Other.CachedResourceHandle);

	// Reset the other entry (without releasing resources which are already moved to this instance) to remove tracks
	// of ownership and mark it as empty/reusable.
	Other.Reset(false);

	return *this;
}

const FSlateResourceHandle& FTextureManager::FTextureEntry::GetResourceHandle() const
{
	if (!CachedResourceHandle.IsValid() && Brush.HasUObject())
	{
		CachedResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
	}
	return CachedResourceHandle;
}

void FTextureManager::FTextureEntry::Reset(bool bReleaseResources)
{
	if (bReleaseResources)
	{
		// Release brush.
		if (Texture.IsValid() && Brush.HasUObject() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource(Brush);
		}

		// Remove texture from root to allow for garbage collection (it might be invalid, if we never set it
		// or this is an application shutdown).
		if (bOwnsTexture && Texture.IsValid())
		{
			Texture->RemoveFromRoot();
		}
	}

	// We use empty name to mark unused entries.
	Name = NAME_None;
	Generation = 0;
	bOwnsTexture = false;

	// Clean fields to make sure that we don't reference released or moved resources.
	Texture.Reset();
	Brush = FSlateNoResource();
	CachedResourceHandle = FSlateResourceHandle();
}

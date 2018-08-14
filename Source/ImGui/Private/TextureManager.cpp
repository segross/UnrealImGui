// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "TextureManager.h"

#include <algorithm>


TextureIndex FTextureManager::CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup)
{
	checkf(FindTextureIndex(Name) == INDEX_NONE, TEXT("Trying to create texture using resource name '%s' that is already registered."), *Name.ToString());

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

	// Create a new entry for the texture.
	return TextureResources.Emplace(Name, Texture);
}

TextureIndex FTextureManager::CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color)
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
	return CreateTexture(Name, Width, Height, Bpp, SrcData, SrcDataCleanup);
}

FTextureManager::FTextureEntry::FTextureEntry(const FName& InName, UTexture2D* InTexture)
	: Name{ InName }
	, Texture{ InTexture }
{
	// Add texture to root to prevent garbage collection.
	Texture->AddToRoot();

	// Create brush and resource handle for input texture.
	Brush.SetResourceObject(InTexture);
	ResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
}

FTextureManager::FTextureEntry::~FTextureEntry()
{
	// Release brush.
	if (Brush.HasUObject() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource(Brush);
	}

	// Remove texture from root to allow for garbage collection (it might be already invalid if this is application
	// shutdown).
	if (Texture.IsValid())
	{
		Texture->RemoveFromRoot();
	}
}

// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "TextureManager.h"

#include <algorithm>


void FTextureManager::InitializeErrorTexture(const FColor& Color)
{
	CreatePlainTextureInternal(NAME_ErrorTexture, 2, 2, Color);
}

TextureIndex FTextureManager::CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup)
{
	checkf(Name != NAME_None, TEXT("Trying to create a texture with a name 'NAME_None' is not allowed."));
	checkf(FindTextureIndex(Name) == INDEX_NONE, TEXT("Trying to create texture using name '%s' that is already registered."), *Name.ToString());

	return CreateTextureInternal(Name, Width, Height, SrcBpp, SrcData, SrcDataCleanup);
}

TextureIndex FTextureManager::CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color)
{
	checkf(Name != NAME_None, TEXT("Trying to create a texture with a name 'NAME_None' is not allowed."));
	checkf(FindTextureIndex(Name) == INDEX_NONE, TEXT("Trying to create texture using name '%s' that is already registered."), *Name.ToString());

	return CreatePlainTextureInternal(Name, Width, Height, Color);
}

TextureIndex FTextureManager::CreateTextureResources(const FName& Name, UTexture2D* Texture, bool bMakeUnique)
{
	checkf(Name != NAME_None, TEXT("Trying to create texture resources with a name 'NAME_None' is not allowed."));
	checkf(Texture, TEXT("Null Texture."));

	if (bMakeUnique)
	{
		checkf(FindTextureIndex(Name) == INDEX_NONE, TEXT("Trying to create texture resources using name '%s' that is already registered.")
			TEXT(" Consider using different name or set bMakeUnique parameter to false."), *Name.ToString());
	}

	// Create an entry for the texture.
	return AddTextureEntry(Name, Texture, false, true);
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
		return AddTextureEntry(Name, Texture, true, false);
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

TextureIndex FTextureManager::AddTextureEntry(const FName& Name, UTexture2D* Texture, bool bAddToRoot, bool bUpdate)
{
	// If we update try to find entry with that name.
	TextureIndex Index = bUpdate ? FindTextureIndex(Name) : INDEX_NONE;

	// If we didn't find, try to find and entry to reuse.
	if (Index == INDEX_NONE)
	{
		Index = FindTextureIndex(NAME_None);
	}

	// Either update/reuse entry or add a new one.
	if (Index != INDEX_NONE)
	{
		TextureResources[Index] = { Name, Texture, bAddToRoot };
		return Index;
	}
	else
	{
		return TextureResources.Emplace(Name, Texture, bAddToRoot);
	}
}

FTextureManager::FTextureEntry::FTextureEntry(const FName& InName, UTexture2D* InTexture, bool bAddToRoot)
	: Name(InName)
{
	checkf(InTexture, TEXT("Null texture."));

	if (bAddToRoot)
	{
		// Get pointer only for textures that we added to root, so we can later release them.
		Texture = InTexture;
		// Add texture to the root to prevent garbage collection.
		InTexture->AddToRoot();
	}

	// Create brush and resource handle for input texture.
	Brush.SetResourceObject(InTexture);
	ResourceHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush);
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
	Texture = MoveTemp(Other.Texture);
	Brush = MoveTemp(Other.Brush);
	ResourceHandle = MoveTemp(Other.ResourceHandle);

	// Reset the other entry (without releasing resources which are already moved to this instance) to remove tracks
	// of ownership and mark it as empty/reusable.
	Other.Reset(false);

	return *this;
}

void FTextureManager::FTextureEntry::Reset(bool bReleaseResources)
{
	if (bReleaseResources)
	{
		// Release brush.
		if (Brush.HasUObject() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().GetRenderer()->ReleaseDynamicResource(Brush);
		}

		// Remove texture from root to allow for garbage collection (it might be invalid, if we never set it
		// or this is an application shutdown).
		if (Texture.IsValid())
		{
			Texture->RemoveFromRoot();
		}
	}

	// We use empty name to mark unused entries.
	Name = NAME_None;

	// Clean fields to make sure that we don't reference released or moved resources.
	Texture.Reset();
	Brush = FSlateNoResource();
	ResourceHandle = FSlateResourceHandle();
}

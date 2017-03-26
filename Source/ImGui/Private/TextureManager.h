// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Core.h>
#include <Styling/SlateBrush.h>
#include <Textures/SlateShaderResource.h>


// Index type to be used as a texture handle.
using TextureIndex = int32;

// Manager for textures resources which can be referenced by a unique name or index.
// Name is primarily for lookup and index provides a direct access to resources.
class FTextureManager
{
public:

	// Creates an empty manager.
	FTextureManager() = default;

	// Copying is disabled to protected resource ownership.
	FTextureManager(const FTextureManager&) = delete;
	FTextureManager& operator=(const FTextureManager&) = delete;

	// Moving transfers ownership and leaves source empty.
	FTextureManager(FTextureManager&&) = default;
	FTextureManager& operator=(FTextureManager&&) = default;

	// Find texture index by name.
	// @param Name - The name of a texture to find
	// @returns The index of a texture with given name or INDEX_NONE if there is no such texture
	TextureIndex FindTextureIndex(const FName& Name) const
	{
		return TextureResources.IndexOfByPredicate([&](const auto& Entry) { return Entry.Name == Name; });
	}

	// Get the name of a texture at given index. Throws exception if index is out of range.
	// @param Index - Index of a texture
	// @returns The name of a texture at given index
	FORCEINLINE FName GetTextureName(TextureIndex Index) const
	{
		return TextureResources[Index].Name;
	}

	// Get the Slate Resource Handle to a texture at given index. Throws exception if index is out of range.
	// @param Index - Index of a texture
	// @returns The Slate Resource Handle for a texture at given index
	FORCEINLINE const FSlateResourceHandle& GetTextureHandle(TextureIndex Index) const
	{
		return TextureResources[Index].ResourceHandle;
	}

	// Create a texture from raw data. Throws exception if there is already a texture with that name.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param SrcBpp - The size in bytes of one pixel
	// @param SrcData - The source data
	// @param bDeleteSrcData - If true, we should delete source data after creating a texture
	// @returns The index of a texture that was created
	TextureIndex CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, bool bDeleteSrc = false);

	// Create a plain texture. Throws exception if there is already a texture with that name.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param Color - The texture color
	// @returns The index of a texture that was created
	TextureIndex CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color);

private:

	// Entry for texture resources. Only supports explicit construction.
	struct FTextureEntry
	{
		FTextureEntry(const FName& InName, UTexture2D* InTexture);
		~FTextureEntry();

		// Copying is not supported.
		FTextureEntry(const FTextureEntry&) = delete;
		FTextureEntry& operator=(const FTextureEntry&) = delete;

		// We rely on TArray and don't implement custom move semantics.
		FTextureEntry(FTextureEntry&&) = delete;
		FTextureEntry& operator=(FTextureEntry&&) = delete;

		FName Name = NAME_None;
		UTexture2D* Texture = nullptr;
		FSlateBrush Brush;
		FSlateResourceHandle ResourceHandle;
	};

	TArray<FTextureEntry> TextureResources;
};

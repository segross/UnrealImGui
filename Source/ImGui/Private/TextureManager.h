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

	// Initialize error texture that will be used for rendering textures without registered resources. Can be called
	// multiple time, if color needs to be changed.
	// Note: Because of any-time module loading and lazy resources initialization goals we can't simply call it from
	// constructor.
	// @param Color - The color of the error texture
	void InitializeErrorTexture(const FColor& Color);

	// Find texture index by name.
	// @param Name - The name of a texture to find
	// @returns The index of a texture with given name or INDEX_NONE if there is no such texture
	TextureIndex FindTextureIndex(const FName& Name) const
	{
		return TextureResources.IndexOfByPredicate([&](const auto& Entry) { return Entry.Name == Name; });
	}

	// Get the name of a texture at given index. Returns NAME_None, if index is out of range.
	// @param Index - Index of a texture
	// @returns The name of a texture at given index or NAME_None if index is out of range.
	FName GetTextureName(TextureIndex Index) const
	{
		return IsInRange(Index) ? TextureResources[Index].Name : NAME_None;
	}

	// Get the Slate Resource Handle to a texture at given index. If index is out of range or resources are not valid
	// it returns a handle to the error texture.
	// @param Index - Index of a texture
	// @returns The Slate Resource Handle for a texture at given index or to error texture, if no valid resources were
	// found at given index
	const FSlateResourceHandle& GetTextureHandle(TextureIndex Index) const
	{
		return IsValidTexture(Index) ? TextureResources[Index].ResourceHandle : ErrorTexture.ResourceHandle;
	}

	// Create a texture from raw data. Throws exception if there is already a texture with that name.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param SrcBpp - The size in bytes of one pixel
	// @param SrcData - The source data
	// @param SrcDataCleanup - Optional function called to release source data after texture is created (only needed, if data need to be released)
	// @returns The index of a texture that was created
	TextureIndex CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup = [](uint8*) {});

	// Create a plain texture. Throws exception if there is already a texture with that name.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param Color - The texture color
	// @returns The index of a texture that was created
	TextureIndex CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color);

	// Create Slate resources to an existing texture, managed externally. As part of an external interface it allows
	// to loosen resource verification policy. By default (consistently with other create function) it protects from
	// creating resources with name that is already registered. If bMakeUnique is false, then existing resources are
	// updated/replaced. Throws exception, if name argument is NAME_None or texture is null.
	// @param Name - The texture name
	// @param Texture - The texture
	// @param bMakeUnique - If true (default) and there is already a texture with given name, then exception is thrown,
	//     otherwise existing resources are updated.
	// @returns The index to created/updated texture resources
	TextureIndex CreateTextureResources(const FName& Name, UTexture2D* Texture, bool bMakeUnique = true);

	// Release resources for given texture. Ignores invalid indices.
	// @param Index - The index of a texture resources
	void ReleaseTextureResources(TextureIndex Index);

private:

	// See CreateTexture for general description.
	// Internal implementations doesn't validate name or resource uniqueness. Instead it uses NAME_ErrorTexture
	// (aka NAME_None) and INDEX_ErrorTexture (aka INDEX_NONE) to identify ErrorTexture.
	TextureIndex CreateTextureInternal(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup = [](uint8*) {});

	// See CreatePlainTexture for general description.
	// Internal implementations doesn't validate name or resource uniqueness. Instead it uses NAME_ErrorTexture
	// (aka NAME_None) and INDEX_ErrorTexture (aka INDEX_NONE) to identify ErrorTexture.
	TextureIndex CreatePlainTextureInternal(const FName& Name, int32 Width, int32 Height, const FColor& Color);

	// Add or reuse texture entry.
	// @param Name - The texture name
	// @param Texture - The texture
	// @param bAddToRoot - If true, we should add texture to root to prevent garbage collection (use for own textures)
	// @returns The index of the entry that we created or reused
	TextureIndex AddTextureEntry(const FName& Name, UTexture2D* Texture, bool bAddToRoot, bool bUpdate);

	// Check whether index is in range allocated for TextureResources (it doesn't mean that resources are valid).
	FORCEINLINE bool IsInRange(TextureIndex Index) const
	{
		return static_cast<uint32>(Index) < static_cast<uint32>(TextureResources.Num());
	}

	// Check whether index is in range and whether texture resources are valid (using NAME_None sentinel).
	FORCEINLINE bool IsValidTexture(TextureIndex Index) const
	{
		return IsInRange(Index) && TextureResources[Index].Name != NAME_None;
	}

	// Entry for texture resources. Only supports explicit construction.
	struct FTextureEntry
	{
		FTextureEntry() = default;
		FTextureEntry(const FName& InName, UTexture2D* InTexture, bool bAddToRoot);
		~FTextureEntry();

		// Copying is not supported.
		FTextureEntry(const FTextureEntry&) = delete;
		FTextureEntry& operator=(const FTextureEntry&) = delete;

		// We rely on TArray and don't implement custom move constructor...
		FTextureEntry(FTextureEntry&&) = delete;
		// ... but we need move assignment to support reusing entries.
		FTextureEntry& operator=(FTextureEntry&& Other);

		FName Name = NAME_None;
		TWeakObjectPtr<UTexture2D> Texture;
		FSlateBrush Brush;
		FSlateResourceHandle ResourceHandle;

	private:

		void Reset(bool bReleaseResources);
	};

	TArray<FTextureEntry> TextureResources;
	FTextureEntry ErrorTexture;

	static constexpr EName NAME_ErrorTexture = NAME_None;
	static constexpr TextureIndex INDEX_ErrorTexture = INDEX_NONE;
};

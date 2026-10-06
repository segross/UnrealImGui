// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

#include <Styling/SlateBrush.h>
#include <Textures/SlateShaderResource.h>
#include <UObject/WeakObjectPtr.h>
#include <imgui.h>


class UTexture2D;
struct ImTextureData;
struct ImDrawData;

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
	FTextureManager(FTextureManager&&) = delete;
	FTextureManager& operator=(FTextureManager&&) = delete;

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
		return TextureResources.IndexOfByPredicate([&](const auto& Entry) { return Entry.GetName() == Name; });
	}

	// Get the name of a texture at given index. Returns NAME_None, if index is out of range.
	// @param Index - Index of a texture
	// @returns The name of a texture at given index or NAME_None if index is out of range.
	FName GetTextureName(TextureIndex Index) const
	{
		return IsInRange(Index) ? TextureResources[Index].GetName() : NAME_None;
	}

	// Resolve an index to an opaque ID containing the registration's generation.
	ImTextureID GetTextureId(TextureIndex Index) const;
	// Registration can outlive its externally owned UObject, so release uses this identity-only check.
	bool IsRegisteredTextureId(ImTextureID Id) const;
	bool IsValidTextureId(ImTextureID Id) const;
	// Invalid, released or collected IDs render with the error texture.
	const FSlateResourceHandle& GetTextureHandle(ImTextureID Id) const
	{
		return IsValidTextureId(Id) ? TextureResources[static_cast<uint32>(Id) - 1].GetResourceHandle() : ErrorTexture.GetResourceHandle();
	}

	// Create a texture from raw data.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param SrcBpp - The size in bytes of one pixel
	// @param SrcData - The source data
	// @param SrcDataCleanup - Optional function called to release source data after texture is created (only needed, if data need to be released)
	// @returns The index of a texture that was created
	TextureIndex CreateTexture(const FName& Name, int32 Width, int32 Height, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8*)> SrcDataCleanup = [](uint8*) {});

	// Create a plain texture.
	// @param Name - The texture name
	// @param Width - The texture width
	// @param Height - The texture height
	// @param Color - The texture color
	// @returns The index of a texture that was created
	TextureIndex CreatePlainTexture(const FName& Name, int32 Width, int32 Height, FColor Color);

	// Create Slate resources to an existing texture, managed externally.
	// @param Name - The texture name
	// @param Texture - The texture
	// @returns The index to created/updated texture resources
	TextureIndex CreateTextureResources(const FName& Name, UTexture2D* Texture);

	// Release resources for given texture. Ignores invalid indices.
	// @param Index - The index of a texture resources
	void ReleaseTextureResources(TextureIndex Index);

	// Honor Dear ImGui's dynamic font atlas requests before staging draw commands.
	void UpdateImGuiTextures(ImDrawData& DrawData);
	void ReleaseImGuiTexture(ImTextureData& TextureData);

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
	TextureIndex AddTextureEntry(const FName& Name, UTexture2D* Texture, bool bAddToRoot);
	void UpdateImGuiTexture(ImTextureData& TextureData);

	// Check whether index is in range allocated for TextureResources (it doesn't mean that resources are valid).
	FORCEINLINE bool IsInRange(TextureIndex Index) const
	{
		return static_cast<uint32>(Index) < static_cast<uint32>(TextureResources.Num());
	}

	// Check whether index is in range and whether texture resources are valid (using NAME_None sentinel).
	FORCEINLINE bool IsValidTexture(TextureIndex Index) const
	{
		return IsInRange(Index) && TextureResources[Index].GetName() != NAME_None && TextureResources[Index].GetTexture() != nullptr;
	}

	// Entry for texture resources. Only supports explicit construction.
	struct FTextureEntry
	{
		FTextureEntry() = default;
		FTextureEntry(const FName& InName, UTexture2D* InTexture, bool bAddToRoot, uint32 InGeneration = 0);
		~FTextureEntry();

		// Copying is not supported.
		FTextureEntry(const FTextureEntry&) = delete;
		FTextureEntry& operator=(const FTextureEntry&) = delete;

		// We rely on TArray and don't implement custom move constructor...
		FTextureEntry(FTextureEntry&&) = delete;
		// ... but we need move assignment to support reusing entries.
		FTextureEntry& operator=(FTextureEntry&& Other);

		const FName& GetName() const { return Name; }
		const FSlateResourceHandle& GetResourceHandle() const;
		UTexture2D* GetTexture() const { return Texture.Get(); }
		uint32 GetGeneration() const { return Generation; }

	private:

		void Reset(bool bReleaseResources);

		FName Name = NAME_None;
		uint32 Generation = 0;
		bool bOwnsTexture = false;
		mutable FSlateResourceHandle CachedResourceHandle;
		TWeakObjectPtr<UTexture2D> Texture;
		FSlateBrush Brush;
	};

	TArray<FTextureEntry> TextureResources;
	FTextureEntry ErrorTexture;

	static constexpr EName NAME_ErrorTexture = NAME_None;
	static constexpr TextureIndex INDEX_ErrorTexture = INDEX_NONE;
};

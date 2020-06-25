// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiTextureHandle.h"

#include "ImGuiInteroperability.h"


FImGuiTextureHandle::FImGuiTextureHandle()
	: FImGuiTextureHandle(NAME_None, ImGuiInterops::ToImTextureID(INDEX_NONE))
{
}

FImGuiTextureHandle::FImGuiTextureHandle(const FName& InName, ImTextureID InTextureId)
	: Name(InName)
	, TextureId(InTextureId)
{
	const TextureIndex Index = ImGuiInterops::ToTextureIndex(TextureId);
	checkf((Index == INDEX_NONE) == (Name == NAME_None),
		TEXT("Mismatch between Name and TextureId parameters, with only one indicating a null handle.")
		TEXT(" Name = '%s', TextureIndex(TextureId) = %d"), *Name.ToString(), Index);
}

// FImGuiTextureHandle::HasValidEntry() is implemented in ImGuiModule.cpp to get access to FImGuiModuleManager instance
// without referencing in this class.

// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

class FString;
struct FImGuiKeyInfo;

namespace DebugExecBindings
{
	void UpdatePlayerInputs(const FImGuiKeyInfo& KeyInfo, const FString& Command);
}

#pragma once
#include "ImGuiDelegates.h"

struct FImguiViewHandle
{
public:
	FImguiViewHandle()
	{
		ContextName = NAME_None;
	}
	FImguiViewHandle(const FName& Name){ ContextName = Name;}
	FName GetContextName() const {  return ContextName; }
	bool IsValid() const { return ContextName != NAME_None;}

	FImGuiDelegateHandle AddEditorImGuiDelegate(const FImGuiDelegate& Delegate) const;
	static void RemoveImGuiDelegate(const FImGuiDelegateHandle& Handle);
protected:
	FName ContextName;
};

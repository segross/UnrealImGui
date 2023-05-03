#include "ImguiContextHandle.h"

#include "ImGuiDelegatesContainer.h"
#include "ImGuiModule.h"


FImGuiDelegateHandle FImguiViewHandle::AddEditorImGuiDelegate(const FImGuiDelegate& Delegate) const
{
	// return { FImGuiDelegatesContainer::Get().OnWorldDebug(ContextName).Add(Delegate),
	// 	EDelegateCategory::Default, ContextName};
	return FImGuiModule::Get().AddImguiDelegate(*this, Delegate);
}

void FImguiViewHandle::RemoveImGuiDelegate(const FImGuiDelegateHandle& Handle)
{
	FImGuiModule::Get().RemoveImGuiDelegate(Handle);
}

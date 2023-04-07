#pragma once
#include "SImGuiLayout.h"

class SCommonGuiLayout : public SImGuiLayout
{
public:
	SLATE_BEGIN_ARGS(SCommonGuiLayout)
	{}
	SLATE_ARGUMENT(FImGuiModuleManager*, ModuleManager)
	SLATE_ARGUMENT(TSharedPtr<SCompoundWidget>, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedPtr<SCompoundWidget> Content;
};

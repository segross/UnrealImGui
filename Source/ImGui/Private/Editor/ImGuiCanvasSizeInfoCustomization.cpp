// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiCanvasSizeInfoCustomization.h"

#if WITH_EDITOR

#include "ImGuiModuleSettings.h"

#include <PropertyCustomizationHelpers.h>


#define LOCTEXT_NAMESPACE "ImGuiEditor"


namespace
{
	EImGuiCanvasSizeType GetCanvasSizeTypeEnumValue(const TSharedPtr<IPropertyHandle>& TypeHandle)
	{
		uint8 ValueAsByte = 0;
		if (TypeHandle.IsValid())
		{
			TypeHandle->GetValue(ValueAsByte);
		}
		return static_cast<EImGuiCanvasSizeType>(ValueAsByte);
	}

	bool IsAny(const TSharedPtr<IPropertyHandle>& TypeHandle, EImGuiCanvasSizeType Value)
	{
		return GetCanvasSizeTypeEnumValue(TypeHandle) == Value;
	}

	template<typename... TRest>
	bool IsAny(const TSharedPtr<IPropertyHandle>& TypeHandle, EImGuiCanvasSizeType First, TRest... Rest)
	{
		return IsAny(TypeHandle, First) || IsAny(TypeHandle, Rest...);
	}

	float ShowToHeight(bool bShow)
	{
		return bShow ? 0.f /* Infinity */ : SMALL_NUMBER;
	}

	EVisibility ShowToVisibility(bool bShow)
	{
		return bShow ? EVisibility::SelfHitTestInvisible : EVisibility::Hidden;
	}
}

//----------------------------------------------------------------------------------------------------
// FImGuiKeyInfoCustomization implementation
//----------------------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization> FImGuiCanvasSizeInfoCustomization::MakeInstance()
{
	return MakeShareable(new FImGuiCanvasSizeInfoCustomization);
}

void FImGuiCanvasSizeInfoCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	TSharedPtr<IPropertyHandle> TypeHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FImGuiCanvasSizeInfo, SizeType));
	TSharedPtr<IPropertyHandle> WidthHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FImGuiCanvasSizeInfo, Width));
	TSharedPtr<IPropertyHandle> HeightHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FImGuiCanvasSizeInfo, Height));
	TSharedPtr<IPropertyHandle> ExtendToViewportHandle = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FImGuiCanvasSizeInfo, bExtendToViewport));

#define ADD_DIMENSION_SLOT(Handle, LPadding) \
	+ SHorizontalBox::Slot()\
	.Padding(LPadding, 0.f, 0.f, 0.f)\
	.HAlign(HAlign_Fill)\
	.MaxWidth(80.f)\
	[\
		SNew(SVerticalBox)\
		+ SVerticalBox::Slot()\
		.AutoHeight()\
		[\
			Handle->CreatePropertyNameWidget()\
		]\
		+ SVerticalBox::Slot()\
		.HAlign(HAlign_Fill)\
		.AutoHeight()\
		[\
			Handle->CreatePropertyValueWidget()\
		]\
	]

	auto SizeRowHeight = TAttribute<float>::Create([TypeHandle]()
	{
		return ShowToHeight(IsAny(TypeHandle, EImGuiCanvasSizeType::Custom));
	});

	auto SizeRowVisibility = TAttribute<EVisibility>::Create([TypeHandle]()
	{
		return ShowToVisibility(IsAny(TypeHandle, EImGuiCanvasSizeType::Custom));
	});

	auto ExtendRowHeight = TAttribute<float>::Create([TypeHandle]()
	{
		return ShowToHeight(IsAny(TypeHandle, EImGuiCanvasSizeType::Custom, EImGuiCanvasSizeType::Desktop));
	});

	auto ExtendRowVisibility = TAttribute<EVisibility>::Create([TypeHandle]()
	{
		return ShowToVisibility(IsAny(TypeHandle, EImGuiCanvasSizeType::Custom, EImGuiCanvasSizeType::Desktop));
	});

	HeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(168.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			TypeHandle->CreatePropertyValueWidget()
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(SizeRowHeight)
		[
			SNew(SHorizontalBox)
			.Visibility(SizeRowVisibility)
			ADD_DIMENSION_SLOT(WidthHandle, 0.f)
			ADD_DIMENSION_SLOT(HeightHandle, 6.f)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(ExtendRowHeight)
		[
			SNew(SHorizontalBox)
			.Visibility(ExtendRowVisibility)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ExtendToViewportHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.Padding(4.f, 0.f, 0.f, 0.f)
			.AutoWidth()
			[
				ExtendToViewportHandle->CreatePropertyNameWidget()
			]
		]
	];

#undef ADD_DIMENSION_SLOT
}

void FImGuiCanvasSizeInfoCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}


#undef LOCTEXT_NAMESPACE

#endif // WITH_EDITOR

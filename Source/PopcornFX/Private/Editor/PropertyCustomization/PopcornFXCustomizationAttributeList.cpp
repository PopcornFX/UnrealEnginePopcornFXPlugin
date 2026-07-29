//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXCustomizationAttributeList.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "PopcornFXAttributeSamplerText.h"
#include "Editor/EditorHelpers.h"
#include "Editor/PopcornFXStyle.h"

#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "IPropertyUtilities.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"
#include "ScopedTransaction.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SVectorInputBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Styling/StyleColors.h"
#include "PopcornFXEditor/Private/Slate/SMultiSelectComboBox.h"

#include "Math/Vector.h"
#include "GraphEditorSettings.h"
#include "Engine/Engine.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "PopcornFXCustomizationAttributeList"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXCustomizationAttributeList, Log, All);

//#define ATTRDEBUB_LOG		UE_LOG
#define ATTRDEBUB_LOG(...)	do { } while(0)

//----------------------------------------------------------------------------

const char			*ResolveAttribSamplerNodeName(const PopcornFX::CParticleAttributeSamplerDeclaration *sampler, EPopcornFXAttributeSamplerType::Type samplerType)
{
	if (!PK_VERIFY(sampler != null))
		return null;

	// Same as for ResolveAttribSamplerType, no need to discard everything that doesn't have a default descriptor
	switch (samplerType)
	{
		case	EPopcornFXAttributeSamplerType::Shape:
			return "AttributeSampler_Shape";
		case	EPopcornFXAttributeSamplerType::Curve:
			return "AttributeSampler_Curve";
		case	EPopcornFXAttributeSamplerType::Image:
			return "AttributeSampler_Image";
		case	EPopcornFXAttributeSamplerType::Grid:
			return "AttributeSampler_Grid";
		case	EPopcornFXAttributeSamplerType::AnimTrack:
			return "Attributesampler_AnimTrack";
		case	EPopcornFXAttributeSamplerType::Text:
			return "AttributeSampler_Text";
		case	EPopcornFXAttributeSamplerType::VectorField:
			return "AttributeSampler_VectorField";
		default:
			PK_ASSERT_NOT_REACHED();
			return null;
	}
}

//----------------------------------------------------------------------------

TSharedPtr<IPropertyHandle>		ResolveSamplerProperties(const TSharedPtr<IPropertyHandle> samplerDescPty, EPopcornFXAttributeSamplerType::Type samplerType, const FString &samplerTypeName)
{
	TSharedPtr<IPropertyHandle>	samplerPropertiesPty;
	switch (samplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_AnimTrackProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Image:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_ImageProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Grid:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_GridProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Curve:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_CurveProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Shape:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_ShapeProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Text:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_TextProperties));
		break;
	case EPopcornFXAttributeSamplerType::Type::VectorField:
		samplerPropertiesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_VectorFieldProperties));
		break;
	default:
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Sampler type '%s' not implemented"), *samplerTypeName);
		break;
	}
	return samplerPropertiesPty;
}

//----------------------------------------------------------------------------

FString				GenerateTypeName(PopcornFX::EBaseTypeID typeId)
{
	const PopcornFX::CBaseTypeTraits &traits = PopcornFX::CBaseTypeTraits::Traits(typeId);
	FString			name;

	if (traits.ScalarType == PopcornFX::BaseType_Bool)
		name = TEXT("B");
	else if (traits.IsFp)
		name = TEXT("F");
	else
		name = TEXT("I");
	int32		dim = traits.VectorDimension;
	name.AppendInt(dim);
	return name;
}

//----------------------------------------------------------------------------

// Copy-Pasted form Editor/PropertyEditor/Private/SConstrainedBox.h
//
class SMyConstrainedBox : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMyConstrainedBox)
		: _MinWidth(0)
		, _MaxWidth(0)
	{}
	SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(float, MinWidth)
		SLATE_ATTRIBUTE(float, MaxWidth)
		SLATE_END_ARGS()

	void Construct(const FArguments &InArgs)
	{
		MinWidth = InArgs._MinWidth;
		MaxWidth = InArgs._MaxWidth;

		ChildSlot
			[
				InArgs._Content.Widget
			];
	}

	virtual FVector2D	ComputeDesiredSize(float LayoutScaleMultiplier) const override
	{
		const float MinWidthVal = MinWidth.Get();
		const float MaxWidthVal = MaxWidth.Get();

		if (MinWidthVal == 0.0f && MaxWidthVal == 0.0f)
			return SCompoundWidget::ComputeDesiredSize(LayoutScaleMultiplier);
		else
		{
			FVector2D ChildSize = ChildSlot.GetWidget()->GetDesiredSize();

			float XVal = FMath::Max(MinWidthVal, ChildSize.X);
			if (MaxWidthVal >= MinWidthVal)
			{
				XVal = FMath::Min(MaxWidthVal, XVal);
			}

			return FVector2D(XVal, ChildSize.Y);
		}
	}
private:
	TAttribute<float> MinWidth;
	TAttribute<float> MaxWidth;
};

const FText				s_AxisTexts[3][4] = {
	// 0 : !isColor
		{
			LOCTEXT("X_Label", "X"),
			LOCTEXT("Y_Label", "Y"),
			LOCTEXT("Z_Label", "Z"),
			LOCTEXT("W_Label", "W"),
		},
		// 1: isColor
		{
			LOCTEXT("R_Label", "R"),
			LOCTEXT("G_Label", "G"),
			LOCTEXT("B_Label", "B"),
			LOCTEXT("A_Label", "A"),
		},
		// 2: isQuaternion
		{
			LOCTEXT("Roll_Label", "Roll"),
			LOCTEXT("Pitch_Label", "Pitch"),
			LOCTEXT("Yaw_Label", "Yaw"),
			LOCTEXT("", ""),
		},
};

const FLinearColor		s_AxisColors[] = {
	AxisDisplayInfo::GetAxisColor(EAxisList::X),
	AxisDisplayInfo::GetAxisColor(EAxisList::Y),
	AxisDisplayInfo::GetAxisColor(EAxisList::Z),
	FLinearColor::Black,
};

#define CREATE_ATTRIBUTE_TRANSACTION_PROPERTY_CHAIN(__PropertyHandle) \
	FPropertyChangedEvent changedProperty(__PropertyHandle->GetProperty()); \
	TSharedRef<FEditPropertyChain> propertyChain(MakeShareable(new FEditPropertyChain)); \
	propertyChain->AddHead(__PropertyHandle->GetProperty()); \
	propertyChain->SetActivePropertyNode(__PropertyHandle->GetProperty()); \
	propertyChain->SetActiveMemberPropertyNode(__PropertyHandle->GetProperty()); \

#define PRE_ATTRIBUTE_TRANSACTION(__Owner, __PropertyHandle) \
	__Owner->Modify(); \
	CREATE_ATTRIBUTE_TRANSACTION_PROPERTY_CHAIN(__PropertyHandle); \
	{ \
	UPopcornFXEmitterComponent	*emitter = Cast<UPopcornFXEmitterComponent>(__Owner); \
	if (emitter) \
		emitter->SetIsTransacting(true); \
	} \
	__Owner->PreEditChange(*propertyChain); \

#define POST_ATTRIBUTE_TRANSACTION(__Owner) \
	FPropertyChangedChainEvent ChainEvent(*propertyChain, changedProperty); \
	__Owner->PostEditChangeChainProperty(ChainEvent); \
	{ \
	UPopcornFXEmitterComponent	*emitter = Cast<UPopcornFXEmitterComponent>(__Owner); \
	if (emitter) \
		emitter->SetIsTransacting(false); \
	} \

// A widget representing an attribute
struct SAttributeWidget : SCompoundWidget
{
	typedef SAttributeWidget								TSelf;
	typedef FPopcornFXCustomizationAttributeList			TParent;

	FPopcornFXCustomizationAttributeList::FAttributeDesc	m_SlateDesc;
	// If true, we're creating a single value of a multiple dimension attribute
	// otherwise it's the header row
	bool													m_IsExpanded = false;
	uint32													m_Dimi = 0;
	
	// The property handle is used to propagate a PropertyChangedEvent to the Owner to handle transactions properly
	TSharedPtr<IPropertyHandle>								m_PropertyHandle;
	UObject													*m_Owner = nullptr;

	FEditableTextBoxStyle									m_EditableTextBoxStyle;

	/** True if the slider is being used to change the value of the property */
	bool													m_bIsUsingSlider;
	/** When using the slider, what was the last committed value */
	PopcornFX::SAttributesContainer_SAttrib					m_LastSliderCommittedValue;

	SLATE_BEGIN_ARGS(SAttributeWidget) { }
		SLATE_ARGUMENT(FPopcornFXCustomizationAttributeList::FAttributeDesc, SlateDesc)
		SLATE_ARGUMENT(TWeakPtr<FPopcornFXCustomizationAttributeList>, Parent)
		SLATE_ARGUMENT(TOptional<bool>, Expanded)
		SLATE_ARGUMENT(TOptional<uint32>, Dimi)
		SLATE_ARGUMENT(TSharedPtr<IPropertyHandle>, PropertyHandle)
	SLATE_END_ARGS()

	void			Construct(const FArguments &InArgs)
	{
		m_SlateDesc = InArgs._SlateDesc;
		m_IsExpanded = InArgs._Expanded.Get(false);
		m_Dimi = InArgs._Dimi.Get(0);
		m_PropertyHandle = InArgs._PropertyHandle;
		// Sets the style like the rest of the editor (default one is different)
		m_EditableTextBoxStyle = FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");

		TArray<UObject *>	beingCustomized;
		m_PropertyHandle->GetOuterObjects(beingCustomized);
		m_Owner = beingCustomized[0];

		// Creating row for a single dimension of the attribute
		TSharedPtr<SHorizontalBox>	hbox;
		SAssignNew(hbox, SHorizontalBox);
		if (m_IsExpanded)
		{
			hbox->AddSlot()
				.VAlign(VAlign_Center)
				.FillWidth(1.0f)
				.Padding(0.0f, 1.0f, 0.0f, 1.0f)
				[
					SNew(SMyConstrainedBox)
						.MinWidth(125.f)
						.MaxWidth(125.f)
						[
							MakeAxis(m_Dimi)
						]
				];
		}
		else // Header row of the attribute
		{
			if (m_SlateDesc.m_DropDownMode == EPopcornFXAttributeDropDownMode::AttributeDropDownMode_SingleSelect)
			{
				hbox->AddSlot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
					.Padding(0.0f, 1.0f, 2.0f, 1.0f)
					[
						SNew(SMyConstrainedBox)
							.MinWidth(125.f)
							[
								MakeSingleSelectEnum()
							]
					];
			}
			else if (m_SlateDesc.m_DropDownMode == EPopcornFXAttributeDropDownMode::AttributeDropDownMode_MultiSelect)
			{
				hbox->AddSlot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
					.Padding(0.0f, 1.0f, 2.0f, 1.0f)
					[
						SNew(SMyConstrainedBox)
							.MinWidth(125.f)
							[
								MakeMultiSelectEnum()
							]
					];
			}
			else if (!m_SlateDesc.m_IsColor)
			{
				for (u32 dimi = 0; dimi < m_SlateDesc.m_VectorDimension; ++dimi)
				{
					hbox->AddSlot()
						.VAlign(VAlign_Center)
						.FillWidth(1.0f)
						.Padding(0.0f, 1.0f, 0.0f, 1.0f)
						[
							SNew(SMyConstrainedBox)
								.MinWidth(125.f)
								[
									MakeAxis(dimi)
								]
						];
				}
			}
			else
			{
				hbox->AddSlot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.FillWidth(1)
							[
								SNew(SColorBlock)
									.Color(this, &SAttributeWidget::OnGetColor)
									.ShowBackgroundForAlpha(true)
									.AlphaDisplayMode(m_SlateDesc.m_VectorDimension == 3 ? EColorBlockAlphaDisplayMode::Ignore : EColorBlockAlphaDisplayMode::Combined)
									.OnMouseButtonDown(this, &SAttributeWidget::OnColorPickerClicked)
									.Size(FVector2D(120.0f, 20.0f))
									.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
							]
						+ SHorizontalBox::Slot()
							.FillWidth(1)
							[
								SNew(SColorBlock)
									.Color(this, &SAttributeWidget::OnGetColor)
									.ShowBackgroundForAlpha(false)
									.AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
									.Visibility(this, &SAttributeWidget::GetVisibilityForOpaqueDisplay)
									.OnMouseButtonDown(this, &SAttributeWidget::OnColorPickerClicked)
									.Size(FVector2D(120.0f, 20.0f))
									.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
							]
					];
			}
		}
		ChildSlot
		[
			hbox.ToSharedRef()
		];
	}

	EVisibility GetVisibilityForOpaqueDisplay() const
	{
		EVisibility OpaqueDisplayVisibility = EVisibility::Collapsed;
		if (m_SlateDesc.m_VectorDimension == 4)
		{
			const bool bColorIsAlreadyOpaque = (OnGetColor().A == 1.0);
			if (bColorIsAlreadyOpaque)
			{
				OpaqueDisplayVisibility = EVisibility::Collapsed;
			}
			else
			{
				OpaqueDisplayVisibility = EVisibility::Visible;
			}
		}

		return OpaqueDisplayVisibility;
	}

	FLinearColor	OnGetColor() const
	{
		FPopcornFXAttributeList *attrList = nullptr;
		if (!_GetAttrib(attrList))
			return FLinearColor::Black;

		PopcornFX::SAttributesContainer_SAttrib	attribValue;
		attrList->GetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue *>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

		const float	r = attribValue.m_Data32f[0];
		const float	g = attribValue.m_Data32f[1];
		const float	b = attribValue.m_Data32f[2];
		const float	a = attribValue.m_Data32f[3];

		return FLinearColor(r, g, b, a);
	}

	FReply			OnColorPickerClicked(const FGeometry &MyGeometry, const FPointerEvent &MouseEvent)
	{
		FLinearColor		initialColor = OnGetColor();
		FColorPickerArgs	pickerArgs;
		{
			pickerArgs.bUseAlpha = m_SlateDesc.m_Traits->VectorDimension == 4;
			pickerArgs.bOnlyRefreshOnMouseUp = false;
			pickerArgs.bOnlyRefreshOnOk = false;
			pickerArgs.sRGBOverride = false;//sRGBOverride;
			pickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
			pickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SAttributeWidget::OnSetColorFromColorPicker);
			pickerArgs.InitialColor = initialColor;
		}

		OpenColorPicker(pickerArgs);
		return FReply::Handled();
	}

	void			OnSetColorFromColorPicker(FLinearColor newColor)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList = nullptr;
		if (!_GetAttrib(attrList))
			return;
		UPopcornFXEffect		*effect = attrList->Effect();
		if (effect == null)
			return;
		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, m_SlateDesc.m_Index));
		if (decl == null)
			return;

		const FScopedTransaction	Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit as Color"));

		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
		{
			PopcornFX::SAttributesContainer_SAttrib	attribValue;
			attribValue.m_Data32f[0] = newColor.R;
			attribValue.m_Data32f[1] = newColor.G;
			attribValue.m_Data32f[2] = newColor.B;
			if (m_SlateDesc.m_Traits->VectorDimension > 3)
				attribValue.m_Data32f[3] = newColor.A;
			decl->ClampToRangeIFN(attribValue);
			attrList->SetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue), true); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
		}
		POST_ATTRIBUTE_TRANSACTION(m_Owner);

	}

	TSharedRef<SWidget>		MakeAxis(uint32 dimi)
	{
		check(dimi < m_SlateDesc.m_Traits->VectorDimension);
		if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
			return _MakeBoolAxis(dimi);
		if (m_SlateDesc.m_Traits->IsFp)
			return _MakeAxis<float>(dimi);
		return _MakeAxis<int32>(dimi);
	}

	TSharedRef<SWidget>		MakeSingleSelectEnum()
	{
		TSharedRef<TSelf>							sharedThis = SharedThis(this);
		TSharedPtr<SComboBox<TSharedPtr<int32>>>	comboBox;

		// Custom SEnumComboBox. Attribute enums aren't reflected enums
		if (m_SlateDesc.m_ReadOnly)
		{
			SAssignNew(comboBox, SComboBox<TSharedPtr<int32>>)
				.OptionsSource(&m_SlateDesc.m_EnumListIndices)
				.OnGenerateWidget_Lambda([sharedThis](TSharedPtr<int32> Item)
					{
						return SNew(STextBlock)
							.Text(Item.IsValid() ? FText::FromString(sharedThis->m_SlateDesc.m_EnumList[*Item]) : FText::GetEmpty())
							.Font(sharedThis->m_SlateDesc.m_Font);
					})
				.Content()
				[
					SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
						.Font(sharedThis->m_SlateDesc.m_Font)
				];
		}
		else
		{
			SAssignNew(comboBox, SComboBox<TSharedPtr<int32>>)
				.OptionsSource(&m_SlateDesc.m_EnumListIndices)
				.OnGenerateWidget_Lambda([sharedThis](TSharedPtr<int32> Item)
					{
						return SNew(STextBlock)
							.Text(Item.IsValid() ? FText::FromString(sharedThis->m_SlateDesc.m_EnumList[*Item]) : FText::GetEmpty())
							.Font(sharedThis->m_SlateDesc.m_Font);
					})
				.OnSelectionChanged(sharedThis, &TSelf::OnValueChangedEnum)
				.Content()
				[
					SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
						.Font(sharedThis->m_SlateDesc.m_Font)
				];
		}
		return comboBox.ToSharedRef();
	}

	void					OnCheckStateChanged(ECheckBoxState State, TSharedPtr<FString> Item)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		int32 value = attrList->GetAttributeDim<int32>(m_SlateDesc.m_Index, 0);
		for (int32 i = 0; i < m_SlateDesc.m_EnumList.Num(); i++)
		{
			if (m_SlateDesc.m_EnumList[i] == *Item)
			{
				if (State == ECheckBoxState::Checked)
				{
					value |= (1 << i);
				}
				else if (State == ECheckBoxState::Unchecked)
				{
					value &= ~(1 << i);
				}
				break;
			}
		}
		TArray<UObject *> outers;
		m_PropertyHandle->GetOuterObjects(outers);
		if (!m_PropertyHandle.IsValid() || m_PropertyHandle->GetProperty() == nullptr)
			return;
		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
		attrList->SetAttributeDim<int32>(m_SlateDesc.m_Index, 0, value, true);
		POST_ATTRIBUTE_TRANSACTION(m_Owner);
	}

	ECheckBoxState			IsMultiSelectEnumChecked(TSharedPtr<FString> Item)
	{
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return ECheckBoxState::Unchecked;
		int32 value = attrList->GetAttributeDim<int32>(m_SlateDesc.m_Index, 0);
		for (int32 i = 0; i < m_SlateDesc.m_EnumList.Num(); i++)
		{
			if (m_SlateDesc.m_EnumList[i] == *Item
				&& value & (1 << i))
			{
				return ECheckBoxState::Checked;
			}
		}
		return ECheckBoxState::Unchecked;
	}

	TSharedRef<SWidget>		MultiSelectHorizontalBox(TSharedPtr<FString> Item)
	{
		return
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock)
					.Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty())
					.Font(m_SlateDesc.m_Font)
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.0f, 0.0f)
			[
				SNew(SCheckBox)
					.OnCheckStateChanged(this, &TSelf::OnCheckStateChanged, Item)
					.IsChecked_Lambda([this, Item]()
						{
							FPopcornFXAttributeList *attrList;
							if (!_GetAttrib(attrList))
								return ECheckBoxState::Unchecked;
							int32 value = attrList->GetAttributeDim<int32>(m_SlateDesc.m_Index, 0);
							for (int32 i = 0; i < m_SlateDesc.m_EnumList.Num(); i++)
							{
								if (m_SlateDesc.m_EnumList[i] == *Item
									&& value & (1 << i))
								{
									return ECheckBoxState::Checked;
								}
							}
							return ECheckBoxState::Unchecked;
						})
			];
	}

	TSharedRef<SWidget>		MakeMultiSelectEnum()
	{
		TSharedRef<TSelf>	sharedThis = SharedThis(this);

		return SNew(SMultiSelectComboBox<TSharedPtr<FString>>)
			.OptionsSource(&m_SlateDesc.m_SharedEnumList)
			.OnGenerateWidget(this, &TSelf::MultiSelectHorizontalBox)
			.Content()
			[
				SNew(STextBlock)
					.Text_Lambda([sharedThis]()
						{
							FPopcornFXAttributeList *attrList;
							if (!sharedThis->_GetAttrib(attrList))
								return FText::FromString("Error: Could not retrieve attribute value");
							FString finalText;
							int32 value = attrList->GetAttributeDim<int32>(sharedThis->m_SlateDesc.m_Index, 0);
							for (int32 i = 0; i < sharedThis->m_SlateDesc.m_EnumList.Num(); i++)
							{
								if (value & (1 << i))
								{
									if (!finalText.IsEmpty())
										finalText += " | ";
									finalText += sharedThis->m_SlateDesc.m_EnumList[i];
								}
							}
							if (finalText.IsEmpty())
								finalText = "None";
							return FText::FromString(finalText);
						})
					.Font(sharedThis->m_SlateDesc.m_Font)
			];
	}

	TSharedRef<SWidget>		_MakeBoolAxis(uint32 dimi)
	{
		// Access a shared reference to 'this'
		TSharedRef<TSelf> sharedThis = SharedThis(this);
		
		check(dimi < m_SlateDesc.m_Traits->VectorDimension);
		if (!m_SlateDesc.m_IsOneShotTrigger)
		{
			TSharedPtr<SCheckBox>	axis;
			if (m_SlateDesc.m_ReadOnly)
			{
				SAssignNew(axis, SCheckBox)
					.IsChecked(sharedThis, &TSelf::GetValueBool, dimi);
			}
			else
			{
				SAssignNew(axis, SCheckBox)
					.OnCheckStateChanged(sharedThis, &TSelf::OnValueChangedBool, dimi)
					.IsChecked(sharedThis, &TSelf::GetValueBool, dimi);
			}
			return axis.ToSharedRef();
		}
		else
		{
			TSharedPtr<SButton>	axis;
			SAssignNew(axis, SButton)
				.Text(FText::FromString("Pulse"))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ClickMethod(EButtonClickMethod::MouseDown)
				.OnClicked(this, &TSelf::OnValuePulsedBool, dimi)
				.ContentPadding(0.0f)
				.ForegroundColor(FSlateColor::UseForeground())
				.IsFocusable(false);
			return axis.ToSharedRef();
		}
	}

	FLinearColor	GetLabelColor(const FPopcornFXCustomizationAttributeList::FAttributeDesc &desc, uint32 dimi)
	{
		if (m_SlateDesc.m_VectorDimension == 1)
		{
			const UGraphEditorSettings *Settings = GetDefault<UGraphEditorSettings>();
			if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_I32)
				return Settings->IntPinTypeColor;
			else if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Float)
				return Settings->FloatPinTypeColor;
			else
				return Settings->FloatPinTypeColor;
		}
		else
		{
			return s_AxisColors[dimi];
		}
	}

	template <typename NumericType>
	TSharedRef<SWidget>		_MakeAxis(uint32 dimi)
	{
		// Access a shared reference to 'this'
		TSharedRef<TSelf> sharedThis = SharedThis(this);

		check(dimi < m_SlateDesc.m_Traits->VectorDimension);

		const bool		isColor = m_SlateDesc.m_IsColor;
		const bool		isQuaternion = m_SlateDesc.m_IsQuaternion;

		FTextBlockStyle			style;
		style.SetColorAndOpacity(FSlateColor::UseForeground());
		style.SetFont(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));
		m_EditableTextBoxStyle.SetTextStyle(style);

		TSharedPtr< SNumericEntryBox<NumericType> >	axis;
		if (m_SlateDesc.m_ReadOnly)
		{
			SAssignNew(axis, SNumericEntryBox<NumericType>)
				.AllowSpin(true)

				.MinValue(m_SlateDesc.m_HasMin ? TOptional<NumericType>(m_SlateDesc.m_Min.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MaxValue(m_SlateDesc.m_HasMax ? TOptional<NumericType>(m_SlateDesc.m_Max.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MinSliderValue(m_SlateDesc.m_HasMin ? TOptional<NumericType>(m_SlateDesc.m_Min.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MaxSliderValue(m_SlateDesc.m_HasMax ? TOptional<NumericType>(m_SlateDesc.m_Max.Get<NumericType>()[dimi]) : TOptional<NumericType>())

				.Value(sharedThis, &TSelf::GetValue<NumericType>, dimi)

				.EditableTextBoxStyle(&m_EditableTextBoxStyle)
				.LabelPadding(FMargin(3))
				.Label()
				[
					SNumericEntryBox<NumericType>::BuildNarrowColorLabel(GetLabelColor(m_SlateDesc, dimi))
				];
		}
		else
		{
			SAssignNew(axis, SNumericEntryBox<NumericType>)
				.AllowSpin(true)

				.MinValue(m_SlateDesc.m_HasMin ? TOptional<NumericType>(m_SlateDesc.m_Min.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MaxValue(m_SlateDesc.m_HasMax ? TOptional<NumericType>(m_SlateDesc.m_Max.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MinSliderValue(m_SlateDesc.m_HasMin ? TOptional<NumericType>(m_SlateDesc.m_Min.Get<NumericType>()[dimi]) : TOptional<NumericType>())
				.MaxSliderValue(m_SlateDesc.m_HasMax ? TOptional<NumericType>(m_SlateDesc.m_Max.Get<NumericType>()[dimi]) : TOptional<NumericType>())

				.Value(sharedThis, &TSelf::GetValue<NumericType>, dimi)
				.OnValueChanged(sharedThis, &TSelf::OnValueChanged<NumericType>, dimi)
				.OnValueCommitted(sharedThis, &TSelf::OnValueCommitted<NumericType>, dimi)
				.OnBeginSliderMovement(sharedThis, &TSelf::OnBeginSliderMovement<NumericType>, dimi)
				.OnEndSliderMovement(sharedThis, &TSelf::OnEndSliderMovement<NumericType>, dimi)

				.EditableTextBoxStyle(&m_EditableTextBoxStyle)
				.LabelPadding(FMargin(3))
				.LabelLocation(SNumericEntryBox<NumericType>::ELabelLocation::Inside)
				.Label()
				[
					SNumericEntryBox<NumericType>::BuildNarrowColorLabel(GetLabelColor(m_SlateDesc, dimi))
				];
		}
		return axis.ToSharedRef();
	}

	bool			_GetAttrib(FPopcornFXAttributeList *&outAttribList) const
	{
		outAttribList = m_SlateDesc.m_AttributeList;
		return outAttribList != null;
	}

	template <typename NumericType>
	TOptional<NumericType>		GetValue(uint32 dimi) const
	{
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return TOptional<NumericType>();

		if (!m_SlateDesc.m_IsQuaternion)
			return attrList->GetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi);
		else
			return attrList->GetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi);
	}

	ECheckBoxState	GetValueBool(uint32 dimi) const
	{
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return ECheckBoxState::Undetermined;
		return attrList->GetAttributeDim<bool>(m_SlateDesc.m_Index, dimi) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	void	OnValueChangedBool(ECheckBoxState value, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
		attrList->SetAttributeDim<bool>(m_SlateDesc.m_Index, dimi, value == ECheckBoxState::Checked, true);
		POST_ATTRIBUTE_TRANSACTION(m_Owner);
	}

	FReply	OnValuePulsedBool(uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return FReply::Handled();
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return FReply::Handled();

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Pulse"));

		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
		attrList->PulseBoolAttributeDim(m_SlateDesc.m_Index, dimi, true);
		POST_ATTRIBUTE_TRANSACTION(m_Owner);
		return FReply::Handled();
	}

	FText	GetValueEnumText() const
	{
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return FText();
		return FText::FromString(m_SlateDesc.m_EnumList[attrList->GetAttributeDim<int32>(m_SlateDesc.m_Index, 0)]);
	}

	void	OnValueChangedEnum(TSharedPtr<int32> selectedItem, ESelectInfo::Type selectInfo)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
		attrList->SetAttributeDim<int32>(m_SlateDesc.m_Index, 0, *selectedItem, true);
		POST_ATTRIBUTE_TRANSACTION(m_Owner);
	}

	template <typename NumericType>
	void		OnValueChanged(const NumericType NewValue, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly || !m_bIsUsingSlider)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;
		//UE_LOG(LogPopcornFXCustomizationAttributeList, Log, TEXT("--- DETAIL ATTRLIST change %p %f ---"), attrList, float(NewValue));

		if (!m_SlateDesc.m_IsQuaternion)
			attrList->SetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi, NewValue, true);
		else
			attrList->SetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi, NewValue, true);
	}

	template <typename NumericType>
	void		OnValueCommitted(const NumericType NewValue, ETextCommit::Type CommitInfo, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		NumericType	value;
		if (!m_SlateDesc.m_IsQuaternion)
			value = attrList->GetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi);
		else
			value = attrList->GetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi);
		/* sometimes an FProperty may have been destroyed due to 2 different events invoking this handler (with the same
		 * NewValue) and the first one nullifying the current FProperty ~ in this case it's too late to not invoke the
		 * method, but we can not run the code instead */
		if (m_bIsUsingSlider || value != NewValue)
		{
			const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

			m_LastSliderCommittedValue.Get<NumericType>()[dimi] = NewValue;

			PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
			if (!m_SlateDesc.m_IsQuaternion)
				attrList->SetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi, NewValue, true);
			else
				attrList->SetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi, NewValue, true);
			POST_ATTRIBUTE_TRANSACTION(m_Owner);
			GEngine->EndTransaction();
		}
	}

	/**
	 * Called when the slider begins to move.  We create a transaction here to undo the property
	 */
	template <typename NumericType>
	void OnBeginSliderMovement(uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		m_bIsUsingSlider = true;

		NumericType	value;
		if (!m_SlateDesc.m_IsQuaternion)
			value = attrList->GetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi);
		else
			value = attrList->GetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi);

		m_LastSliderCommittedValue.Get<NumericType>()[dimi] = value;
		GEditor->BeginTransaction(TEXT("PopcornFX Attribute"), LOCTEXT("AttributeCommit", "Attribute Value Commit"), nullptr);
		PRE_ATTRIBUTE_TRANSACTION(m_Owner, m_PropertyHandle);
	}

	/**
	 * Called when the slider stops moving.  We end the previously created transaction
	 */
	template <typename NumericType>
	void OnEndSliderMovement(NumericType NewValue, uint32 dimi)
	{
		{
			UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(m_Owner);
			if (emitter)
				emitter->SetIsTransacting(false);
		}

		if (m_SlateDesc.m_ReadOnly)
			return;
		FPopcornFXAttributeList *attrList;
		if (!_GetAttrib(attrList))
			return;

		m_bIsUsingSlider = false;

		// When the slider end, we may have not called SetValue(NewValue) without the InteractiveChange|NotTransactable flags.
		//That prevents some transaction and callback to be triggered like the NotifyHook.
		if (m_LastSliderCommittedValue.Get<NumericType>()[dimi] != NewValue)
		{
			if (!m_SlateDesc.m_IsQuaternion)
				attrList->SetAttributeDim<NumericType>(m_SlateDesc.m_Index, dimi, NewValue, true);
			else
				attrList->SetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi, NewValue, true);

			CREATE_ATTRIBUTE_TRANSACTION_PROPERTY_CHAIN(m_PropertyHandle);
			POST_ATTRIBUTE_TRANSACTION(m_Owner);
			GEditor->EndTransaction();
		}
	}
};

FPopcornFXCustomizationAttributeList::FPopcornFXCustomizationAttributeList()
:	m_Effect(null)
{
	ATTRDEBUB_LOG(LogPopcornFXCustomizationAttributeList, Log, TEXT("FPopcornFXCustomizationAttributeList ctor %p"), this);
}

FPopcornFXCustomizationAttributeList::~FPopcornFXCustomizationAttributeList()
{
	ATTRDEBUB_LOG(LogPopcornFXCustomizationAttributeList, Log, TEXT("FPopcornFXCustomizationAttributeList dtor %p"), this);
}

FReply		FPopcornFXCustomizationAttributeList::OnResetClicked(FAttributeDesc slateDesc)
{
	if (slateDesc.m_ReadOnly)
		return FReply::Handled();

	FPopcornFXAttributeList *attrList = AttrList();
	if (attrList == null)
		return FReply::Handled();

	const FScopedTransaction Transaction(LOCTEXT("AttributeReset", "Attribute Reset"));
	attrList->SetAttribute(slateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue *>(&slateDesc.m_Def), true); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
	return FReply::Handled();
}

FReply		FPopcornFXCustomizationAttributeList::OnDimResetClicked(uint32 dimi, FAttributeDesc slateDesc)
{
	if (slateDesc.m_ReadOnly)
		return FReply::Handled();

	FPopcornFXAttributeList *attrList = AttrList();
	if (attrList == null)
		return FReply::Handled();

	const FScopedTransaction Transaction(LOCTEXT("AttributeResetDim", "Attribute Reset Dimension"));

	if (slateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
	{
		const bool	defaultValue = reinterpret_cast<const bool *>(slateDesc.m_Def.Get<u32>())[dimi];
		attrList->SetAttributeDim<bool>(slateDesc.m_Index, dimi, defaultValue, true);
	}
	else
	{
		const s32	defaultValue = slateDesc.m_Def.Get<s32>()[dimi];
		attrList->SetAttributeDim<s32>(slateDesc.m_Index, dimi, defaultValue, true);
	}
	return FReply::Handled();
}

EVisibility		FPopcornFXCustomizationAttributeList::GetResetVisibility(FAttributeDesc slateDesc) const
{
	const FPopcornFXAttributeList *attrList = AttrList();
	if (attrList == null)
		return EVisibility::Hidden;

	PopcornFX::SAttributesContainer_SAttrib	attribValue;
	attrList->GetAttribute(slateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue *>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

	if (slateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
	{
		for (uint32 dimi = 0; dimi < slateDesc.m_Traits->VectorDimension; ++dimi)
		{
			if (reinterpret_cast<bool *>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool *>(slateDesc.m_Def.Get<uint32>())[dimi])
				return EVisibility::Visible;
		}
	}
	else
	{
		for (uint32 dimi = 0; dimi < slateDesc.m_Traits->VectorDimension; ++dimi)
		{
			if (attribValue.Get<uint32>()[dimi] != slateDesc.m_Def.Get<uint32>()[dimi])
				return EVisibility::Visible;
		}
	}
	return EVisibility::Hidden;
}

EVisibility		FPopcornFXCustomizationAttributeList::GetDimResetVisibility(uint32 dimi, FAttributeDesc slateDesc) const
{
	const FPopcornFXAttributeList *attrList = AttrList();
	if (attrList == null)
		return EVisibility::Hidden;

	PopcornFX::SAttributesContainer_SAttrib	attribValue;
	attrList->GetAttribute(slateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue *>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day

	if (slateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
	{
		if (reinterpret_cast<bool *>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool *>(slateDesc.m_Def.Get<uint32>())[dimi])
			return EVisibility::Visible;
	}
	else
	{
		if (attribValue.Get<uint32>()[dimi] != slateDesc.m_Def.Get<uint32>()[dimi])
			return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

void	FPopcornFXCustomizationAttributeList::RebuildAndRefresh()
{
	const FPopcornFXAttributeList *attrList = AttrList();
	if (attrList == nullptr)
		return;

	// Categories are only contained in the effect's DefaultAttributeList
	const FPopcornFXAttributeList *defAttrList = null;
	UPopcornFXEffect *resolvedEffect = attrList->Effect();
	if (!resolvedEffect)
	{
		m_IGroups.Empty();
		m_NumAttributes.Empty();
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Could not resolve effect"));
		return;
	}
	if (attrList != null)
	{
		defAttrList = attrList->GetDefaultAttributeList(resolvedEffect);
		if (defAttrList != null)
		{
			m_IGroups.SetNum(defAttrList->GetCategoryCount());
			m_NumAttributes.SetNum(defAttrList->GetCategoryCount());
		}
		else
		{
			UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Could not retrieve categories!"));
		}
	}

	const u32	categoryCount = m_IGroups.Num();

	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		PK_ASSERT(defAttrList != null);
		m_IGroups[iCategory] = &m_ChildBuilder->AddGroup(FName(defAttrList->GetCategoryName(iCategory)), FText::FromString(defAttrList->GetCategoryName(iCategory)));
		// Customize just to remove the splitter so it looks like a "title" 
		m_IGroups[iCategory]->HeaderRow()
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(defAttrList->GetCategoryName(iCategory)))
						.ToolTipText(FText::FromString(defAttrList->GetCategoryName(iCategory)))
						.Font(m_DetailLayoutBuilder->GetDetailFont())

				]
		];
	}

	RebuildAttributes();
	// Groups are empty if something failed, which can happen when changing the effect of blueprints or other serialization stuff
	if (m_IGroups.Num() == 0)
		return;
	RebuildSamplers();
}

FPopcornFXAttributeList *FPopcornFXCustomizationAttributeList::RetrieveAttributeList() const
{
	const TArray<UObject* > &objects = m_BeingCustomized;
	if (objects.Num() != 1)
		return null;
	UObject *outer = objects[0];
	FPopcornFXAttributeList *attrList = null;
	APopcornFXEmitter *emitter = null;
	UPopcornFXEmitterComponent *emitterComponent = null;
	UPopcornFXEffect *effect = null;

	if ((emitter = Cast<APopcornFXEmitter>(outer)) != null)
	{
		outer = emitter->PopcornFXEmitterComponent;
	}
	else
		outer = objects[0];

	// Try re-fetching attribute list from Outer to make sur everything is up to date

	if ((emitterComponent = Cast<UPopcornFXEmitterComponent>(outer)) != null)
	{
		// Don't try to display attributes on an emitter with no effect attached
		if (emitterComponent->Effect == null)
			return null;
		FPopcornFXAttributeList *a = emitterComponent->GetAttributeList();
		if (!PK_VERIFY(a != null))
			return null;
		PK_ASSERT(attrList == null || attrList == a);
		attrList = a;
	}
	else if ((effect = Cast<UPopcornFXEffect>(outer)) != null)
	{
		FPopcornFXAttributeList *a = &effect->DefaultAttributeList;
		if (!PK_VERIFY(a != null))
			return null;
		PK_ASSERT(attrList == null || attrList == a);
		attrList = a;
	}
	return attrList;
}

FPopcornFXAttributeList *FPopcornFXCustomizationAttributeList::AttrList()
{
	return RetrieveAttributeList();
}

const FPopcornFXAttributeList	*FPopcornFXCustomizationAttributeList::AttrList() const
{
	return RetrieveAttributeList();
}

void	FPopcornFXCustomizationAttributeList::RebuildIFN()
{
	if (PK_VERIFY(m_PropertyUtilities.IsValid()))
		m_PropertyUtilities->ForceRefresh();
}

void	FPopcornFXCustomizationAttributeList::UpdateSampler(const FPopcornFXSamplerDesc *desc, FPopcornFXAttributeSampler *sampler)
{
	if (desc->m_UseSamplerAsset)
	{
		sampler->RefreshFromProperties(desc->m_SamplerAsset != nullptr ? desc->m_SamplerAsset->GetProperties() : nullptr);
	}
	else
	{
		sampler->RefreshFromProperties(desc->ResolveAttributeProperties());
	}
	RebuildIFN();
}

void	FPopcornFXCustomizationAttributeList::Rebuild()
{
	const FPopcornFXAttributeList *attrList = AttrList(); // AttrList() will ask for rebuild ifn, we dont want that here
	if (attrList != null)
	{
		m_Effect = attrList->Effect();

		if (m_Effect && !m_Effect->OnEffectReimported.IsBoundToObject(this))
		{
			m_Effect->OnEffectReimported.AddThreadSafeSP(this, &FPopcornFXCustomizationAttributeList::RebuildIFN);
		}
	}
	UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(m_BeingCustomized[0]);
	if (emitter)
	{
		if (!emitter->OnRequestUIRefresh.IsBoundToObject(this))
		{
			emitter->OnRequestUIRefresh.AddThreadSafeSP(this, &FPopcornFXCustomizationAttributeList::RebuildIFN);
		}
	}

	RebuildAndRefresh();
}

void	FPopcornFXCustomizationAttributeList::BuildAttribute(const FPopcornFXAttributeDesc *desc, const FPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory)
{
	UPopcornFXEffect	*effect = attrList->Effect();
	if (effect == null)
		return;
	// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE nativization bugs. To refactor some day
	const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, attri));
	if (decl == null || decl->IsPrivate())
		return;

	const PopcornFX::EBaseTypeID	attributeBaseTypeID = (PopcornFX::EBaseTypeID)desc->AttributeBaseTypeID();

	FAttributeDesc		slateDesc;

	slateDesc.m_AttributeList = AttrList();
	slateDesc.m_Font = m_DetailLayoutBuilder->GetDetailFont();
	slateDesc.m_Index = attri;
	slateDesc.m_Traits = &(PopcornFX::CBaseTypeTraits::Traits(attributeBaseTypeID));

	slateDesc.m_IsQuaternion = attributeBaseTypeID == PopcornFX::BaseType_Quaternion;
	slateDesc.m_IsOneShotTrigger = decl->OneShotTrigger();

	// force vector dimension to 3 for quaternion, allow to display 3 float and use euler angles in editor
	slateDesc.m_VectorDimension = (!slateDesc.m_IsQuaternion) ? slateDesc.m_Traits->VectorDimension : 3;

	slateDesc.m_DropDownMode = desc->m_DropDownMode;
	slateDesc.m_EnumList = desc->m_EnumList;
	slateDesc.m_EnumListIndices.SetNum(slateDesc.m_EnumList.Num());
	slateDesc.m_SharedEnumList.SetNum(slateDesc.m_EnumList.Num());
	for (s32 i = 0; i < slateDesc.m_EnumListIndices.Num(); ++i)
	{
		slateDesc.m_EnumListIndices[i] = MakeShareable(new int32(i)); // SEnumComboBox::Construct
		slateDesc.m_SharedEnumList[i] = MakeShared<FString>(slateDesc.m_EnumList[i]);
	}

	const FString		&name = desc->m_AttributeName;

	FString				description = ToUE(decl->Description().MapDefault()).Replace(TEXT("\\n"), TEXT("\n"));
	FString				shortDescription;
	int32				shortOffset;
	if (description.FindChar('\n', shortOffset))
		shortDescription = description.Left(shortOffset - 1);
	else
		shortDescription = description;

	// TODO: add an icon for quaternion attributes, right now fallbacks to F3
	const FString		typeName = (!slateDesc.m_IsQuaternion) ? GenerateTypeName(attributeBaseTypeID) : TEXT("F3");

	slateDesc.m_IsColor =
		slateDesc.m_Traits->IsFp && slateDesc.m_Traits->VectorDimension >= 3 &&
		(desc->m_AttributeSemantic == EPopcornFXAttributeSemantic::Type::AttributeSemantic_Color || name.Contains(TEXT("color")) || name.Contains(TEXT("colour")) || name.Contains(TEXT("rgb")));
	PK_ASSERT(slateDesc.m_IsColor || desc->m_AttributeSemantic != EPopcornFXAttributeSemantic::Type::AttributeSemantic_Color); // Something is wrong

	// force min and max for quaternion
	if (!slateDesc.m_IsQuaternion)
	{
		slateDesc.m_HasMin = decl->HasMin();
		slateDesc.m_HasMax = decl->HasMax();

		slateDesc.m_Min = decl->GetMinValue();
		slateDesc.m_Max = decl->GetMaxValue();
	}
	else
	{
		// for quaternion min = 0.f, max = 360.f
		PopcornFX::SAttributesContainer_SAttrib quaternionAttribMin;
		quaternionAttribMin.m_Data32f[0] = quaternionAttribMin.m_Data32f[1] = quaternionAttribMin.m_Data32f[2] = 0.f;

		PopcornFX::SAttributesContainer_SAttrib quaternionAttribMax;
		quaternionAttribMax.m_Data32f[0] = quaternionAttribMax.m_Data32f[1] = quaternionAttribMax.m_Data32f[2] = 360.f;

		slateDesc.m_HasMin = true;
		slateDesc.m_HasMax = true;

		slateDesc.m_Min = quaternionAttribMin;
		slateDesc.m_Max = quaternionAttribMax;
	}

	slateDesc.m_Def = decl->GetDefaultValue();

	TSharedPtr<IPropertyHandle> rawDataPty = m_AttributeListPty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPopcornFXAttributeList, m_AttributesRawData));
	if (!PK_VERIFY(rawDataPty.IsValid()))
		return;
	FDetailWidgetDecl	*nameContent = nullptr;

	if (slateDesc.m_VectorDimension == 1)
	{
		FDetailWidgetRow	&customWidget = m_IGroups[iCategory]->AddWidgetRow();
		nameContent = &customWidget.NameContent();

		customWidget.ValueContent()
		[
			SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(false).PropertyHandle(rawDataPty)
		];
		customWidget.ResetToDefaultContent()
			[
				SNew(SButton)
					.OnClicked(this, &FPopcornFXCustomizationAttributeList::OnResetClicked, slateDesc)
					.Visibility(this, &FPopcornFXCustomizationAttributeList::GetResetVisibility, slateDesc)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value"))
					.ButtonColorAndOpacity(FSlateColor::UseForeground())
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.Content()
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
			];
	}
	else
	{
		IDetailGroup		&group = m_IGroups[iCategory]->AddGroup(desc->AttributeFName(), FText::FromString(desc->m_AttributeName));
		
		FDetailWidgetRow	&headerRow = group.HeaderRow();
		nameContent = &headerRow.NameContent();
			
		for (u32 dimi = 0; dimi < slateDesc.m_VectorDimension; ++dimi)
		{
			group.AddWidgetRow()
				.NameContent()
				[
					SNew(STextBlock)
						.Text(s_AxisTexts[(slateDesc.m_IsQuaternion) ? 2 : slateDesc.m_IsColor][dimi])
						.Font(m_DetailLayoutBuilder->GetDetailFont())
				]
				.ValueContent()
				[
					SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(true).Dimi(dimi).PropertyHandle(rawDataPty)
				]
				.ResetToDefaultContent()
				[
					SNew(SButton)
						.OnClicked(this, &FPopcornFXCustomizationAttributeList::OnDimResetClicked, dimi, slateDesc)
						.Visibility(this, &FPopcornFXCustomizationAttributeList::GetDimResetVisibility, dimi, slateDesc)
						.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value"))
						.ButtonColorAndOpacity(FSlateColor::UseForeground())
						.ButtonStyle(FAppStyle::Get(), "NoBorder")
						.Content()
						[
							SNew(SImage)
								.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
						]
				];
		}
		headerRow.ValueContent()
			[
				SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(false).PropertyHandle(rawDataPty)
			];
		headerRow.ResetToDefaultContent()
			[
				SNew(SButton)
					.OnClicked(this, &FPopcornFXCustomizationAttributeList::OnResetClicked, slateDesc)
					.Visibility(this, &FPopcornFXCustomizationAttributeList::GetResetVisibility, slateDesc)
					.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value"))
					.ButtonColorAndOpacity(FSlateColor::UseForeground())
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.Content()
					[
						SNew(SImage)
							.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
					]
			];
	}

	if (nameContent)
	{
		(*nameContent)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
							.WidthOverride(16)
							.HeightOverride(16)
							[
								SNew(SImage)
									.Image(FPopcornFXStyle::GetBrush(*(TEXT("PopcornFX.Attribute.") + typeName)))
									.ColorAndOpacity(FSlateColor::UseForeground())
							]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f)
					[
						SNew(STextBlock)
							.Text(FText::FromString(name))
							.ToolTipText(FText::FromString(description))
							.Font(m_DetailLayoutBuilder->GetDetailFont())
					]
			];
	}
}

void	FPopcornFXCustomizationAttributeList::RebuildAttributes()
{
	ATTRDEBUB_LOG(LogPopcornFXCustomizationAttributeList, Log, TEXT("rebuild attributes"));

	const FPopcornFXAttributeList	*attrList = AttrList();

	// Categories are only contained in the effect's DefaultAttributeList
	const FPopcornFXAttributeList	*defAttrList = null;

	defAttrList = attrList->GetDefaultAttributeList(attrList->Effect());
	// Attribute serialization can break in blueprints. In that case, all the attributes are defaulted
	// i.e their type is set to invalid. Easy to check. Just rebuild all
	if (attrList->AttributeDescCount() > 0 && !attrList->m_AttributeDescs[0].ValidAttributeType())
	{
		if (m_IGroups.Num() > 0 || m_NumAttributes.Num() > 0)
		{
			// Wrong state, rebuild everything
			m_IGroups[0]->AddWidgetRow()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.Padding(4.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(FText::FromString("Attribute list is in an invalid state"))
								.Font(m_DetailLayoutBuilder->GetDetailFont())
								.ColorAndOpacity(USlateThemeManager::Get().GetColor(EStyleColor::Error))
						]
				];
			m_IGroups.Empty();
			m_NumAttributes.Empty();
			if (PK_VERIFY(m_PropertyUtilities.IsValid()))
				m_PropertyUtilities->ForceRefresh();
		}
		return;
	}

	PK_ASSERT((int32)defAttrList->GetCategoryCount() == m_IGroups.Num());

	// Browse back name list
	const u32	categoryCount = defAttrList->GetCategoryCount();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		const uint32	attrCount = attrList->AttributeDescCount();
		for (u32 attri = 0; attri < attrCount; ++attri)
		{
			const FPopcornFXAttributeDesc	*desc = attrList->GetAttributeDesc(attri);
			check(desc != null);
			
			if (desc->m_IsPrivate)
				continue;

			if (desc->m_AttributeCategoryName != defAttrList->GetCategoryName(iCategory))
			{
				if ((!desc->m_AttributeCategoryName.IsEmpty()) || iCategory > 0)
					continue;
			}
			BuildAttribute(desc, attrList, attri, iCategory);
			m_NumAttributes[iCategory]++;
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeList::BuildSampler(const FPopcornFXSamplerDesc *desc, const TSharedPtr<IPropertyHandle> samplerDescPty, FPopcornFXAttributeList *attrList, uint32 sampleri, uint32 iCategory)
{
	const FString		&name = desc->m_SamplerName;

	FString				defNode;
	FName				samplerIconName;

	UPopcornFXEffect *effect = attrList->Effect();
	if (effect == null)
		return;

	// TODO(Attributes refactor): This should not use 'CParticleNodeSamplerData'
	const PopcornFX::CParticleAttributeSamplerDeclaration *particleSampler = static_cast<const PopcornFX::CParticleAttributeSamplerDeclaration *>(attrList->GetParticleSampler(effect, sampleri));
	if (particleSampler == null)
		return;

	const char *nodeName = ResolveAttribSamplerNodeName(particleSampler, desc->m_SamplerType);
	if (nodeName != null)
	{
		defNode = nodeName;
		samplerIconName = FName(*("PopcornFX.Node." + defNode));
	}
	else
	{
		defNode = "?????";
		samplerIconName = FName(TEXT("PopcornFX.BadIcon32"));
	}

	// Editing an emitter: the object being edited is the emitter
	// Editing an effect asset: the object being edited is the effect, emitter is effect->PreviewEmitter
	UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(m_BeingCustomized[0]);
	if (emitter == nullptr)
	{
		emitter = Cast<UPopcornFXEmitterComponent>(effect->PreviewEmitter);
	}
	if (!PK_VERIFY(emitter != null))
	{
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Could not retrieve the emitter associated with this sampler"));
		return;
	}

	FPopcornFXAttributeSampler *sampler = attrList->ResolveAttributeSampler(sampleri);
	if (sampler == nullptr)
		return;

	const FPopcornFXAttributeSamplerProperties	*properties = desc->ResolveAttributeProperties();

	FSlateColor		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Foreground);
	FText			tooltipText = FText::FromString(name + ": " + defNode);
	if (properties && ((emitter->m_IncompatibleProperties.Contains(properties) && !emitter->m_IncompatibleProperties[properties].m_Properties.IsEmpty())
		|| properties->m_UnsupportedProperties.Num() > 0))
	{
		samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Error);
		tooltipText = FText::FromString("One or more properties are not supported. Default values exported from PopcornFX will be used");
	}

	TSharedPtr<IPropertyHandle>	samplerAssetPty;
	if (desc->m_UseSamplerAsset)
	{
		// Adds properties to reference an external sampler
		samplerAssetPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_SamplerAsset));
		UObject *samplerAsset = nullptr;
		samplerAssetPty->GetValue(samplerAsset);
		if (samplerAsset == nullptr)
		{
			samplerNameColor = USlateThemeManager::Get().GetColor(EStyleColor::Error);
			tooltipText = FText::FromString("No sampler asset");
		}
	}

	IDetailGroup		&newGroup = m_IGroups[iCategory]->AddGroup(FName(desc->m_SamplerName), FText::FromString(desc->m_SamplerName));
	FDetailWidgetRow	&headerRow = newGroup.HeaderRow();
	FDetailWidgetDecl	*headerRowContent = &headerRow.WholeRowContent();
	(*headerRowContent)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(1.f)
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
						.WidthOverride(16)
						.HeightOverride(16)
						[
							SNew(SImage)
								.Image(FPopcornFXStyle::GetBrush(samplerIconName))
								.ColorAndOpacity(samplerNameColor)
						]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(name))
						.ToolTipText(tooltipText)
						.Font(m_DetailLayoutBuilder->GetDetailFont())
						.ColorAndOpacity(samplerNameColor)
				]
		];

	if (!samplerDescPty.IsValid() || !samplerDescPty->IsValidHandle())
	{
		return;
	}

	TSharedPtr<IPropertyHandle>	useSamplerAssetPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_UseSamplerAsset));
	PK_ASSERT(useSamplerAssetPty.IsValid() && useSamplerAssetPty->IsValidHandle());
	if (!useSamplerAssetPty.IsValid() || !useSamplerAssetPty->IsValidHandle())
		return;

	IDetailPropertyRow &useSamplerAssetPtyRow = newGroup.AddPropertyRow(useSamplerAssetPty.ToSharedRef()).DisplayName(FText::FromString("Use sampler asset?"));
	TSharedPtr<SWidget> defaultNameWidget;
	TSharedPtr<SWidget> defaultValueWidget;
	useSamplerAssetPtyRow.GetDefaultWidgets(defaultNameWidget, defaultValueWidget);

	useSamplerAssetPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeList::UpdateSampler, desc, sampler));

	//TSharedPtr<IPropertyHandle>	restartWhenSamplerChangesPty = samplerDescPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_RestartWhenSamplerChanges));
	//PK_ASSERT(restartWhenSamplerChangesPty.IsValid() && restartWhenSamplerChangesPty->IsValidHandle());
	//newGroup.AddPropertyRow(restartWhenSamplerChangesPty.ToSharedRef()).DisplayName(FText::FromString("Restart when sampler changes?"))
	//	.EditCondition(desc->m_UseSamplerAsset, FOnBooleanValueChanged()).EditConditionHides(true);

	if (desc->m_UseSamplerAsset && samplerAssetPty.IsValid() && samplerAssetPty->IsValidHandle())
	{
		IDetailPropertyRow &SyncDisplayProperty = newGroup.AddPropertyRow(samplerAssetPty.ToSharedRef()).DisplayName(FText::FromString("Sampler asset"));
		TSharedPtr<SWidget> nameWidget;
		TSharedPtr<SWidget> valueWidget;
		SyncDisplayProperty.GetDefaultWidgets(nameWidget, valueWidget);
		samplerAssetPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeList::UpdateSampler, desc, sampler));
		// This erases the current Name, Value and ResetToDefault widgets, don't forget to set them back if you want them!
		SyncDisplayProperty.CustomWidget(true)
			.NameContent()
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(samplerAssetPty->GetPropertyDisplayName())
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
							.ColorAndOpacity(samplerNameColor)
					]
			]
		.ValueContent() // Set the default Value widget back
			[
				valueWidget.ToSharedRef()
			];
	}
	else
	{
		TSharedPtr<IPropertyHandle>	samplerStructPty = ResolveSamplerProperties(samplerDescPty, desc->SamplerType(), defNode);
		if (samplerStructPty.IsValid() && samplerStructPty->IsValidHandle())
		{
			// Adds a custom property row, see PropertyCustomization/PopcornFXCustomizationAttributeSamplerXXX for each sampler
			newGroup.AddPropertyRow(samplerStructPty.ToSharedRef());
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeList::RebuildSamplers()
{
	if (!m_SamplersDescPty->IsValidHandle())
	{
		return;
	}

	FPopcornFXAttributeList *attrList = AttrList();

	// Categories are only contained in the effect's DefaultAttributeList
	const FPopcornFXAttributeList *defAttrList = attrList->GetDefaultAttributeList(attrList->Effect());

	if ((int32)defAttrList->GetCategoryCount() != m_IGroups.Num())
		return;

	const u32	categoryCount = defAttrList->GetCategoryCount();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		const uint32	samplerCount = attrList->SamplerDescCount();
		for (u32 sampleri = 0; sampleri < samplerCount; ++sampleri)
		{
			FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(sampleri);
			check(desc != null);
			if (desc->m_SamplerType == EPopcornFXAttributeSamplerType::None || desc->m_IsPrivate)
				continue;
			if (desc->m_AttributeCategoryName != defAttrList->GetCategoryName(iCategory))
			{
				if ((!desc->m_AttributeCategoryName.IsEmpty()) || iCategory > 0)
					continue;
			}

			TSharedPtr<IPropertyHandle> samplerDescPty = m_SamplersDescPty->GetChildHandle(sampleri);
			BuildSampler(desc, samplerDescPty, attrList, sampleri, iCategory);
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeList::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow &HeaderRow, IPropertyTypeCustomizationUtils &CustomizationUtils)
{

}

//----------------------------------------------------------------------------

void	FPopcornFXCustomizationAttributeList::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder &ChildBuilder, IPropertyTypeCustomizationUtils &CustomizationUtils)
{
	m_PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	m_ChildBuilder = &ChildBuilder;
	PropertyHandle->GetOuterObjects(m_BeingCustomized);

	m_AttributeListPty = PropertyHandle;
	FPopcornFXAttributeList *attrList = AttrList();
	if (attrList)
		attrList->OnSamplersRefreshed.AddThreadSafeSP(this, &FPopcornFXCustomizationAttributeList::RebuildIFN);
	if (!PK_VERIFY(IsValidHandle(m_AttributeListPty)))
	{
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Can't retrieve attribute list property!"));
		return;
	}

	m_AttributesRawDataPty = m_AttributeListPty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPopcornFXAttributeList, m_AttributesRawData));
	if (!PK_VERIFY(IsValidHandle(m_AttributesRawDataPty)))
	{
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Can't retrieve attributes raw data description property!"));
		return;
	}

	m_SamplersDescPty = m_AttributeListPty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPopcornFXAttributeList, m_SamplerDescs));
	if (!PK_VERIFY(IsValidHandle(m_SamplersDescPty)))
	{
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Can't retrieve samplers desc property!"));
		return;
	}

	TSharedPtr<IPropertyHandle> effectPty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPopcornFXAttributeList, m_Effect));
	if (!PK_VERIFY(IsValidHandle(effectPty)))
	{
		UE_LOG(LogPopcornFXCustomizationAttributeList, Error, TEXT("Can't retrieve effect property!"));
		return;
	}
	effectPty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPopcornFXCustomizationAttributeList::RebuildIFN));

	Rebuild();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR

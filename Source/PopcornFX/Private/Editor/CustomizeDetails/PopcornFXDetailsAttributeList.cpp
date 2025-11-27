//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeList.h"

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
#include "ScopedTransaction.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Colors/SColorPicker.h"

#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeList"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsAttributeList, Log, All);

//#define ATTRDEBUB_LOG		UE_LOG
#define ATTRDEBUB_LOG(...)	do { } while(0)

//----------------------------------------------------------------------------

UPopcornFXEffect	*ResolveEffect(const UPopcornFXAttributeList *attrList)
{
	if (attrList == null)
		return null;
	UPopcornFXEmitterComponent	*cpnt = Cast<UPopcornFXEmitterComponent>(attrList->GetOuter());
	if (cpnt != null)
		return cpnt->Effect;
	UPopcornFXEffect			*effect = Cast<UPopcornFXEffect>(attrList->GetOuter());
	return effect;
}

//----------------------------------------------------------------------------

EPopcornFXAttribSamplerShapeType::Type	ToUEShapeType(PopcornFX::CShapeDescriptor::EShapeType pkShapeType)
{
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Box				== (u32)PopcornFX::CShapeDescriptor::ShapeBox);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Sphere			== (u32)PopcornFX::CShapeDescriptor::ShapeSphere);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Ellipsoid		== (u32)PopcornFX::CShapeDescriptor::ShapeEllipsoid);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cylinder			== (u32)PopcornFX::CShapeDescriptor::ShapeCylinder);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Capsule			== (u32)PopcornFX::CShapeDescriptor::ShapeCapsule);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cone				== (u32)PopcornFX::CShapeDescriptor::ShapeCone);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::StaticMesh		== (u32)PopcornFX::CShapeDescriptor::ShapeMesh);
	PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::SkeletalMesh - 1	== (u32)PopcornFX::CShapeDescriptor::ShapeMesh);
	//PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Collection	== (u32)PopcornFX::CShapeDescriptor::ShapeMeshCollection);
	return static_cast<EPopcornFXAttribSamplerShapeType::Type>(pkShapeType);
}

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
		case	EPopcornFXAttributeSamplerType::Turbulence:
			return "AttributeSampler_VectorField";
		default:
			PK_ASSERT_NOT_REACHED();
			return null;
	}
}

//----------------------------------------------------------------------------

TSharedPtr<IPropertyHandle>		ResolveSamplerProperties(const TSharedPtr<IPropertyHandle> samplerPty, EPopcornFXAttributeSamplerType::Type samplerType, const FString &samplerTypeName)
{
	TSharedPtr<IPropertyHandle>	samplerPropertiesPty;
	switch (samplerType)
	{
	case EPopcornFXAttributeSamplerType::Type::AnimTrack:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerAnimTrack, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Image:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerImage, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Grid:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerGrid, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Curve:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerCurve, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Shape:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerShape, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Text:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerText, Properties));
		break;
	case EPopcornFXAttributeSamplerType::Type::Turbulence:
		samplerPropertiesPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(UPopcornFXAttributeSamplerVectorField, Properties));
		break;
	default:
		UE_LOG(LogPopcornFXDetailsAttributeList, Error, TEXT("Sampler type '%s' not implemented"), *samplerTypeName);
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

// Copy-Pasted form Editor/PropertyEditor/Private/SDetailSingleItemRow.cpp
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

//SNumericEntryBox<float>::RedLabelBackgroundColor,
//SNumericEntryBox<float>::GreenLabelBackgroundColor,
//SNumericEntryBox<float>::BlueLabelBackgroundColor,

const FLinearColor		s_AxisColors[] = {
	FLinearColor(0.594f,0.0197f,0.0f),
	FLinearColor(0.1349f,0.3959f,0.0f),
	FLinearColor(0.0251f,0.207f,0.85f),
	FLinearColor::Black,
	//FLinearColor::Red,
	//FLinearColor::Green,
	//FLinearColor::Blue,
	//FLinearColor::Black,
};

template <typename _Scalar>
struct IsFloat
{
	enum { Value = false };
};

template <>
struct IsFloat<float>
{
	enum { Value = true };
};

//float SDetailTableRowBase::ScrollbarPaddingSize = 16.0f
static const float	kDefaultScrollBarPaddingSize = 16.0f;

// Data needed to craft a widget representing an attribute
struct FAttributeDesc
{
public:
	typedef FAttributeDesc					TSelf;
	typedef FPopcornFXDetailsAttributeList	TParent;

	TWeakPtr<TParent>						m_Parent;

	bool									m_ReadOnly = false;

	uint32									m_Index = 0;
	u32										m_VectorDimension = 0;
	const PopcornFX::CBaseTypeTraits*		m_Traits = null;

	FText									m_Title;
	FText									m_Description;
	FText									m_ShortDescription;

	bool									m_IsColor = false;
	bool									m_IsQuaternion = false;
	bool									m_IsOneShotTrigger = false;
	EPopcornFXAttributeDropDownMode::Type	m_DropDownMode;
	TArray<FString>							m_EnumList; // copy
	TArray<TSharedPtr<int32>>				m_EnumListIndices;
	bool									m_HasMin;
	bool									m_HasMax;

	PopcornFX::SAttributesContainer_SAttrib	m_Min;
	PopcornFX::SAttributesContainer_SAttrib	m_Max;
	PopcornFX::SAttributesContainer_SAttrib	m_Def;
};

// A widget representing an attribute
struct SAttributeWidget : SCompoundWidget
{
	typedef SAttributeWidget				TSelf;
	typedef FPopcornFXDetailsAttributeList	TParent;

	FAttributeDesc							m_SlateDesc;
	bool									m_IsExpanded = false;
	uint32									m_Dimi = 0;

	SLATE_BEGIN_ARGS(SAttributeWidget) { }
		SLATE_ARGUMENT(FAttributeDesc, SlateDesc)
		SLATE_ARGUMENT(TWeakPtr<FPopcornFXDetailsAttributeList>, Parent)
		SLATE_ARGUMENT(TOptional<bool>, Expanded)
		SLATE_ARGUMENT(TOptional<uint32>, Dimi)
	SLATE_END_ARGS()

	void			Construct(const FArguments &InArgs)
	{
		m_SlateDesc = InArgs._SlateDesc;
		m_IsExpanded = InArgs._Expanded.Get(false);
		m_Dimi = InArgs._Dimi.Get(0);

		TSharedPtr<SHorizontalBox>	hbox;
		SAssignNew(hbox, SHorizontalBox);
		if (m_IsExpanded)
		{
			hbox->AddSlot()
				.VAlign(VAlign_Center)
				.FillWidth(1.0f)
				.Padding(3.0f, 1.0f, 2.0f, 1.0f)
				[
					SNew(SMyConstrainedBox)
						.MinWidth(125.f)
						.MaxWidth(125.f)
						[
							MakeAxis(m_Dimi)
						]
				];
		}
		else
		{
			if (m_SlateDesc.m_DropDownMode == EPopcornFXAttributeDropDownMode::AttributeDropDownMode_SingleSelect)
			{
				hbox->AddSlot()
					.VAlign(VAlign_Center)
					.FillWidth(1.0f)
					.Padding(3.0f, 1.0f, 2.0f, 1.0f)
					[
						SNew(SMyConstrainedBox)
							.MinWidth(125.f)
							.MaxWidth(125.f)
							[
								MakeSingleSelectEnum()
							]
					];
			}
			else
			{
				for (u32 dimi = 0; dimi < m_SlateDesc.m_VectorDimension; ++dimi)
				{
					hbox->AddSlot()
						.VAlign(VAlign_Center)
						.FillWidth(1.0f)
						.Padding(3.0f, 1.0f, 2.0f, 1.0f)
						[
							SNew(SMyConstrainedBox)
								.MinWidth(125.f)
								.MaxWidth(125.f)
								[
									MakeAxis(dimi)
								]
						];
				}
			}
			hbox->AddSlot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SBox)
						.WidthOverride(20)
						.HeightOverride(20)
						[
							SNew(SButton)
								.ButtonStyle(FCoreStyle::Get(), "NoBorder")
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Right)
								.Visibility(this, &SAttributeWidget::GetColorPickerVisibility)
								.ClickMethod(EButtonClickMethod::MouseDown)
								.OnClicked(this, &SAttributeWidget::OnColorPickerClicked)
								.ContentPadding(5.f)
								.ForegroundColor(FSlateColor::UseForeground())
								.IsFocusable(false)
								.ToolTipText(LOCTEXT("DisplayColorPickerTooltip", "Color Picker"))
								[
									SNew(SImage)
										.Image(FCoreStyle::Get().GetBrush("ColorPicker.Mode"))
								]
						]
				];
			hbox->AddSlot()
				.Padding(1.f)
				.AutoWidth()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					MakeResetButton()
				];
		}
		ChildSlot
		[
			hbox.ToSharedRef()
		];
	}

	EVisibility		GetColorPickerVisibility() const
	{
		return m_SlateDesc.m_IsColor ? EVisibility::Visible : EVisibility::Hidden;
	}

	FReply			OnColorPickerClicked()
	{
		TSharedPtr<TParent>		parent = m_SlateDesc.m_Parent.Pin();
		if (!parent.IsValid())
			return FReply::Unhandled();
		UPopcornFXAttributeList	*attrList = parent->AttrList();
		check(attrList != null);

		PopcornFX::SAttributesContainer_SAttrib	attribValue;
		attrList->GetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

		const float	r = attribValue.m_Data32f[0];
		const float	g = attribValue.m_Data32f[1];
		const float	b = attribValue.m_Data32f[2];
		const float	a = attribValue.m_Data32f[3];

		FLinearColor		initialColor(r, g, b, a);
		FColorPickerArgs	pickerArgs;
		{
			pickerArgs.bUseAlpha = m_SlateDesc.m_Traits->VectorDimension == 4;
			pickerArgs.bOnlyRefreshOnMouseUp = false;
			pickerArgs.bOnlyRefreshOnOk = false;
			pickerArgs.sRGBOverride = false;//sRGBOverride;
			pickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
			pickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SAttributeWidget::OnSetColorFromColorPicker);
			pickerArgs.InitialColor = initialColor;
			//pickerArgs.ParentWidget = parent;
			//pickerArgs.OptionalOwningDetailsView = parent;
		}

		OpenColorPicker(pickerArgs);
		return FReply::Handled();
	}

	void			OnSetColorFromColorPicker(FLinearColor newColor)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		TSharedPtr<TParent>		parent = m_SlateDesc.m_Parent.Pin();
		if (!parent.IsValid())
			return;
		UPopcornFXAttributeList	*attrList = parent->AttrList();
		check(attrList != null);
		UPopcornFXEffect		*effect = ResolveEffect(attrList);
		if (effect == null)
			return;
		// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
		const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, m_SlateDesc.m_Index));
		if (decl == null)
			return;

		attrList->SetFlags(RF_Transactional);

		const FScopedTransaction	Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit as Color"));

		attrList->Modify();
		{
			PopcornFX::SAttributesContainer_SAttrib	attribValue;
			attribValue.m_Data32f[0] = newColor.R;
			attribValue.m_Data32f[1] = newColor.G;
			attribValue.m_Data32f[2] = newColor.B;
			if (m_SlateDesc.m_Traits->VectorDimension > 3)
				attribValue.m_Data32f[3] = newColor.A;
			decl->ClampToRangeIFN(attribValue);
			attrList->SetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue), true); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
		}
		attrList->PostEditChange();
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

	TSharedRef<SWidget>	MakeSingleSelectEnum()
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
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));
					})
				.Content()
				[
					SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
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
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"));
					})
				.OnSelectionChanged(sharedThis, &TSelf::OnValueChangedEnum)
				.Content()
				[
					SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
				];
		}
		return comboBox.ToSharedRef();
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

	template <typename _Scalar>
	TSharedRef<SWidget>		_MakeAxis(uint32 dimi)
	{
		// Access a shared reference to 'this'
		TSharedRef<TSelf> sharedThis = SharedThis(this);

		check(dimi < m_SlateDesc.m_Traits->VectorDimension);

		const bool		isColor = m_SlateDesc.m_IsColor;
		const bool		isQuaternion = m_SlateDesc.m_IsQuaternion;

		TSharedPtr< SNumericEntryBox<_Scalar> >	axis;
		if (m_SlateDesc.m_ReadOnly)
		{
			SAssignNew(axis, SNumericEntryBox<_Scalar>)
				.AllowSpin(true)

				.MinValue(m_SlateDesc.m_HasMin ? TOptional<_Scalar>(m_SlateDesc.m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MaxValue(m_SlateDesc.m_HasMax ? TOptional<_Scalar>(m_SlateDesc.m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MinSliderValue(m_SlateDesc.m_HasMin ? TOptional<_Scalar>(m_SlateDesc.m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MaxSliderValue(m_SlateDesc.m_HasMax ? TOptional<_Scalar>(m_SlateDesc.m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())

				.Value(sharedThis, &TSelf::GetValue<_Scalar>, dimi)

				.LabelVAlign(VAlign_Center)
				.LabelPadding(0)
				.Label()
				[
					SNumericEntryBox<float>::BuildLabel(s_AxisTexts[(isQuaternion) ? 2 : isColor][dimi], FSlateColor::UseForeground(), s_AxisColors[dimi])
				];
		}
		else
		{
			SAssignNew(axis, SNumericEntryBox<_Scalar>)
				.AllowSpin(true)

				.MinValue(m_SlateDesc.m_HasMin ? TOptional<_Scalar>(m_SlateDesc.m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MaxValue(m_SlateDesc.m_HasMax ? TOptional<_Scalar>(m_SlateDesc.m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MinSliderValue(m_SlateDesc.m_HasMin ? TOptional<_Scalar>(m_SlateDesc.m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
				.MaxSliderValue(m_SlateDesc.m_HasMax ? TOptional<_Scalar>(m_SlateDesc.m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())

				.Value(sharedThis, &TSelf::GetValue<_Scalar>, dimi)
				.OnValueChanged(sharedThis, &TSelf::OnValueChanged<_Scalar>, dimi)
				.OnValueCommitted(sharedThis, &TSelf::OnValueCommitted<_Scalar>, dimi)

				.LabelVAlign(VAlign_Center)
				.LabelPadding(0)
				.Label()
				[
					SNumericEntryBox<float>::BuildLabel(s_AxisTexts[(isQuaternion) ? 2 : isColor][dimi], FSlateColor::UseForeground(), s_AxisColors[dimi])
				];
		}
		return axis.ToSharedRef();
	}

	TSharedRef<SWidget>		MakeResetButton(uint32 dimi)
	{
		return SNew(SButton)
			.OnClicked(this, &TSelf::OnDimResetClicked, dimi)
			.Visibility(this, &TSelf::GetDimResetVisibility, dimi)
			.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.Content()
			[
				SNew(SImage)
					.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
			];
	}

	TSharedRef<SWidget>		MakeResetButton()
	{
		return SNew(SButton)
			.OnClicked(this, &TSelf::OnResetClicked)
			.Visibility(this, &TSelf::GetResetVisibility)
			.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset this property to its default value"))
			.ButtonColorAndOpacity(FSlateColor::UseForeground())
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.Content()
			[
				SNew(SImage)
					.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
			];
	}

	bool			_GetAttrib(UPopcornFXAttributeList *&outAttribList) const
	{
		TSharedPtr<TParent>		parent = m_SlateDesc.m_Parent.Pin();

		if (!parent.IsValid())
			return false;
		outAttribList = parent->AttrList();
		return outAttribList != null;
	}

	template <typename _Scalar>
	TOptional<_Scalar>		GetValue(uint32 dimi) const
	{
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return TOptional<_Scalar>();

		if (!m_SlateDesc.m_IsQuaternion)
			return attrList->GetAttributeDim<_Scalar>(m_SlateDesc.m_Index, dimi);
		else
			return attrList->GetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi);
	}

	ECheckBoxState	GetValueBool(uint32 dimi) const
	{
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return ECheckBoxState::Undetermined;
		return attrList->GetAttributeDim<bool>(m_SlateDesc.m_Index, dimi) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	void	OnValueChangedBool(ECheckBoxState value, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return;

		attrList->SetFlags(RF_Transactional);

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		attrList->Modify();
		attrList->SetAttributeDim<bool>(m_SlateDesc.m_Index, dimi, value == ECheckBoxState::Checked, true);
		attrList->PostEditChange();
	}

	FReply	OnValuePulsedBool(uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return FReply::Handled();
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return FReply::Handled();

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Pulse"));
		attrList->SetFlags(RF_Transactional);
		attrList->Modify();
		attrList->PulseBoolAttributeDim(m_SlateDesc.m_Index, dimi, true);
		attrList->PostEditChange();
		return FReply::Handled();
	}

	FText	GetValueEnumText() const
	{
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return FText();
		return FText::FromString(m_SlateDesc.m_EnumList[attrList->GetAttributeDim<int32>(m_SlateDesc.m_Index, 0)]);
	}

	void	OnValueChangedEnum(TSharedPtr<int32> selectedItem, ESelectInfo::Type selectInfo)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return;

		attrList->SetFlags(RF_Transactional);

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		attrList->Modify();
		attrList->SetAttributeDim<int32>(m_SlateDesc.m_Index, 0, *selectedItem, true);
		attrList->PostEditChange();
	}

	template <typename _Scalar>
	void		OnValueChanged(const _Scalar value, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return;
		//attrList->SetFlags(RF_Transactional);
		//attrList->Modify();

		if (!m_SlateDesc.m_IsQuaternion)
			attrList->SetAttributeDim<_Scalar>(m_SlateDesc.m_Index, dimi, value, true);
		else
			attrList->SetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi, value, true);
	}

	template <typename _Scalar>
	void		OnValueCommitted(const _Scalar value, ETextCommit::Type CommitInfo, uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return;
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return;

		attrList->SetFlags(RF_Transactional);

		const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

		//UE_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("--- DETAIL ATTRLIST commit %p %f --- %s"), attrList, float(value), *attrList->GetFullName());

		attrList->Modify();

		if (!m_SlateDesc.m_IsQuaternion)
			attrList->SetAttributeDim<_Scalar>(m_SlateDesc.m_Index, dimi, value, true);
		else
			attrList->SetAttributeQuaternionDim(m_SlateDesc.m_Index, dimi, value, true);

		attrList->PostEditChange();
	}

	FReply				OnResetClicked()
	{
		if (m_SlateDesc.m_ReadOnly)
			return FReply::Handled();

		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return FReply::Handled();

		const FScopedTransaction Transaction(LOCTEXT("AttributeReset", "Attribute Reset"));
		attrList->SetFlags(RF_Transactional);
		attrList->Modify();
		attrList->SetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&m_SlateDesc.m_Def), true); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
		attrList->PostEditChange();
		return FReply::Handled();
	}

	FReply				OnDimResetClicked(uint32 dimi)
	{
		if (m_SlateDesc.m_ReadOnly)
			return FReply::Handled();

		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return FReply::Handled();

		const FScopedTransaction Transaction(LOCTEXT("AttributeResetDim", "Attribute Reset Dimension"));
		attrList->SetFlags(RF_Transactional);
		attrList->Modify();

		if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
		{
			const bool	defaultValue = reinterpret_cast<const bool*>(m_SlateDesc.m_Def.Get<u32>())[dimi];
			attrList->SetAttributeDim<bool>(m_SlateDesc.m_Index, dimi, defaultValue, true);
		}
		else
		{
			const s32	defaultValue = m_SlateDesc.m_Def.Get<s32>()[dimi];
			attrList->SetAttributeDim<s32>(m_SlateDesc.m_Index, dimi, defaultValue, true);
		}
		attrList->PostEditChange();
		return FReply::Handled();
	}

	EVisibility			GetResetVisibility() const
	{
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return EVisibility::Hidden;

		PopcornFX::SAttributesContainer_SAttrib	attribValue;
		attrList->GetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

		if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
		{
			for (uint32 dimi = 0; dimi < m_SlateDesc.m_Traits->VectorDimension; ++dimi)
			{
				if (reinterpret_cast<bool*>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool*>(m_SlateDesc.m_Def.Get<uint32>())[dimi])
					return EVisibility::Visible;
			}
		}
		else
		{
			for (uint32 dimi = 0; dimi < m_SlateDesc.m_Traits->VectorDimension; ++dimi)
			{
				if (attribValue.Get<uint32>()[dimi] != m_SlateDesc.m_Def.Get<uint32>()[dimi])
					return EVisibility::Visible;
			}
		}
		return EVisibility::Hidden;
	}

	EVisibility			GetDimResetVisibility(uint32 dimi) const
	{
		UPopcornFXAttributeList	*attrList;
		if (!_GetAttrib(attrList))
			return EVisibility::Hidden;

		PopcornFX::SAttributesContainer_SAttrib	attribValue;
		attrList->GetAttribute(m_SlateDesc.m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

		if (m_SlateDesc.m_Traits->ScalarType == PopcornFX::BaseType_Bool)
		{
			if (reinterpret_cast<bool*>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool*>(m_SlateDesc.m_Def.Get<uint32>())[dimi])
				return EVisibility::Visible;
		}
		else
		{
			if (attribValue.Get<uint32>()[dimi] != m_SlateDesc.m_Def.Get<uint32>()[dimi])
				return EVisibility::Visible;
		}
		return EVisibility::Hidden;
	}
};

FPopcornFXDetailsAttributeList::FPopcornFXDetailsAttributeList()
:	m_RefreshQueued(false)
,	m_ColumnWidth(0.65f) // Same value as SDetailsViewBase
,	m_FileVersionId(0)
,	m_Effect(null)
{
	ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("FPopcornFXDetailsAttributeList ctor %p"), this);
}

FPopcornFXDetailsAttributeList::~FPopcornFXDetailsAttributeList()
{
	ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("FPopcornFXDetailsAttributeList dtor %p"), this);
}

UPopcornFXAttributeList				*FPopcornFXDetailsAttributeList::UnsafeAttrList()
{
	const TArray<TWeakObjectPtr<UObject> >	&objects = m_BeingCustomized;
	//m_DetailBuilder->GetObjectsBeingCustomized(objects);
	if (objects.Num() != 1)
		return null;
	UObject							*outer = objects[0].Get();
	UPopcornFXAttributeList			*attrList = null;
	APopcornFXEmitter				*emitter = null;
	UPopcornFXEmitterComponent		*emitterComponent = null;
	UPopcornFXEffect				*effect = null;

	if ((attrList = Cast<UPopcornFXAttributeList>(outer)) != null)
		outer = attrList->GetOuter();
	else if ((emitter = Cast<APopcornFXEmitter>(outer)) != null)
	{
		outer = emitter->PopcornFXEmitterComponent;
	}
	else
		outer = objects[0].Get();

	// try re-fetch attribute list from Outer to make sur everything is up to date

	if ((emitterComponent = Cast<UPopcornFXEmitterComponent>(outer)) != null)
	{
		// Don't try to display attributes on an emitter with no effect attached
		if (emitterComponent->Effect == null)
			return null;
		UPopcornFXAttributeList		*a = emitterComponent->GetAttributeList();
		if (!PK_VERIFY(a != null))
			return null;
		PK_ASSERT(attrList == null || attrList == a);
		attrList = a;
	}
	else if ((effect = Cast<UPopcornFXEffect>(outer)) != null)
	{
		UPopcornFXAttributeList		*a = effect->GetDefaultAttributeList();
		if (!PK_VERIFY(a != null))
			return null;
		PK_ASSERT(attrList == null || attrList == a);
		attrList = a;
	}
	return attrList;
}

void	FPopcornFXDetailsAttributeList::RebuildAndRefresh()
{
	m_RefreshQueued = false;

	const UPopcornFXAttributeList *attrList = UnsafeAttrList();

	// Categories are only contained in the effect's DefaultAttributeList
	const UPopcornFXAttributeList *defAttrList = null;
	UPopcornFXEffect *resolvedEffect = ResolveEffect(attrList);
	if (!resolvedEffect)
	{
		m_IGroups.Empty();
		m_NumAttributes.Empty();
		if (PK_VERIFY(m_PropertyUtilities.IsValid()))
			m_PropertyUtilities->ForceRefresh();
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
			UE_LOG(LogPopcornFXDetailsAttributeList, Error, TEXT("Could not retrieve categories!"));
		}
	}

	const u32	categoryCount = m_IGroups.Num();

	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		PK_ASSERT(defAttrList != null);
		m_IGroups[iCategory] = &m_AttributeListCategory->AddGroup(FName(defAttrList->GetCategoryName(iCategory)), FText::FromString(defAttrList->GetCategoryName(iCategory)));
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
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))

				]
		];
	}

	RebuildAttributes();
	RebuildSamplers();

	if (PK_VERIFY(m_PropertyUtilities.IsValid()))
		m_PropertyUtilities->ForceRefresh();
}

UPopcornFXAttributeList		*FPopcornFXDetailsAttributeList::AttrList()
{
	UPopcornFXAttributeList		*attrList = UnsafeAttrList();
	if (attrList == null)
	{
		if (!m_RefreshQueued) // Avoid queueing tons of events in the same frame
		{
			bool	rebuild = false;
			const u32	categoryCount = m_IGroups.Num();
			for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
				rebuild |= m_NumAttributes[iCategory] > 0;
			rebuild &= PK_VERIFY(m_PropertyUtilities.IsValid());

			// We need to clear the remaining attributes
			if (rebuild)
			{
				m_RefreshQueued = true;
				m_PropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeList::RebuildIFN));
			}
		}
		return null;
	}
	if (m_FileVersionId != attrList->FileVersionId() || // Effect was reimported
		m_Effect != attrList->Effect()) // Effect changed
	{
		if (!m_RefreshQueued) // Avoid queueing tons of events in the same frame
		{
			m_RefreshQueued = true;
			if (PK_VERIFY(m_PropertyUtilities.IsValid()))
				m_PropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeList::RebuildIFN));
		}
		return null;
	}
	return attrList;
}

void	FPopcornFXDetailsAttributeList::RebuildIFN()
{
	if (PK_VERIFY(m_PropertyUtilities.IsValid()))
		m_PropertyUtilities->ForceRefresh();
}

void	FPopcornFXDetailsAttributeList::Rebuild()
{
	const UPopcornFXAttributeList *attrList = UnsafeAttrList(); // AttrList() will ask for rebuild ifn, we dont want that here
	if (attrList != null)
	{
		m_FileVersionId = attrList->FileVersionId();
		m_Effect = attrList->Effect();

		if (!m_Effect->OnEffectReimported.IsBoundToObject(this))
		{
			m_Effect->OnEffectReimported.AddThreadSafeSP(this, &FPopcornFXDetailsAttributeList::RebuildIFN);
		}
	}
	UPopcornFXEmitterComponent *emitter = Cast<UPopcornFXEmitterComponent>(m_BeingCustomized[0].Get());
	if (emitter)
	{
		if (!emitter->OnRequestUIRefresh.IsBoundToObject(this))
		{
			emitter->OnRequestUIRefresh.AddThreadSafeSP(this, &FPopcornFXDetailsAttributeList::RebuildIFN);
		}
	}

	RebuildAndRefresh();
}

void	FPopcornFXDetailsAttributeList::BuildAttribute(const FPopcornFXAttributeDesc *desc, const UPopcornFXAttributeList *attrList, uint32 attri, uint32 iCategory)
{
	UPopcornFXEffect	*effect = ResolveEffect(attrList);
	if (effect == null)
		return;
	// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
	const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, attri));
	if (decl == null || decl->IsPrivate())
		return;

	const PopcornFX::EBaseTypeID	attributeBaseTypeID = (PopcornFX::EBaseTypeID)desc->AttributeBaseTypeID();

	FAttributeDesc		slateDesc;

	slateDesc.m_Parent = SharedThis(this);
	slateDesc.m_Index = attri;
	slateDesc.m_Traits = &(PopcornFX::CBaseTypeTraits::Traits(attributeBaseTypeID));

	slateDesc.m_IsQuaternion = attributeBaseTypeID == PopcornFX::BaseType_Quaternion;
	slateDesc.m_IsOneShotTrigger = decl->OneShotTrigger();

	// force vector dimension to 3 for quaternion, allow to display 3 float and use euler angles in editor
	slateDesc.m_VectorDimension = (!slateDesc.m_IsQuaternion) ? slateDesc.m_Traits->VectorDimension : 3;

	slateDesc.m_DropDownMode = desc->m_DropDownMode;
	slateDesc.m_EnumList = desc->m_EnumList;
	slateDesc.m_EnumListIndices.SetNum(slateDesc.m_EnumList.Num());
	for (s32 i = 0; i < slateDesc.m_EnumListIndices.Num(); ++i)
		slateDesc.m_EnumListIndices[i] = MakeShareable(new int32(i)); // SEnumComboBox::Construct

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

	FDetailWidgetDecl	*nameContent = nullptr;

	if (slateDesc.m_VectorDimension == 1)
	{
		FDetailWidgetRow	&customWidget = m_IGroups[iCategory]->AddWidgetRow();
		nameContent = &customWidget.NameContent();

		customWidget.ValueContent()
		[
			SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(false)
		];
	}
	else
	{
		IDetailGroup		&group = m_IGroups[iCategory]->AddGroup(desc->AttributeFName(), FText::FromString(desc->m_AttributeName));
		
		FDetailWidgetRow	&headerRow = group.HeaderRow();
		nameContent = &headerRow.NameContent();
			
		for (u32 dimi = 0; dimi < slateDesc.m_VectorDimension; ++dimi)
		{
			group.AddWidgetRow().ValueContent()
				[
					SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(true).Dimi(dimi)
				];
		}
		headerRow.ValueContent()
			[
				SNew(SAttributeWidget).SlateDesc(slateDesc).Expanded(false)
			];
	}

	if (nameContent)
	{
		(*nameContent)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(1.f)
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
					[
						SNew(STextBlock)
							.Text(FText::FromString(name))
							.ToolTipText(FText::FromString(description))
							.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
					]
			];
	}
}

void	FPopcornFXDetailsAttributeList::RebuildAttributes()
{
	ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("rebuild attributes"));

	bool							validAttributeList = false;
	const UPopcornFXAttributeList	*attrList = AttrList();

	// Categories are only contained in the effect's DefaultAttributeList
	const UPopcornFXAttributeList	*defAttrList = null;

	validAttributeList = attrList != null;
	if (validAttributeList)
	{
		defAttrList = attrList->GetDefaultAttributeList(ResolveEffect(attrList));
		validAttributeList = defAttrList != null;
	}
	if (!validAttributeList)
	{
		if (m_IGroups.Num() > 0 || m_NumAttributes.Num() > 0)
		{
			// Wrong state, rebuild everything
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
		const uint32	attrCount = attrList->AttributeCount();
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

void	FPopcornFXDetailsAttributeList::RebuildSamplers()
{
	if (!m_SamplersPty.IsValid() || !m_SamplersPty->IsValidHandle())
	{
		return;
	}

	const UPopcornFXAttributeList *attrList = AttrList();
	if (attrList == null)
		return;
	// Categories are only contained in the effect's DefaultAttributeList
	const UPopcornFXAttributeList *defAttrList = attrList->GetDefaultAttributeList(ResolveEffect(attrList));
	if (defAttrList == null)
		return;

	PK_ASSERT((int32)defAttrList->GetCategoryCount() == m_IGroups.Num());

	const u32	categoryCount = defAttrList->GetCategoryCount();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		const uint32	samplerCount = attrList->SamplerCount();
		for (u32 sampleri = 0; sampleri < samplerCount; ++sampleri)
		{
			const FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(sampleri);
			check(desc != null);
			if (desc->m_SamplerType == EPopcornFXAttributeSamplerType::None || desc->m_IsPrivate)
				continue;
			if (desc->m_AttributeCategoryName != defAttrList->GetCategoryName(iCategory))
			{
				if ((!desc->m_AttributeCategoryName.IsEmpty()) || iCategory > 0)
					continue;
			}
			// Remove emitter references on external samplers if they're not used anymore
			if (!desc->m_UseExternalSampler)
			{
				UPopcornFXEmitterComponent	*emitter = Cast<UPopcornFXEmitterComponent>(m_BeingCustomized[0].Get());
				if (emitter)
				{
					UPopcornFXAttributeSampler *sampler = desc->ResolveExternalAttributeSampler(emitter, null);
					if (sampler)
						sampler->m_EmittersUsingThis.Remove(emitter);
				}
			}

			TSharedPtr<IPropertyHandle> samplerDescPty = m_SamplersDescPty->GetChildHandle(sampleri);
			TSharedPtr<IPropertyHandle> samplerPty = m_SamplersPty->GetChildHandle(sampleri);
			BuildSampler(desc, samplerPty, samplerDescPty, attrList, sampleri, iCategory);
		}
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR

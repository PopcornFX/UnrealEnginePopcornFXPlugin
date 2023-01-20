//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDetailsAttributeList.h"

#include "Assets/PopcornFXEffect.h"
#include "PopcornFXEmitter.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeList.h"
#include "Editor/EditorHelpers.h"
#include "Editor/PopcornFXStyle.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "EditorStyleSet.h"

#include "Engine/Engine.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_samplers.h>
#include <pk_particles/include/ps_samplers_classes.h>
#include <pk_particles/include/ps_attributes.h>

#define LOCTEXT_NAMESPACE "PopcornFXDetailsAttributeList"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXDetailsAttributeList, Log, All);

//#define ATTRDEBUB_LOG		UE_LOG
#define ATTRDEBUB_LOG(...)	do { } while(0)

namespace
{
	UPopcornFXEffect		*ResolveEffect(const UPopcornFXAttributeList *attrList)
	{
		UPopcornFXEmitterComponent	*cpnt = Cast<UPopcornFXEmitterComponent>(attrList->GetOuter());
		if (cpnt != null)
			return cpnt->Effect;
		UPopcornFXEffect			*effect = Cast<UPopcornFXEffect>(attrList->GetOuter());
		return effect;
	}

	//----------------------------------------------------------------------------

	EPopcornFXAttribSamplerShapeType::Type	ToUEShapeType(PopcornFX::CShapeDescriptor::EShapeType pkShapeType)
	{
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Box			== (u32)PopcornFX::CShapeDescriptor::ShapeBox);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Sphere		== (u32)PopcornFX::CShapeDescriptor::ShapeSphere);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Ellipsoid	== (u32)PopcornFX::CShapeDescriptor::ShapeEllipsoid);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cylinder		== (u32)PopcornFX::CShapeDescriptor::ShapeCylinder);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Capsule		== (u32)PopcornFX::CShapeDescriptor::ShapeCapsule);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Cone			== (u32)PopcornFX::CShapeDescriptor::ShapeCone);
		PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Mesh			== (u32)PopcornFX::CShapeDescriptor::ShapeMesh);
		//PK_STATIC_ASSERT(EPopcornFXAttribSamplerShapeType::Collection	== (u32)PopcornFX::CShapeDescriptor::ShapeMeshCollection);
		return static_cast<EPopcornFXAttribSamplerShapeType::Type>(pkShapeType);
	}

	//----------------------------------------------------------------------------

	const char		*ResolveAttribSamplerNodeName(const PopcornFX::CParticleAttributeSamplerDeclaration *sampler, EPopcornFXAttributeSamplerType::Type samplerType)
	{
		if (!PK_VERIFY(sampler != null))
			return null;

		// Same as for ResolveAttribSamplerType, no need to discard everything that doesn't have a default descriptor
		switch (samplerType)
		{
			case	EPopcornFXAttributeSamplerType::Shape:
				{
					const PopcornFX::CParticleSamplerDescriptor	*desc = sampler->GetSamplerDefaultDescriptor().Get();
					if (desc == null || !PK_VERIFY(desc->SamplerTypeID() == PopcornFX::CParticleSamplerDescriptor_Shape::SamplerTypeID()))
						return "CParticleSamplerShape";

					const PopcornFX::CParticleSamplerDescriptor_Shape_Default	*shapeDesc = PopcornFX::checked_cast<const PopcornFX::CParticleSamplerDescriptor_Shape_Default*>(desc);
					if (!PK_VERIFY(shapeDesc->m_Shape != null))
						return "CParticleSamplerShape";
					return ResolveAttribSamplerShapeNodeName(ToUEShapeType(shapeDesc->m_Shape->ShapeType()));
				}
			case	EPopcornFXAttributeSamplerType::Curve:
				return "CParticleSamplerCurve";
			case	EPopcornFXAttributeSamplerType::Image:
				return "CParticleSamplerTexture";
			case	EPopcornFXAttributeSamplerType::AnimTrack:
				return "CParticleSamplerAnimTrack";
			case	EPopcornFXAttributeSamplerType::Text:
				return "CParticleSamplerText";
			case	EPopcornFXAttributeSamplerType::Turbulence:
				return "CParticleSamplerProceduralTurbulence";
			default:
				PK_ASSERT_NOT_REACHED();
				return null;
		}
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

		void Construct(const FArguments& InArgs)
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

	FString				GenerateTypeName(PopcornFX::EBaseTypeID typeId)
	{
		const PopcornFX::CBaseTypeTraits	&traits = PopcornFX::CBaseTypeTraits::Traits(typeId);
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

	class	SAttributeDesc : public SCompoundWidget
	{
	public:
		typedef SAttributeDesc						TSelf;
		typedef FPopcornFXDetailsAttributeList		TParent;

		SLATE_BEGIN_ARGS(SAttributeDesc)
		{}
			SLATE_ARGUMENT(int32, AttribI)
			SLATE_ARGUMENT(TWeakPtr<FPopcornFXDetailsAttributeList>, Parent)
			SLATE_ARGUMENT(TOptional<bool>, Dirtify)
			SLATE_ARGUMENT(TOptional<bool>, ReadOnly)
			SLATE_ARGUMENT(TOptional<bool>, Expanded)
			SLATE_END_ARGS()
		SAttributeDesc()
		{
			ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("SAttributeDesc ctor %p"), this);
		}

		~SAttributeDesc()
		{
			ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("SAttributeDesc dotr %p"), this);
		}

		void	OnLeftColumnResized(float InNewWidth)
		{
			// SDetailSingleItemRow.cpp:
			// This has to be bound or the splitter will take it upon itself to determine the size
			// We do nothing here because it is handled by the column size data
		}

		void	Construct(const FArguments& InArgs)
		{
			m_Parent = InArgs._Parent;
			m_Index = uint32(InArgs._AttribI);

			TSharedPtr<TParent>		parent = m_Parent.Pin();

			m_Dirtify = InArgs._Dirtify.Get(true);
			m_ReadOnly = InArgs._ReadOnly.Get(false);
			m_ExpandOnly = InArgs._Expanded.Get(false);

			if (!Init())
				return;

			TSharedPtr<SVerticalBox>	vbox;
			SAssignNew(vbox, SVerticalBox);

			if (!m_ExpandOnly)
			{
				TSharedPtr<SHorizontalBox>	attrDesc;
				TSharedPtr<SHorizontalBox>	attrValuesInline;

				SAssignNew(attrDesc, SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SAssignNew(m_ExpanderArrow, SButton)
						.ButtonStyle(FCoreStyle::Get(), "NoBorder")
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Visibility(this, &SAttributeDesc::GetExpanderVisibility)
						.ClickMethod(EButtonClickMethod::MouseDown)
						.OnClicked(this, &SAttributeDesc::OnArrowClicked)
						.ContentPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
						.ForegroundColor(FSlateColor::UseForeground())
						.IsFocusable(false)
						[
							SNew(SImage)
							.Image(this, &SAttributeDesc::GetExpanderImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(1.f)
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(20)
						.HeightOverride(20)
						[
							SNew(SImage)
							.Image(FPopcornFXStyle::GetBrush(m_AttributeIcon))
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(m_Title)
						.ToolTipText(m_Description)
					];

				SAssignNew(attrValuesInline, SHorizontalBox);
				if (m_DropDownMode == EPopcornFXAttributeDropDownMode::AttributeDropDownMode_SingleSelect)
				{
					attrValuesInline->AddSlot()
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
					for (u32 dimi = 0; dimi < m_VectorDimension; ++dimi)
					{
						attrValuesInline->AddSlot()
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
				attrValuesInline->AddSlot()
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
							.Visibility(this, &SAttributeDesc::GetColorPickerVisibility)
							.ClickMethod(EButtonClickMethod::MouseDown)
							.OnClicked(this, &SAttributeDesc::OnColorPickerClicked)
							.ContentPadding(0.f)
							.ForegroundColor(FSlateColor::UseForeground())
							.IsFocusable(false)
							.ToolTipText(LOCTEXT("DisplayColorPickerTooltip", "Color Picker"))
							[
								SNew(SImage)
								.Image(FCoreStyle::Get().GetBrush("ColorPicker.Mode"))
							]
						]
					];
				attrValuesInline->AddSlot()
					.Padding(1.f)
					.AutoWidth()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						MakeResetButton()
					];

				TSharedPtr<SSplitter>		inlineSplitter;
				SAssignNew(inlineSplitter, SSplitter)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.Style(FAppStyle::Get(), "DetailsView.Splitter")
#else
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.PhysicalSplitterHandleSize(1.0f)
				.HitDetectionSplitterHandleSize(5.0f)
					+ SSplitter::Slot()
					.Value(parent->LeftColumnWidth)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAttributeDesc::OnLeftColumnResized))
					[
						attrDesc.ToSharedRef()
					]
					+ SSplitter::Slot()
					.Value(parent->RightColumnWidth)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(parent.Get(), &FPopcornFXDetailsAttributeList::OnSetColumnWidth))
					[
						attrValuesInline.ToSharedRef()
					];

				vbox->AddSlot()
					[
						SNew(SBorder)
#if (ENGINE_MAJOR_VERSION == 5)
						.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
						.BorderBackgroundColor(this, &SAttributeDesc::GetOuterBackgroundColor)
#else
						.BorderImage(this, &SAttributeDesc::GetBorderImage)
#endif
						.Padding(FMargin(0.0f, 0.0f, kDefaultScrollBarPaddingSize, 0.0f))
						[
							inlineSplitter.ToSharedRef()
						]
					];
			}
			else
			{
				TSharedPtr<SVerticalBox>	attrValuesExpanded;
				SAssignNew(attrValuesExpanded, SVerticalBox);

				for (u32 dimi = 0; dimi < m_VectorDimension; ++dimi)
				{
					attrValuesExpanded->AddSlot()
						.FillHeight(1.0f)
						.Padding(0.0f, 2.0f)
						[
							SNew(SSplitter)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
							.Style(FAppStyle::Get(), "DetailsView.Splitter")
#else
							.Style(FEditorStyle::Get(), "DetailsView.Splitter")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
							.PhysicalSplitterHandleSize(1.0f)
							.HitDetectionSplitterHandleSize(5.0f)
								+ SSplitter::Slot()
								.Value(parent->LeftColumnWidth)
								.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAttributeDesc::OnLeftColumnResized))
								[
									SNew(SHorizontalBox)
								]
								+ SSplitter::Slot()
								.Value(parent->RightColumnWidth)
								.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(parent.Get(), &FPopcornFXDetailsAttributeList::OnSetColumnWidth))
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.FillWidth(1.f)
										.HAlign(HAlign_Fill)
										.Padding(3.0f, 0.0f)
										[
											SNew(SMyConstrainedBox)
											.MinWidth(125.f)
											.MaxWidth(125.f)
											[
												MakeAxis(dimi)
											]
										]
										+ SHorizontalBox::Slot()
										.Padding(1.f)
										.AutoWidth()
										.HAlign(HAlign_Center)
										.VAlign(VAlign_Center)
										[
											MakeResetButton(dimi)
										]
								]
						];
				}
				vbox->AddSlot()
					[
						SNew(SBorder)
#if (ENGINE_MAJOR_VERSION == 5)
						.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
						.BorderBackgroundColor(this, &SAttributeDesc::GetOuterBackgroundColor)
#else
						.BorderImage(this, &SAttributeDesc::GetBorderImage)
#endif
						.Padding(FMargin(0.0f, 0.0f, kDefaultScrollBarPaddingSize, 0.0f))
						[
							attrValuesExpanded.ToSharedRef()
						]
					];
			}

			ChildSlot
			[
				vbox.ToSharedRef()
			];
		}

	private:
		bool		Init()
		{
			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return false;

			//m_AttributesPty = parent->m_SamplersPty;
			//if (!m_AttributesPty->IsValidHandle())
			//	return false;

			//m_AttributesRawDataPty = parent->m_AttributesRawDataPty;

			const UPopcornFXAttributeList		*attrList = parent->AttrList();
			check(attrList != null);

			//m_Parent = parent;
			//m_Index = index;
			UPopcornFXEffect			*effect = ResolveEffect(attrList);
			if (effect == null)
				return false;
			// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
			const PopcornFX::CParticleAttributeDeclaration		*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, m_Index));
			if (decl == null)
				return false;
			const FPopcornFXAttributeDesc	*desc = attrList->GetAttributeDesc(m_Index);
			check(desc != null);

			const PopcornFX::EBaseTypeID	attributeBaseTypeID = (PopcornFX::EBaseTypeID)desc->AttributeBaseTypeID();

			m_Traits = &(PopcornFX::CBaseTypeTraits::Traits(attributeBaseTypeID));

			m_IsQuaternion = attributeBaseTypeID == PopcornFX::BaseType_Quaternion;

			// force vector dimension to 3 for quaternion, allow to display 3 float and use euler angles in editor
			m_VectorDimension = (!m_IsQuaternion) ? m_Traits->VectorDimension : 3;

			m_DropDownMode = desc->m_DropDownMode;
			m_EnumList = desc->m_EnumList;
			m_EnumListIndices.SetNum(m_EnumList.Num());
			for (s32 i = 0; i < m_EnumListIndices.Num(); ++i)
				m_EnumListIndices[i] = MakeShareable(new int32(i)); // SEnumComboBox::Construct

			const FString	name = desc->AttributeName();
			m_Title = FText::FromString(name);

			{
				FString			description;
				FString			shortDescription;
				description = (const UTF16CHAR*)decl->Description().MapDefault().Data();
				description = description.Replace(TEXT("\\n"), TEXT("\n"));
				int32		shortOffset;
				if (description.FindChar('\n', shortOffset))
					shortDescription = description.Left(shortOffset - 1);
				else
					shortDescription = description;
				m_Description = FText::FromString(description);
				m_ShortDescription = FText::FromString(shortDescription);
			}

			// TODO: add an icon for quaternion attributes, right now fallbacks to F3
			const FString			typeName = (!m_IsQuaternion) ? GenerateTypeName(attributeBaseTypeID) : TEXT("F3");
			m_AttributeIcon = *(TEXT("PopcornFX.Attribute.") + typeName);

			m_IsColor =
				m_Traits->IsFp && m_Traits->VectorDimension >= 3 &&
				(desc->m_AttributeSemantic == EPopcornFXAttributeSemantic::Type::AttributeSemantic_Color || name.Contains(TEXT("color")) || name.Contains(TEXT("colour")) || name.Contains(TEXT("rgb")));
			PK_ASSERT(m_IsColor || desc->m_AttributeSemantic != EPopcornFXAttributeSemantic::Type::AttributeSemantic_Color); // Something is wrong

			// force min and max for quaternion
			if (!m_IsQuaternion)
			{
				m_HasMin = decl->HasMin();
				m_HasMax = decl->HasMax();

				m_Min = decl->GetMinValue();
				m_Max = decl->GetMaxValue();
			}
			else
			{
				// for quaternion min = 0.f, max = 360.f
				PopcornFX::SAttributesContainer_SAttrib quaternionAttribMin;
				quaternionAttribMin.m_Data32f[0] = quaternionAttribMin.m_Data32f[1] = quaternionAttribMin.m_Data32f[2] = 0.f;

				PopcornFX::SAttributesContainer_SAttrib quaternionAttribMax;
				quaternionAttribMax.m_Data32f[0] = quaternionAttribMax.m_Data32f[1] = quaternionAttribMax.m_Data32f[2] = 360.f;

				m_HasMin = true;
				m_HasMax = true;

				m_Min = quaternionAttribMin;
				m_Max = quaternionAttribMax;
			}

			m_Def = decl->GetDefaultValue();

			return true;
		}

		EVisibility		GetExpanderVisibility() const
		{
			return m_Traits->VectorDimension > 1 ? EVisibility::Visible : EVisibility::Hidden;
		}

		EVisibility		GetColorPickerVisibility() const
		{
			return m_IsColor ? EVisibility::Visible : EVisibility::Hidden;
		}

		FReply			OnColorPickerClicked()
		{
			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return FReply::Unhandled();
			UPopcornFXAttributeList	*attrList = parent->AttrList();
			check(attrList != null);

			PopcornFX::SAttributesContainer_SAttrib	attribValue;
			attrList->GetAttribute(m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

			const float	r = attribValue.m_Data32f[0];
			const float	g = attribValue.m_Data32f[1];
			const float	b = attribValue.m_Data32f[2];
			const float	a = attribValue.m_Data32f[3];

			FLinearColor		initialColor(r, g, b, a);
			FColorPickerArgs	pickerArgs;
			{
				pickerArgs.bUseAlpha = m_Traits->VectorDimension == 4;
				pickerArgs.bOnlyRefreshOnMouseUp = false;
				pickerArgs.bOnlyRefreshOnOk = false;
				pickerArgs.sRGBOverride = false;//sRGBOverride;
				pickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
				pickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SAttributeDesc::OnSetColorFromColorPicker);
				pickerArgs.InitialColorOverride = initialColor;
				//pickerArgs.ParentWidget = parent;
				//pickerArgs.OptionalOwningDetailsView = parent;
			}

			OpenColorPicker(pickerArgs);
			return FReply::Handled();
		}

		void			OnSetColorFromColorPicker(FLinearColor newColor)
		{
			if (m_ReadOnly)
				return;
			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return;
			UPopcornFXAttributeList		*attrList = parent->AttrList();
			check(attrList != null);
			UPopcornFXEffect			*effect = ResolveEffect(attrList);
			if (effect == null)
				return;
			// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
			const PopcornFX::CParticleAttributeDeclaration	*decl = static_cast<const PopcornFX::CParticleAttributeDeclaration*>(attrList->GetAttributeDeclaration(effect, m_Index));
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
				if (m_Traits->VectorDimension > 3)
					attribValue.m_Data32f[3] = newColor.A;
				decl->ClampToRangeIFN(attribValue);
				attrList->SetAttribute(m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
			}
			attrList->PostEditChange();
		}

		FReply			OnArrowClicked()
		{
			PK_ASSERT(GetExpanderVisibility() == EVisibility::Visible);

			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return FReply::Handled();

			UPopcornFXAttributeList	*attrList = parent->AttrList();
			check(attrList != null);
			attrList->ToggleExpandedAttributeDetails(m_Index);

			parent->RebuildDetails();
			return FReply::Handled();
		}

		const FSlateBrush	*GetExpanderImage() const
		{
			FName	resourceName;
			if (m_ExpandOnly)
			{
				if (m_ExpanderArrow->IsHovered())
				{
					static FName ExpandedHoveredName = "TreeArrow_Expanded_Hovered";
					resourceName = ExpandedHoveredName;
				}
				else
				{
					static FName	expandedName = "TreeArrow_Expanded";
					resourceName = expandedName;
				}
			}
			else
			{
				if (m_ExpanderArrow->IsHovered())
				{
					static FName CollapsedHoveredName = "TreeArrow_Collapsed_Hovered";
					resourceName = CollapsedHoveredName;
				}
				else
				{
					static FName	collapsedName = "TreeArrow_Collapsed";
					resourceName = collapsedName;
				}
			}
			return FCoreStyle::Get().GetBrush(resourceName);
		}

#if (ENGINE_MAJOR_VERSION == 5)
	/** Get the background color of the outer part of the row, which contains the edit condition and extension widgets. */
	FSlateColor	GetOuterBackgroundColor() const
	{
		if (IsHovered())
			return FAppStyle::Get().GetSlateColor("Colors.Header");
		return FAppStyle::Get().GetSlateColor("Colors.Panel");
	}
#else
		const FSlateBrush	*GetBorderImage() const
		{
			if (IsHovered())
				return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered");
			return FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
		}
#endif

		TSharedRef<SWidget>		MakeAxis(uint32 dimi)
		{
			check(dimi < m_Traits->VectorDimension);
			if (m_Traits->ScalarType == PopcornFX::BaseType_Bool)
				return _MakeBoolAxis(dimi);
			if (m_Traits->IsFp)
				return _MakeAxis<float>(dimi);
			return _MakeAxis<int32>(dimi);
		}

		TSharedRef<SWidget>	MakeSingleSelectEnum()
		{
			TSharedRef<TSelf>							sharedThis = SharedThis(this);
			TSharedPtr<SComboBox<TSharedPtr<int32>>>	comboBox;

			// Custom SEnumComboBox. Attribute enums aren't reflected enums
			if (m_ReadOnly)
			{
				SAssignNew(comboBox, SComboBox<TSharedPtr<int32>>)
					.OptionsSource(&m_EnumListIndices)
					.OnGenerateWidget_Lambda([sharedThis](TSharedPtr<int32> Item)
						{
							return SNew(STextBlock).Text(Item.IsValid() ? FText::FromString(sharedThis->m_EnumList[*Item]) : FText::GetEmpty());
						})
					.Content()
					[
						SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
					];
			}
			else
			{
				SAssignNew(comboBox, SComboBox<TSharedPtr<int32>>)
					.OptionsSource(&m_EnumListIndices)
					.OnGenerateWidget_Lambda([sharedThis](TSharedPtr<int32> Item)
						{
							return SNew(STextBlock).Text(Item.IsValid() ? FText::FromString(sharedThis->m_EnumList[*Item]) : FText::GetEmpty());
						})
					.OnSelectionChanged(sharedThis, &TSelf::OnValueChangedEnum)
					.Content()
					[
						SNew(STextBlock)
						.Text(sharedThis, &TSelf::GetValueEnumText)
					];
			}
			return comboBox.ToSharedRef();
		}

		TSharedRef<SWidget>		_MakeBoolAxis(uint32 dimi)
		{
			// Access a shared reference to 'this'
			TSharedRef<TSelf> sharedThis = SharedThis(this);

			check(dimi < m_Traits->VectorDimension);
			TSharedPtr< SCheckBox >	axis;
			if (m_ReadOnly)
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

		template <typename _Scalar>
		TSharedRef<SWidget>		_MakeAxis(uint32 dimi)
		{
			// Access a shared reference to 'this'
			TSharedRef<TSelf> sharedThis = SharedThis(this);

			check(dimi < m_Traits->VectorDimension);

			const bool		isColor = m_IsColor;
			const bool		isQuaternion = m_IsQuaternion;

			TSharedPtr< SNumericEntryBox<_Scalar> >	axis;
			if (m_ReadOnly)
			{
				SAssignNew(axis, SNumericEntryBox<_Scalar>)
					.AllowSpin(true)

					.MinValue(m_HasMin ? TOptional<_Scalar>(m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MaxValue(m_HasMax ? TOptional<_Scalar>(m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MinSliderValue(m_HasMin ? TOptional<_Scalar>(m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MaxSliderValue(m_HasMax ? TOptional<_Scalar>(m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())

					.Value(sharedThis, &TSelf::GetValue<_Scalar>, dimi)

					.LabelVAlign(VAlign_Center)
					.LabelPadding(0)
					.Label()
					[
						SNumericEntryBox<float>::BuildLabel(s_AxisTexts[(isQuaternion) ? 2 : isColor][dimi], FLinearColor::White, s_AxisColors[dimi])
					];
			}
			else
			{
				SAssignNew(axis, SNumericEntryBox<_Scalar>)
					.AllowSpin(true)

					.MinValue(m_HasMin ? TOptional<_Scalar>(m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MaxValue(m_HasMax ? TOptional<_Scalar>(m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MinSliderValue(m_HasMin ? TOptional<_Scalar>(m_Min.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())
					.MaxSliderValue(m_HasMax ? TOptional<_Scalar>(m_Max.Get<_Scalar>()[dimi]) : TOptional<_Scalar>())

					.Value(sharedThis, &TSelf::GetValue<_Scalar>, dimi)
					.OnValueChanged(sharedThis, &TSelf::OnValueChanged<_Scalar>, dimi)
					.OnValueCommitted(sharedThis, &TSelf::OnValueCommitted<_Scalar>, dimi)

					.LabelVAlign(VAlign_Center)
					.LabelPadding(0)
					.Label()
					[
						SNumericEntryBox<float>::BuildLabel(s_AxisTexts[(isQuaternion) ? 2 : isColor][dimi], FLinearColor::White, s_AxisColors[dimi])
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
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
#else
				.ButtonStyle(FEditorStyle::Get(), "NoBorder")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.Content()
				[
					SNew(SImage)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#else
					.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				];
		}

		TSharedRef<SWidget>		MakeResetButton()
		{
			return SNew(SButton)
				.OnClicked(this, &TSelf::OnResetClicked)
				.Visibility(this, &TSelf::GetResetVisibility)
				.ToolTipText(LOCTEXT("ResetToDefaultToolTip", "Reset to Default"))
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
#else
				.ButtonStyle(FEditorStyle::Get(), "NoBorder")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.Content()
				[
					SNew(SImage)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					.Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#else
					.Image(FEditorStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				];
		}

		bool			_GetAttrib(UPopcornFXAttributeList *&outAttribList) const
		{
			TSharedPtr<TParent>		parent = m_Parent.Pin();

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

			if (!m_IsQuaternion)
				return attrList->GetAttributeDim<_Scalar>(m_Index, dimi);
			else
				return attrList->GetAttributeQuaternionDim(m_Index, dimi);
		}

		ECheckBoxState	GetValueBool(uint32 dimi) const
		{
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return ECheckBoxState::Undetermined;
			return attrList->GetAttributeDim<bool>(m_Index, dimi) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		}

		void	OnValueChangedBool(ECheckBoxState value, uint32 dimi)
		{
			if (m_ReadOnly)
				return;
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return;

			attrList->SetFlags(RF_Transactional);

			const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

			attrList->Modify();
			attrList->SetAttributeDim<bool>(m_Index, dimi, value == ECheckBoxState::Checked);
			attrList->PostEditChange();
		}

		FText	GetValueEnumText() const
		{
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return FText();
			return FText::FromString(m_EnumList[attrList->GetAttributeDim<int32>(m_Index, 0)]);
		}

		void	OnValueChangedEnum(TSharedPtr<int32> selectedItem, ESelectInfo::Type selectInfo)
		{
			if (m_ReadOnly)
				return;
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return;

			attrList->SetFlags(RF_Transactional);

			const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

			attrList->Modify();
			attrList->SetAttributeDim<int32>(m_Index, 0, *selectedItem);
			attrList->PostEditChange();
		}

		template <typename _Scalar>
		void		OnValueChanged(const _Scalar value, uint32 dimi)
		{
			if (m_ReadOnly)
				return;
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return;
			//attrList->SetFlags(RF_Transactional);
			//attrList->Modify();

			if (!m_IsQuaternion)
				attrList->SetAttributeDim<_Scalar>(m_Index, dimi, value);
			else
				attrList->SetAttributeQuaternionDim(m_Index, dimi, value);
		}

		template <typename _Scalar>
		void		OnValueCommitted(const _Scalar value, ETextCommit::Type CommitInfo, uint32 dimi)
		{
			if (m_ReadOnly)
				return;
			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return;

			attrList->SetFlags(RF_Transactional);

			const FScopedTransaction Transaction(LOCTEXT("AttributeCommit", "Attribute Value Commit"));

			//UE_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("--- DETAIL ATTRLIST commit %p %f --- %s"), attrList, float(value), *attrList->GetFullName());

			attrList->Modify();

			if (!m_IsQuaternion)
				attrList->SetAttributeDim<_Scalar>(m_Index, dimi, value);
			else
				attrList->SetAttributeQuaternionDim(m_Index, dimi, value);

			attrList->PostEditChange();
		}

		FReply				OnResetClicked()
		{
			if (m_ReadOnly)
				return FReply::Handled();

			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return FReply::Handled();

			const FScopedTransaction Transaction(LOCTEXT("AttributeReset", "Attribute Reset"));
			attrList->SetFlags(RF_Transactional);
			attrList->Modify();
			attrList->SetAttribute(m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&m_Def)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day
			attrList->PostEditChange();
			return FReply::Handled();
		}

		FReply				OnDimResetClicked(uint32 dimi)
		{
			if (m_ReadOnly)
				return FReply::Handled();

			UPopcornFXAttributeList	*attrList;
			if (!_GetAttrib(attrList))
				return FReply::Handled();

			const FScopedTransaction Transaction(LOCTEXT("AttributeResetDim", "Attribute Reset Dimension"));
			attrList->SetFlags(RF_Transactional);
			attrList->Modify();

			if (m_Traits->ScalarType == PopcornFX::BaseType_Bool)
			{
				const bool	defaultValue = reinterpret_cast<const bool*>(m_Def.Get<u32>())[dimi];
				attrList->SetAttributeDim<bool>(m_Index, dimi, defaultValue);
			}
			else
			{
				const s32	defaultValue = m_Def.Get<s32>()[dimi];
				attrList->SetAttributeDim<s32>(m_Index, dimi, defaultValue);
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
			attrList->GetAttribute(m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

			if (m_Traits->ScalarType == PopcornFX::BaseType_Bool)
			{
				for (uint32 dimi = 0; dimi < m_Traits->VectorDimension; ++dimi)
				{
					if (reinterpret_cast<bool*>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool*>(m_Def.Get<uint32>())[dimi])
						return EVisibility::Visible;
				}
			}
			else
			{
				for (uint32 dimi = 0; dimi < m_Traits->VectorDimension; ++dimi)
				{
					if (attribValue.Get<uint32>()[dimi] != m_Def.Get<uint32>()[dimi])
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
			attrList->GetAttribute(m_Index, *reinterpret_cast<FPopcornFXAttributeValue*>(&attribValue)); // Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

			if (m_Traits->ScalarType == PopcornFX::BaseType_Bool)
			{
				if (reinterpret_cast<bool*>(attribValue.Get<uint32>())[dimi] != reinterpret_cast<const bool*>(m_Def.Get<uint32>())[dimi])
					return EVisibility::Visible;
			}
			else
			{
				if (attribValue.Get<uint32>()[dimi] != m_Def.Get<uint32>()[dimi])
					return EVisibility::Visible;
			}
			return EVisibility::Hidden;
		}

	private:
		TWeakPtr<TParent>	m_Parent;

		//TSharedPtr<IPropertyHandle>		m_AttributesPty;
		//TSharedPtr<IPropertyHandle>	m_AttributesRawDataPty;

		TSharedPtr<SButton>	m_ExpanderArrow;

		bool				m_Dirtify = true;
		bool				m_ReadOnly = false;
		bool				m_ExpandOnly = false;

		uint32				m_Index;
		u32					m_VectorDimension;
		const PopcornFX::CBaseTypeTraits	*m_Traits;

		FText				m_Title;
		FText				m_Description;
		FText				m_ShortDescription;
		FName				m_AttributeIcon;

		bool									m_IsColor;
		bool									m_IsQuaternion;
		EPopcornFXAttributeDropDownMode::Type	m_DropDownMode;
		TArray<FString>							m_EnumList; // copy
		TArray<TSharedPtr<int32>>				m_EnumListIndices;
		bool									m_HasMin;
		bool									m_HasMax;

		PopcornFX::SAttributesContainer_SAttrib	m_Min;
		PopcornFX::SAttributesContainer_SAttrib	m_Max;
		PopcornFX::SAttributesContainer_SAttrib	m_Def;
	};


	class	SAttributeSamplerDesc : public SCompoundWidget
	{
	public:
		typedef SAttributeSamplerDesc				TSelf;
		typedef FPopcornFXDetailsAttributeList		TParent;

		SLATE_BEGIN_ARGS(SAttributeSamplerDesc)
		{}
			SLATE_ARGUMENT(int32, SamplerI)
			SLATE_ARGUMENT(TWeakPtr<FPopcornFXDetailsAttributeList>, Parent)
			SLATE_ARGUMENT(TOptional<bool>, Dirtify)
			SLATE_ARGUMENT(TOptional<bool>, ReadOnly)
			SLATE_ARGUMENT(TOptional<bool>, Expanded)
			SLATE_END_ARGS()
			SAttributeSamplerDesc()
		{
			ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("SAttributeSamplerDesc ctor %p"), this);
		}

		~SAttributeSamplerDesc()
		{
			ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("SAttributeSamplerDesc dotr %p"), this);
		}

		void	Construct(const FArguments& InArgs)
		{
			m_Parent = InArgs._Parent;
			m_Index = uint32(InArgs._SamplerI);

			TSharedPtr<TParent>	parent = m_Parent.Pin();
			if (!parent.IsValid())
				return;

			m_Dirtify = InArgs._Dirtify.Get(true);
			m_ReadOnly = InArgs._ReadOnly.Get(false);
			m_ExpandOnly = InArgs._Expanded.Get(false);

			if (!Init())
				return;

			uint32	childCount = 0;
			const FVector2D Icon32(32.0f, 32.0f);

			m_SamplersPty->GetNumChildren(childCount);
			if (m_Index >= childCount)
				return;

			TSharedPtr<IPropertyHandle>		samplerPty = m_SamplersPty->GetChildHandle(m_Index);
			PK_ASSERT(samplerPty.IsValid() && samplerPty->IsValidHandle());

			TSharedPtr<SVerticalBox>	vbox;
			SAssignNew(vbox, SVerticalBox);
			if (!m_ExpandOnly)
			{
				TSharedPtr<SHorizontalBox>	samplerDesc;
				SAssignNew(samplerDesc, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SAssignNew(m_ExpanderArrow, SButton)
						.ButtonStyle(FCoreStyle::Get(), "NoBorder")
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.ClickMethod(EButtonClickMethod::MouseDown)
						.OnClicked(this, &SAttributeSamplerDesc::OnArrowClicked)
						.ContentPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
						.ForegroundColor(FSlateColor::UseForeground())
						.IsFocusable(false)
						[
							SNew(SImage)
							.Image(this, &SAttributeSamplerDesc::GetExpanderImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(1.f)
					.AutoWidth()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.WidthOverride(20)
						.HeightOverride(20)
						[
							SNew(SImage)
							.Image(FPopcornFXStyle::GetBrush(m_SamplerIcon))
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(m_Title)
						.ToolTipText(m_Description)
					];

				TSharedPtr<SHorizontalBox>	samplerValueInline;
				SAssignNew(samplerValueInline, SHorizontalBox)/*
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.FillWidth(1.0f)
					.Padding(3.0f, 0.0f)
					[
						samplerActorPty->CreatePropertyValueWidget()
					]*/;

				TSharedPtr<SSplitter>	inlineSplitter;
				SAssignNew(inlineSplitter, SSplitter)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.Style(FAppStyle::Get(), "DetailsView.Splitter")
#else
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
				.PhysicalSplitterHandleSize(1.0f)
				.HitDetectionSplitterHandleSize(5.0f)
					+ SSplitter::Slot()
					.Value(parent->LeftColumnWidth)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAttributeSamplerDesc::OnLeftColumnResized))
					[
						samplerDesc.ToSharedRef()
					]
					+ SSplitter::Slot()
					.Value(parent->RightColumnWidth)
					.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(parent.Get(), &FPopcornFXDetailsAttributeList::OnSetColumnWidth))
					[
						samplerValueInline.ToSharedRef()
					];

				vbox->AddSlot()
					[
						SNew(SBorder)
#if (ENGINE_MAJOR_VERSION == 5)
						.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
						.BorderBackgroundColor(this, &SAttributeSamplerDesc::GetOuterBackgroundColor)
#else
						.BorderImage(this, &SAttributeSamplerDesc::GetBorderImage)
#endif
						.Padding(FMargin(0.0f, 0.0f, kDefaultScrollBarPaddingSize, 0.0f))
						[
							inlineSplitter.ToSharedRef()
						]
					];
			}
			else
			{
				TSharedPtr<IPropertyHandle>		samplerActorPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_AttributeSamplerActor));
				PK_ASSERT(samplerActorPty.IsValid() && samplerActorPty->IsValidHandle());

				TSharedPtr<IPropertyHandle>		samplerCpntPty = samplerPty->GetChildHandle(GET_MEMBER_NAME_STRING_CHECKED(FPopcornFXSamplerDesc, m_AttributeSamplerComponentProperty));
				PK_ASSERT(samplerCpntPty.IsValid() && samplerCpntPty->IsValidHandle());

				SAssignNew(m_ExpandedNames, SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AttrSamplerActorName", "Target Actor"))
						.ToolTipText(LOCTEXT("AttrSamplerActorTooltip", "Actor to look into for a UPopcornFXAttributeSampler"))
					]
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.Padding(0.0f, 2.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AttrSamplerSubPropertyName", "Child Component Name"))
						.ToolTipText(LOCTEXT("AttrSamplerSubPropertyTooltip", "Looks for a component with this name in the specified actor.\n- Leave this blank to use the target actor's RootComponent.\n- If no target actor is specified, looks into this emitter's RootComponent."))
					];

				SAssignNew(m_ExpandedContent, SVerticalBox)
					+ SVerticalBox::Slot()
					.Padding(0.0f, 2.0f)
					[
						samplerActorPty->CreatePropertyValueWidget()
					]
					+ SVerticalBox::Slot()
					.Padding(0.0f, 2.0f)
					[
						samplerCpntPty->CreatePropertyValueWidget()
					];

				TSharedPtr<SVerticalBox>	samplerValueExpanded;
				SAssignNew(samplerValueExpanded, SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SSplitter)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
						.Style(FAppStyle::Get(), "DetailsView.Splitter")
#else
						.Style(FEditorStyle::Get(), "DetailsView.Splitter")
#endif (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
						.PhysicalSplitterHandleSize(1.0f)
						.HitDetectionSplitterHandleSize(5.0f)
							+ SSplitter::Slot()
							.Value(parent->LeftColumnWidth)
							.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SAttributeSamplerDesc::OnLeftColumnResized))
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.Padding(30.0f, 0.0f)
									.HAlign(HAlign_Fill)
									.VAlign(VAlign_Center)
									.FillWidth(1.0f)
									.AutoWidth()
									[
										m_ExpandedNames.ToSharedRef()
									]
							]
							+ SSplitter::Slot()
							.Value(parent->RightColumnWidth)
							.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(parent.Get(), &FPopcornFXDetailsAttributeList::OnSetColumnWidth))
							[
								SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.HAlign(HAlign_Fill)
									.Padding(3.0f, 0.0f)
									[
										m_ExpandedContent.ToSharedRef()
									]
							]
					];

				vbox->AddSlot()
					[
						SNew(SBorder)
#if (ENGINE_MAJOR_VERSION == 5)
						.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
						.BorderBackgroundColor(this, &SAttributeSamplerDesc::GetOuterBackgroundColor)
#else
						.BorderImage(this, &SAttributeSamplerDesc::GetBorderImage)
#endif
						.Padding(FMargin(0.0f, 0.0f, kDefaultScrollBarPaddingSize, 0.0f))
						[
							samplerValueExpanded.ToSharedRef()
						]
					];
			}

			ChildSlot
				[
					vbox.ToSharedRef()
				];
		}

	private:
		bool		Init()
		{
			TSharedPtr<TParent>					parent = m_Parent.Pin();
			if (!parent.IsValid())
				return false;

			m_SamplersPty = parent->m_SamplersPty;
			if (!m_SamplersPty->IsValidHandle())
				return false;

			const UPopcornFXAttributeList		*attrList = parent->AttrList();
			check(attrList != null);

			const FPopcornFXSamplerDesc			*desc = attrList->GetSamplerDesc(m_Index);
			if (desc == null)
				return false;

			m_SamplerType = desc->m_SamplerType;

			const FString	name = desc->SamplerName();
			m_Title = FText::FromString(name);

			FString			defNode;

			UPopcornFXEffect			*effect = ResolveEffect(attrList);
			if (effect == null)
				return false;

			// Ugly cast, so PopcornFXAttributeList.h is a public header to satisfy UE4 nativization bugs. To refactor some day

			// TODO(Attributes refactor): This should not use 'CParticleNodeSamplerData'
			const PopcornFX::CParticleAttributeSamplerDeclaration	*particleSampler = static_cast<const PopcornFX::CParticleAttributeSamplerDeclaration*>(attrList->GetParticleSampler(effect, m_Index));
			if (particleSampler == null)
				return true;
			m_HasDefaultDescriptor = particleSampler->GetSamplerDefaultDescriptor() != null;
			const char			*nodeName = ResolveAttribSamplerNodeName(particleSampler, m_SamplerType);
			if (nodeName != null)
			{
				defNode = nodeName;
				m_SamplerIcon = FName(*("PopcornFX.Node." + defNode));
			}
			else
			{
				defNode = "?????";
				m_SamplerIcon = FName(TEXT("PopcornFX.BadIcon32"));
			}

			m_Description = FText::FromString(name + " (default: " + defNode + ")");
			m_ShortDescription = FText::FromString(defNode);
			return true;
		}

		void	OnLeftColumnResized(float InNewWidth)
		{
			// SDetailSingleItemRow.cpp:
			// This has to be bound or the splitter will take it upon itself to determine the size
			// We do nothing here because it is handled by the column size data
		}

		FReply			OnArrowClicked()
		{
			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return FReply::Handled();

			UPopcornFXAttributeList	*attrList = parent->AttrList();
			check(attrList != null);
			attrList->ToggleExpandedSamplerDetails(m_Index);

			parent->RebuildDetails();
			return FReply::Handled();
		}

		const FSlateBrush	*GetExpanderImage() const
		{
			FName	resourceName;
			if (m_ExpandOnly)
			{
				if (m_ExpanderArrow->IsHovered())
				{
					static FName ExpandedHoveredName = "TreeArrow_Expanded_Hovered";
					resourceName = ExpandedHoveredName;
				}
				else
				{
					static FName	expandedName = "TreeArrow_Expanded";
					resourceName = expandedName;
				}
			}
			else
			{
				if (m_ExpanderArrow->IsHovered())
				{
					static FName CollapsedHoveredName = "TreeArrow_Collapsed_Hovered";
					resourceName = CollapsedHoveredName;
				}
				else
				{
					static FName	collapsedName = "TreeArrow_Collapsed";
					resourceName = collapsedName;
				}
			}
			return FCoreStyle::Get().GetBrush(resourceName);
		}


#if (ENGINE_MAJOR_VERSION == 5)
		/** Get the background color of the outer part of the row, which contains the edit condition and extension widgets. */
		FSlateColor	GetOuterBackgroundColor() const
		{
			if (IsHovered())
				return FAppStyle::Get().GetSlateColor("Colors.Header");
			return FAppStyle::Get().GetSlateColor("Colors.Panel");
		}
#else
		const FSlateBrush	*GetBorderImage() const
		{
			if (IsHovered())
				return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered");
			return FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
		}
#endif

	private:
		TWeakPtr<TParent>	m_Parent;

		//TSharedPtr<IPropertyHandle>	m_AttributesPty;
		TSharedPtr<IPropertyHandle>		m_SamplersPty;

		TSharedPtr<SButton>				m_ExpanderArrow;
		TSharedPtr<SVerticalBox>		m_ExpandedContent;
		TSharedPtr<SVerticalBox>		m_ExpandedNames;

		bool				m_Dirtify = true;
		bool				m_ReadOnly = false;
		bool				m_ExpandOnly = false;
		bool				m_HasDefaultDescriptor = false;

		uint32				m_Index;

		EPopcornFXAttributeSamplerType::Type	m_SamplerType;

		FText				m_Title;
		FText				m_Description;
		FText				m_ShortDescription;
		FName				m_SamplerIcon;
	};

	class	SPopcornFXAttributeCategory : public SCompoundWidget
	{
	public:
		typedef SPopcornFXAttributeCategory			TSelf;
		typedef FPopcornFXDetailsAttributeList		TParent;

		SLATE_BEGIN_ARGS(SPopcornFXAttributeCategory)
		{}
			SLATE_ARGUMENT(uint32, CategoryI)
			SLATE_ARGUMENT(FText, CategoryName)
			SLATE_ARGUMENT(TWeakPtr<FPopcornFXDetailsAttributeList>, Parent)
			SLATE_END_ARGS()

		SPopcornFXAttributeCategory() { }
		~SPopcornFXAttributeCategory() { }

		void	Construct(const FArguments& InArgs)
		{
			m_CategoryName = InArgs._CategoryName;
			m_Parent = InArgs._Parent;
			m_Index = uint32(InArgs._CategoryI);

			const float		kBorderVerticalPadding = 3.0f;

			SAssignNew(m_Attributes, SVerticalBox);

			SAssignNew(m_VBox, SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(m_Border, SBorder)
				.BorderImage(this, &SPopcornFXAttributeCategory::GetBorderImage)
				.Padding(FMargin( 0.0f, kBorderVerticalPadding, kDefaultScrollBarPaddingSize, kBorderVerticalPadding))
				.BorderBackgroundColor(FLinearColor(0.6f, 0.6f, 0.6f, 1.0f))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					[
						SAssignNew(m_ExpanderArrow, SButton)
						.ButtonStyle(FCoreStyle::Get(), "NoBorder")
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.ClickMethod(EButtonClickMethod::MouseDown)
						.OnClicked(this, &SPopcornFXAttributeCategory::OnArrowClicked)
						.ContentPadding(0.f)
						.ForegroundColor(FSlateColor::UseForeground())
						.IsFocusable(false)
						[
							SNew(SImage)
							.Image(this, &SPopcornFXAttributeCategory::GetExpanderImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(m_CategoryName)
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
						.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
#else
						.Font(FEditorStyle::GetFontStyle("PropertyWindow.NormalFont"))
#endif // (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				m_Attributes.ToSharedRef()
			];

			ChildSlot
			[
				m_VBox.ToSharedRef()
			];
		}

		void	AddAttribute(const FPopcornFXAttributeDesc *desc, uint32 attri)
		{
			PK_ASSERT(desc != null);

			m_Attributes->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SAttributeDesc)
				.Parent(m_Parent)
				.AttribI(attri)
				.Expanded(false)
			];

			if (desc->m_IsExpanded)
			{
				m_Attributes->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SAttributeDesc)
					.Parent(m_Parent)
					.AttribI(attri)
					.Expanded(true)
				];
			}
		}

		void	AddSampler(const FPopcornFXSamplerDesc *desc, uint32 sampleri)
		{
			PK_ASSERT(desc != null);

			TSharedPtr<SAttributeSamplerDesc>	sdesc;
			m_Attributes->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SAttributeSamplerDesc)
				.Parent(m_Parent)
				.SamplerI(sampleri)
			];

			if (desc->m_IsExpanded)
			{
				m_Attributes->AddSlot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SAttributeSamplerDesc)
					.Parent(m_Parent)
					.SamplerI(sampleri)
					.Expanded(true)
				];
			}
		}

		void	ClearAttributes()
		{
			PK_ASSERT(m_Attributes.IsValid());
			m_Attributes->ClearChildren();
		}

		uint32	NumAttributes()
		{
			PK_ASSERT(m_Attributes.IsValid());
			return m_Attributes->NumSlots();
		}

	private:
		const FSlateBrush	*GetExpanderImage() const
		{
			FName	resourceName;
			if (m_ExpanderArrow->IsHovered())
			{
				static FName	collapsedHoveredName = "TreeArrow_Collapsed_Hovered";
				resourceName = collapsedHoveredName;
			}
			else
			{
				static FName	collapsedName = "TreeArrow_Collapsed";
				resourceName = collapsedName;
			}
			return FCoreStyle::Get().GetBrush(resourceName);
		}

		const FSlateBrush	*GetBorderImage() const
		{
#if (ENGINE_MAJOR_VERSION == 5)
			if (m_Border->IsHovered())
				return FAppStyle::Get().GetBrush("DetailsView.CategoryTop_Hovered");
			return FAppStyle::Get().GetBrush("DetailsView.CategoryTop");
#else
			if (m_Border->IsHovered())
				return FEditorStyle::GetBrush("DetailsView.CategoryTop_Hovered");
			return FEditorStyle::GetBrush("DetailsView.CategoryTop");
#endif // (ENGINE_MAJOR_VERSION == 5)
		}

		FReply			OnArrowClicked()
		{
			TSharedPtr<TParent>		parent = m_Parent.Pin();
			if (!parent.IsValid())
				return FReply::Handled();

			UPopcornFXAttributeList	*attrList = parent->AttrList();
			check(attrList != null);
			attrList->ToggleExpandedCategoryDetails(m_Index);

			parent->RebuildDetails();
			return FReply::Handled();
		}

	private:
		TWeakPtr<TParent>	m_Parent;
		FText				m_CategoryName;
		uint32				m_Index;

		TSharedPtr<SBorder>			m_Border;
		TSharedPtr<SVerticalBox>	m_Attributes;
		TSharedPtr<SVerticalBox>	m_VBox;
		TSharedPtr<SButton>			m_ExpanderArrow;
	};
}

//static
TSharedRef<IDetailCustomization>		FPopcornFXDetailsAttributeList::MakeInstance()
{
	return MakeShareable(new FPopcornFXDetailsAttributeList);
}

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
	else
	{
		PK_ASSERT_NOT_REACHED();
	}
	return attrList;
}

void	FPopcornFXDetailsAttributeList::RebuildAndRefresh()
{
	m_RefreshQueued = false;

	LeftColumnWidth = TAttribute<float>(this, &FPopcornFXDetailsAttributeList::OnGetLeftColumnWidth);
	RightColumnWidth = TAttribute<float>(this, &FPopcornFXDetailsAttributeList::OnGetRightColumnWidth);

	RebuildAttributes();
	RebuildSamplers();

	if (PK_VERIFY(m_PropertyUtilities.IsValid()))
		m_PropertyUtilities->RequestRefresh();
}

UPopcornFXAttributeList		*FPopcornFXDetailsAttributeList::AttrList()
{
	UPopcornFXAttributeList		*attrList = UnsafeAttrList();
	if (attrList == null)
	{
		if (!m_RefreshQueued) // Avoid queueing tons of events in the same frame
		{
			bool	rebuild = false;
			const u32	categoryCount = m_SCategories.Num();
			for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
				rebuild |= m_SCategories[iCategory]->NumAttributes() > 0;
			rebuild &= PK_VERIFY(m_PropertyUtilities.IsValid());

			// We need to clear the remaining attributes
			if (rebuild)
			{
				m_RefreshQueued = true;
				m_PropertyUtilities->EnqueueDeferredAction(FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeList::RebuildAndRefresh));
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
	m_RefreshQueued = false;
	const UPopcornFXAttributeList		*attrList = UnsafeAttrList(); // AttrList() will ask for rebuild ifn, we dont want that here
	if (!PK_VERIFY(attrList != null))
		return;
	if (m_FileVersionId != attrList->FileVersionId() || // Effect was reimported
		m_Effect != attrList->Effect()) // Effect changed
	{
		// Full rebuild
		m_FileVersionId = attrList->FileVersionId();
		m_Effect = attrList->Effect();
		RebuildDetails();
	}
}

void	FPopcornFXDetailsAttributeList::Rebuild()
{
	const UPopcornFXAttributeList		*attrList = UnsafeAttrList(); // AttrList() will ask for rebuild ifn, we dont want that here
	if (attrList == null)
		return;
	m_ColumnWidth = attrList->GetColumnWidth();
	m_FileVersionId = attrList->FileVersionId();
	m_Effect = attrList->Effect();
	RebuildAndRefresh();
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
		if (m_SCategories.Num() > 0)
		{
			// Wrong state, rebuild everything
			m_SCategories.Empty();
			RebuildDetails();
		}
		return;
	}

	PK_ASSERT((int32)defAttrList->GetCategoryCount() == m_SCategories.Num());

	// Browse back name list
	const u32	categoryCount = defAttrList->GetCategoryCount();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		PK_ASSERT(m_SCategories[iCategory].IsValid());
		m_SCategories[iCategory]->ClearAttributes(); // Will also clear samplers
		if (!attrList->IsCategoryExpanded(iCategory)) // Expanded state is recovered from the attr list, not the default
			continue;
		const uint32	attrCount = attrList->AttributeCount();
		for (u32 attri = 0; attri < attrCount; ++attri)
		{
			const FPopcornFXAttributeDesc	*desc = attrList->GetAttributeDesc(attri);
			check(desc != null);

			if (desc->m_AttributeCategoryName != defAttrList->GetCategoryName(iCategory))
			{
				if ((desc->m_AttributeCategoryName.IsValid() && !desc->m_AttributeCategoryName.IsNone()) || iCategory > 0)
					continue;
			}
			m_SCategories[iCategory]->AddAttribute(desc, attri);
		}
	}
}

void	FPopcornFXDetailsAttributeList::RebuildSamplers()
{
	ATTRDEBUB_LOG(LogPopcornFXDetailsAttributeList, Log, TEXT("rebuild samplers"));

	const UPopcornFXAttributeList	*attrList = AttrList();
	if (attrList == null)
		return;
	// Categories are only contained in the effect's DefaultAttributeList
	const UPopcornFXAttributeList	*defAttrList = attrList->GetDefaultAttributeList(ResolveEffect(attrList));
	if (defAttrList == null)
		return;

	PK_ASSERT((int32)defAttrList->GetCategoryCount() == m_SCategories.Num());

	const u32	categoryCount = defAttrList->GetCategoryCount();
	for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
	{
		const uint32	samplerCount = attrList->SamplerCount();
		if (!attrList->IsCategoryExpanded(iCategory)) // Expanded state is recovered from the attr list, not the default
			continue;
		for (u32 sampleri = 0; sampleri < samplerCount; ++sampleri)
		{
			const FPopcornFXSamplerDesc		*desc = attrList->GetSamplerDesc(sampleri);
			check(desc != null);
			if (desc->m_SamplerType == EPopcornFXAttributeSamplerType::None)
				continue;
			if (desc->m_AttributeCategoryName != defAttrList->GetCategoryName(iCategory))
			{
				if ((desc->m_AttributeCategoryName.IsValid() && !desc->m_AttributeCategoryName.IsNone()) || iCategory > 0)
					continue;
			}
			m_SCategories[iCategory]->AddSampler(desc, sampleri);
		}
	}
}

void	FPopcornFXDetailsAttributeList::RebuildDetails()
{
	IDetailLayoutBuilder	*detailLayoutBuilder = m_DetailLayoutBuilder.Pin().Get();
	if (!PK_VERIFY(detailLayoutBuilder != null))
		return;
	UPopcornFXAttributeList		*attrList = UnsafeAttrList();
	if (attrList != null)
		attrList->SetColumnWidth(m_ColumnWidth); // Store column width if valid attribute list

	detailLayoutBuilder->ForceRefreshDetails();
}

EVisibility		FPopcornFXDetailsAttributeList::AttribVisibilityAndRefresh()
{
	const UPopcornFXAttributeList		*attrList = AttrList(); // will enqueue a refresh IFN
	(void)attrList;
	return EVisibility::Visible;
}

void	FPopcornFXDetailsAttributeList::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder> &detailBuilder)
{
	m_DetailLayoutBuilder = detailBuilder;
	CustomizeDetails(*detailBuilder);
}

void	FPopcornFXDetailsAttributeList::CustomizeDetails(IDetailLayoutBuilder &detailBuilder)
{
	//m_DetailBuilder = &detailLayout;
	m_PropertyUtilities = detailBuilder.GetPropertyUtilities();
	detailBuilder.GetObjectsBeingCustomized(m_BeingCustomized);

	IDetailCategoryBuilder			&attrListCategory = detailBuilder.EditCategory("PopcornFX Attributes");

	detailBuilder.HideProperty("m_FileVersionId");
	detailBuilder.HideProperty("m_Attributes");
	detailBuilder.HideProperty("m_Samplers");
	detailBuilder.HideProperty("m_AttributesRawData");

	m_FileVersionIdPty = detailBuilder.GetProperty("m_FileVersionId");
	if (!PK_VERIFY(IsValidHandle(m_FileVersionIdPty)))
		return;

	m_SamplersPty = detailBuilder.GetProperty("m_Samplers");
	if (!PK_VERIFY(IsValidHandle(m_SamplersPty)))
		return;

	const UPopcornFXAttributeList	*attrList = UnsafeAttrList();

	// Actually we do, otherwise the details panel will not refresh if we pick up a new emitter
#if 0
	// AttrList() will ask for rebuild ifn, we dont want that here
	if (attrList == null)
		return;
#endif

	// Categories are only contained in the effect's DefaultAttributeList
	const UPopcornFXAttributeList	*defAttrList = null;
	if (attrList != null)
	{
		defAttrList = attrList->GetDefaultAttributeList(ResolveEffect(attrList));
		if (defAttrList != null)
			m_SCategories.SetNum(defAttrList->GetCategoryCount());
	}

	// Hacky callback to trigger refresh.
	// Added to fix refresh when effect without attributes was re-imported with attribute.
	TAttribute<EVisibility>		attribVisibilityAndRefresh =
		TAttribute<EVisibility>::Create(
			TAttribute<EVisibility>::FGetter::CreateSP(this, &FPopcornFXDetailsAttributeList::AttribVisibilityAndRefresh));

	TSharedPtr<SVerticalBox>	mainWidget;
	{
		SAssignNew(mainWidget, SVerticalBox);

		const u32	categoryCount = m_SCategories.Num();
		for (u32 iCategory = 0; iCategory < categoryCount; ++iCategory)
		{
			PK_ASSERT(defAttrList != null);

			mainWidget->AddSlot()
			.AutoHeight()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			.Padding(0.0f)
			[
				SAssignNew(m_SCategories[iCategory], SPopcornFXAttributeCategory)
				.CategoryName(FText::FromName(defAttrList->GetCategoryName(iCategory)))
				.Parent(SharedThis(this))
				.CategoryI(iCategory)
			];
		}
	}

	attrListCategory.AddCustomRow(LOCTEXT("Attributes", "Attributes"), false)
		.Visibility(attribVisibilityAndRefresh)
		.WholeRowContent()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			mainWidget.ToSharedRef()
		];

	Rebuild();

	{
		FSimpleDelegate		refreshAttrs = FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeList::RebuildAndRefresh);
		m_SamplersPty->AsArray()->SetOnNumElementsChanged(refreshAttrs);
	}
	{
		// It does not seem to work (this is why AttribVisibilityAndRefresh exists)
		FSimpleDelegate		refreshAttrs = FSimpleDelegate::CreateSP(this, &FPopcornFXDetailsAttributeList::Rebuild);
		m_FileVersionIdPty->SetOnPropertyValueChanged(refreshAttrs);
	}
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE

//----------------------------------------------------------------------------
#endif // WITH_EDITOR

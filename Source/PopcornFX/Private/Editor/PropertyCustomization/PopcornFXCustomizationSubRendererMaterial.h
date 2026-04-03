//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "PropertyEditorModule.h"
#include "AssetThumbnail.h"

class FPopcornFXCustomizationSubRendererMaterial : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// overrides IPropertyTypeCustomization
	virtual void	CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void	CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	struct FPopcornFXSubRendererMaterial		*Self() const;

	static FString	_RendererNameFromType(const int type);
	FText			OnGetThumbnailToolTip(int32 thumbId) const;
	void			OnThumbnailPtyChange(int32 thumbId);
	EVisibility		OnGetThumbnailVisibility(int32 thumbId) const;

	FReply			OnResetClicked();
	EVisibility		GetResetVisibility() const;

	bool			OnFilterMaterialPicker(const FAssetData& InAssetData) const;

	TSharedPtr<IPropertyHandle>		m_SelfPty;
	TSharedPtr<IPropertyHandle>		m_MaterialPty;

	enum
	{
		Thumb_Diffuse = 0,
		Thumb_DiffuseRamp,
		Thumb_Emissive,
		Thumb_AlphaRemap,
		Thumb_MotionVectors,
		Thumb_SixWay_RLTS,
		Thumb_SixWay_BBF,
		Thumb_VATPosition,
		Thumb_VATNormal,
		Thumb_VATColor,
		Thumb_VATRotation,
		Thumb_SkeletalAnimation,
		Thumb_Normal,
		Thumb_Specular,
		Thumb_Mesh,
		Thumb_SkeletalMesh,
		__MaxThumb,
	};

	struct FThumbs
	{
		TSharedPtr<IPropertyHandle>		m_Pty;
		TSharedPtr<FAssetThumbnail>		m_Tumbnail;
	};
	FThumbs								m_Thumbs[__MaxThumb];

};

#endif // WITH_EDITOR

//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if WITH_EDITOR

#include "PopcornFXMinimal.h"

#include "AssetTypeActions_Base.h"

#include "UObject/WeakObjectPtrTemplates.h"
#include "Assets/PopcornFXFile.h" // needed for TWeakObjectPtr

class FPopcornFXFileTypeActions : public FAssetTypeActions_Base
{
public:
	FPopcornFXFileTypeActions(EAssetTypeCategories::Type category);

	UClass					*m_SupportedClass;
	FText					m_Name;
	FColor					m_Color;

	virtual FText			GetName() const override { return m_Name; }
	virtual FColor			GetTypeColor() const override { return m_Color; }
	virtual UClass			*GetSupportedClass() const override  { return m_SupportedClass; }
	virtual bool			HasActions(const TArray<UObject*>& inObjects) const override { return true; }
	virtual void			GetActions(const TArray<UObject*>& inObjects, FMenuBuilder& menuBuilder) override;
	virtual uint32			GetCategories() override { return m_Category; }

	virtual void			OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

public:
	/** Handler for when Edit is selected */
	static void	ExecuteEdit(TArray<TWeakObjectPtr<UPopcornFXFile>> objects);

	/** Handler for when Reimport is selected */
	static void	ExecuteReimport(TArray<TWeakObjectPtr<UPopcornFXFile>> objects);

private:
	/** Handler for when Reimport and reset is selected */
	static void	ExecuteReimportReset(TArray<TWeakObjectPtr<UPopcornFXFile>> objects);

	/** Handler for when FindInExplorer is selected */
	static void	ExecuteFindInExplorer(TArray<TWeakObjectPtr<UPopcornFXFile>> objects);

	/** Returns true to allow execution of source file commands */
	bool	CanExecuteSourceCommands() const;

private:
	EAssetTypeCategories::Type			m_Category;
	bool								m_CanExecuteFindInExplorer;
};

#endif // WITH_EDITOR

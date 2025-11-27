//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDependencyModulePropertyEditor.h"

#include "Editor/CustomizeDetails/PopcornFXDetailsSceneComponent.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsEmitterComponent.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeList.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerShape.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerCurve.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerImage.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerGrid.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerVectorField.h"
#include "Editor/CustomizeDetails/PopcornFXDetailsAttributeSamplerActor.h"

#include "Editor/PropertyCustomization/PopcornFXCustomizationAssetDep.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationRendererMaterial.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationSubRendererMaterial.h"

#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerImage.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerShape.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerCurve.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerGrid.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerText.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerAnimTrack.h"
#include "Editor/PropertyCustomization/PopcornFXCustomizationAttributeSamplerVectorField.h"


#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

FPopcornFXDependencyModulePropertyEditor::FPopcornFXDependencyModulePropertyEditor()
{

}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModulePropertyEditor::Load()
{
	if (m_Loaded)
		return;

	if (!PK_VERIFY(FModuleManager::Get().IsModuleLoaded("PropertyEditor")))
		return;
	FPropertyEditorModule	&propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Register the custom editor
	propertyModule.RegisterCustomClassLayout("PopcornFXEmitterComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPopcornFXDetailsEmitterComponent::MakeInstance));
	propertyModule.RegisterCustomClassLayout("PopcornFXSceneComponent", FOnGetDetailCustomizationInstance::CreateStatic(&FPopcornFXDetailsSceneComponent::MakeInstance));
	propertyModule.RegisterCustomClassLayout("PopcornFXAttributeList", FOnGetDetailCustomizationInstance::CreateStatic(&FPopcornFXDetailsAttributeListHidden::MakeInstance));

	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAssetDep", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAssetDep::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXRendererMaterial", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationRendererMaterial::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXSubRendererMaterial", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationSubRendererMaterial::MakeInstance));

	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerProperties", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSampler::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesImage", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerImage::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesShape", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerShape::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesGrid", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerGrid::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesCurve", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerCurve::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesText", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerText::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesVectorField", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerVectorField::MakeInstance));
	propertyModule.RegisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesAnimTrack", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPopcornFXCustomizationAttributeSamplerAnimTrack::MakeInstance));

	m_Loaded = true;
}

//----------------------------------------------------------------------------

void	FPopcornFXDependencyModulePropertyEditor::Unload()
{
	if (!m_Loaded)
		return;
	m_Loaded = false;

	if (!/*PK_VERIFY*/(FModuleManager::Get().IsModuleLoaded("PropertyEditor"))) // @FIXME: we should be called when the plugin is still loaded
		return;

	FPropertyEditorModule&	propertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// Unregister the custom editor
	propertyModule.UnregisterCustomClassLayout("PopcornFXEmitterComponent");
	propertyModule.UnregisterCustomClassLayout("PopcornFXSceneComponent");
	propertyModule.UnregisterCustomClassLayout("PopcornFXAttributeList");

	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAssetDep");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXRendererMaterial");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXSubRendererMaterial");

	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerProperties");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesImage");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesShape");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesGrid");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesCurve");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesText");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesVectorField");
	propertyModule.UnregisterCustomPropertyTypeLayout("PopcornFXAttributeSamplerPropertiesAnimTrack");
}

//----------------------------------------------------------------------------

#endif // WITH_EDITOR

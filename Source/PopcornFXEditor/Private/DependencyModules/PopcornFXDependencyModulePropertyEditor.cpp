//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "PopcornFXDependencyModulePropertyEditor.h"

#include "CustomizeDetails/PopcornFXDetailsSceneComponent.h"
#include "CustomizeDetails/PopcornFXDetailsEmitterComponent.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeList.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerShape.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerCurve.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerImage.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerGrid.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerVectorField.h"
#include "CustomizeDetails/PopcornFXDetailsAttributeSamplerActor.h"

#include "PropertyCustomization/PopcornFXCustomizationAssetDep.h"
#include "PropertyCustomization/PopcornFXCustomizationRendererMaterial.h"
#include "PropertyCustomization/PopcornFXCustomizationSubRendererMaterial.h"

#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerImage.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerShape.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerCurve.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerGrid.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerText.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerAnimTrack.h"
#include "PropertyCustomization/PopcornFXCustomizationAttributeSamplerVectorField.h"


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

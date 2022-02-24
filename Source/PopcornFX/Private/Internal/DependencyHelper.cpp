//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#if WITH_EDITOR
#include "DependencyHelper.h"

#include "PopcornFXSDK.h"
#include <pk_base_object/include/hbo_helpers.h>
#include <pk_kernel/include/kr_file.h>

static bool		_CheckFieldIsVisible(const PopcornFX::CBaseObject *hbo, const PopcornFX::HBO::CFieldDefinition &fieldDef)
{
	// don't fetch hidden fields
	if (fieldDef.VisibleInEditor(hbo) == PopcornFX::CBaseObject::FieldVisible)
		return true;
	//PopcornFX::CLog::Log(PK_INFO, "Ignored hidden field %s :: %s", hbo->RawName().Data(), fieldDef.m_Name);
	return false;
}

void	PopcornFX::GatherDependencies(const PopcornFX::CString &path, const PopcornFX::CbDependency &cb)
{
	PopcornFX::PBaseObjectFile	srcFile = PopcornFX::HBO::g_Context->FindFile(path);
	const bool		ownsFile = (srcFile == null);
	if (ownsFile)
		srcFile = PopcornFX::HBO::g_Context->LoadFile(path, true);

	if (srcFile == null)
		return;	// FAIL !

	const TMemoryView<const PopcornFX::PBaseObject>		objectList = srcFile->ObjectList();
	for (u32 i = 0; i < objectList.Count(); i++)
	{
		const PopcornFX::CBaseObject	*hbo = objectList[i].Get();
		PK_ASSERT(hbo != null);
		if (hbo == null)
			continue;
		u32	fieldCount = hbo->FieldCount();
		for (u32 field = 0; field < fieldCount; field++)
		{
			const PopcornFX::HBO::CFieldDefinition		&fieldDef = hbo->GetFieldDefinition(field);
			const PopcornFX::HBO::CFieldAttributesBase	&fieldAttribs = fieldDef.GetBaseAttributes();
			const PopcornFX::HBO::SGenericType			&fieldType = fieldDef.Type();

			// two possible cases: either the field is a text string tagged as a path, or the field is a link to another HBO file.
			if (fieldType.SubType() == PopcornFX::HBO::GenericType_String)	// text string
			{
				if (PopcornFX::HBO::CaracsIsPath(fieldAttribs.m_Caracs))	// tagged as a path
				{
					// don't fetch hidden fields
					if (!_CheckFieldIsVisible(hbo, fieldDef))
						continue;
					if (fieldType.IsArray())	// array of paths
					{
						const PopcornFX::TArray<CString, PopcornFX::HBO::TFieldArrayController>	&strArray = hbo->GetField<PopcornFX::TArray<CString> >(field);
						for (u32 j = 0; j < strArray.Count(); j++)
						{
							if (!strArray[j].Empty())
								cb(strArray[j], *hbo, field);
						}
					}
					else
					{
						const CString	&str = hbo->GetField<CString>(field);
						if (!str.Empty())
							cb(str, *hbo, field);
					}
				}
			}
			else if (fieldType.SubType() == PopcornFX::HBO::GenericType_Link)	// link
			{
				// don't fetch hidden fields
				if (!_CheckFieldIsVisible(hbo, fieldDef))
					continue;
				//	 MAKE SURE SELF FILE (NO EXTERNAL PATHS)
				const CString	&filePath = hbo->File()->Path();
				if (fieldType.IsArray())	// array
				{
					const PopcornFX::TArray<PopcornFX::HBO::CLink, PopcornFX::HBO::TFieldArrayController>	&linkArray = hbo->GetField<PopcornFX::TArray<PopcornFX::HBO::CLink> >(field);
					for (u32 j = 0; j < linkArray.Count(); j++)
					{
						const CBaseObject	*obj = linkArray[j].DataFeed().Get();
						if (obj != null && obj->File() == hbo->File())
						{
							if (!filePath.Empty())
								cb(filePath, *hbo, field);
						}
					}
				}
				else
				{
					const PopcornFX::HBO::CLink		&link = hbo->GetField<PopcornFX::HBO::CLink>(field);
					const CBaseObject				*obj = link.DataFeed().Get();
					if (obj != null && obj->File() == hbo->File())
					{
						if (!filePath.Empty())
							cb(filePath, *hbo, field);
					}
				}
			}
		}
	}

	if (ownsFile)
		srcFile->Unload();
	return;
}

#endif // WITH_EDITOR

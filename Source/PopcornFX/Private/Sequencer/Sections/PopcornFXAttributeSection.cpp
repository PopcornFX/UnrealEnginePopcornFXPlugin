//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------


#if WITH_EDITOR

#include "Sequencer/Sections/PopcornFXAttributeSection.h"

#include "ScopedTransaction.h"
#include "Sections/MovieSceneParameterSection.h"

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSection"

// Hugo: NOTE -- This is ~exact copy of ParameterSection.cpp/h which is private right now
// Ideally, thirdparty could rely on the default implementation

bool	FPopcornFXAttributeSection::RequestDeleteCategory( const TArray<FName>& CategoryNamePath )
{
	const FScopedTransaction Transaction( LOCTEXT( "DeleteVectorOrColorParameter", "Delete vector or color parameter" ) );
	UMovieSceneParameterSection* ParameterSection = Cast<UMovieSceneParameterSection>( GetSectionObject() );
	if( ParameterSection->Modify() )
	{
		bool bVectorParameterDeleted = ParameterSection->RemoveVectorParameter( CategoryNamePath[0] );
		bool bColorParameterDeleted = ParameterSection->RemoveColorParameter( CategoryNamePath[0] );
		return bVectorParameterDeleted || bColorParameterDeleted;
	}
	return false;
}

bool	FPopcornFXAttributeSection::RequestDeleteKeyArea( const TArray<FName>& KeyAreaNamePath )
{
	// Only handle paths with a single name, in all other cases the user is deleting a component of a vector parameter.
	if ( KeyAreaNamePath.Num() == 1)
	{
		const FScopedTransaction Transaction( LOCTEXT( "DeleteScalarParameter", "Delete scalar parameter" ) );
		UMovieSceneParameterSection* ParameterSection = Cast<UMovieSceneParameterSection>( GetSectionObject() );
		if (ParameterSection->TryModify())
		{
			return ParameterSection->RemoveScalarParameter( KeyAreaNamePath[0] );
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE

#endif //WITH_EDITOR

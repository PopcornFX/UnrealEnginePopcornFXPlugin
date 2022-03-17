//--------------------------------------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved. https://www.popcornfx.com/terms-and-conditions/
//--------------------------------------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#define	PKUE_COMMON_SHADER_PATH(__name)		"/Plugin/PopcornFX/Private/" TEXT(__name)
#define	PKUE_GLOBAL_SHADER_PATH(__name)		PKUE_COMMON_SHADER_PATH(__name) TEXT(".usf")
#define	PKUE_SHADER_PATH(__name)			PKUE_COMMON_SHADER_PATH(__name) TEXT(".ush")

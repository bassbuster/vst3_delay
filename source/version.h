#pragma once

#include "projectversion.h"

#define stringOriginalFilename	"megalay.vst3"

#ifdef DEVELOPMENT
#define stringPluginName "Megalay dev"
#else
#define stringPluginName "Megalay"
#endif

#if defined(_WIN64) || defined(__x86_64__)
#define stringFileDescription	"VST Megalay - MIDI controlled delay processor (64Bit)"
#else
#define stringFileDescription	"VST Megalay - MIDI controlled delay processor"
#endif
#define stringCompanyName		"BassBuster\0"
#define stringLegalCopyright	"(c) 2011 BassBuster"
#define stringLegalTrademarks	"VST is a trademark of Steinberg Media Technologies GmbH"

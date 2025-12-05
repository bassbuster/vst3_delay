#include "megalaycontroller.h"
#include "megalayparamids.h"

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include "base/source/fstring.h"

#include <stdio.h>
#include <math.h>

#define READ_PARAMETER(SavedParameter) \
int32 SavedParameter = 0.f;\
    if (state->read (&SavedParameter, sizeof (int32)) != kResultOk)\
{\
        return kResultFalse;\
}

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// GainParameter Declaration
// example of custom parameter (overwriting to and fromString)
//------------------------------------------------------------------------
class MegalayParameter : public Parameter
{
public:
	MegalayParameter (int32 flags, int32 id);
	MegalayParameter (const TChar* title, const TChar* units, int32 flags, int32 id);

    void toString (ParamValue normValue, String128 string) const override;
    bool fromString (const TChar* string, ParamValue& normValue) const override;
};

//------------------------------------------------------------------------
// GainParameter Implementation
//------------------------------------------------------------------------
MegalayParameter::MegalayParameter (int32 flags, int32 id)
{
	Steinberg::UString (info.title, USTRINGSIZE (info.title)).assign (USTRING ("Gain"));
	Steinberg::UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 0.5f;
	info.unitId = kRootUnitId;

	setNormalized (1.f);
}

MegalayParameter::MegalayParameter (const TChar* title, const TChar* units, int32 flags, int32 id)
{
	Steinberg::UString (info.title, USTRINGSIZE (info.title)).assign (title);
	Steinberg::UString (info.units, USTRINGSIZE (info.units)).assign (units);
	//if (units) {
	//	info.units.assign (units);
	//}

	info.flags = flags;
	info.id = id;
	info.stepCount = 0;
	info.defaultNormalizedValue = 0.5f;
	info.unitId = kRootUnitId;

	setNormalized (1.f);
}

//------------------------------------------------------------------------
void MegalayParameter::toString (ParamValue normValue, String128 string) const
{
	char text[32];
	if (normValue > 0.0001)
	{
		sprintf (text, "%.2f", 20 * log10 ((float)normValue));
	}
	else
	{
		strcpy (text, "-oo");
	}

	Steinberg::UString (string, 128).fromAscii (text);
}

//------------------------------------------------------------------------
bool MegalayParameter::fromString (const TChar* string, ParamValue& normValue) const
{
	String wrapper ((TChar*)string); // don't know buffer size here!
	double tmp = 0.0;
	if (wrapper.scanFloat (tmp))
	{
		// allow only values between -oo and 0dB
		if (tmp > 0.0)
		{
			tmp = -tmp;
		}

		normValue = exp (log (10.f) * (float)tmp / 20.f);
		return true;
	}
	return false;
}


//------------------------------------------------------------------------
// AGainController Implementation
//------------------------------------------------------------------------
AMegalayController::AMegalayController ()
: iMidiGain (0)
, iMidiAuxGain (kCtrlGPC1)
, iMidiDelay (kCtrlFilterResonance)
, iMidiFeed (kCtrlFilterCutoff)
, iMidiCut (kCtrlGPC6)
, iMidiMix (kCtrlEff1Depth)
, iBalance (0)
, iMidiVibroFreq (kCtrlGPC5)
, iMidiVibroDens (kCtrlGPC4)
, iMidiPower (kCtrlBreath)
, iMidiPingPong (0)
, iMidiBypass (0)
{} 

tresult PLUGIN_API AMegalayController::initialize (FUnknown* context)
{
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	//--- Create Units-------------
	UnitInfo unitInfo;
	Unit* unit;

	// create root only if you want to use the programListId
	unitInfo.id = kRootUnitId;	// always for Root Unit
	unitInfo.parentUnitId = kNoParentUnitId;	// always for Root Unit
	Steinberg::UString (unitInfo.name, USTRINGSIZE (unitInfo.name)).assign (USTRING ("Root"));
	unitInfo.programListId = kNoProgramListId;
	
	unit = new Unit (unitInfo);
	addUnit (unit);

	//---Create Parameters------------

	//---VuMeter parameter---
	int32 stepCount = 0;
	ParamValue defaultVal = 0;
	int32 flags = ParameterInfo::kIsReadOnly;
	int32 tag = kVuPPMId;
	parameters.addParameter (STR16 ("VuPPM"), 0, stepCount, defaultVal, flags, tag);

	//---Bypass parameter---
	stepCount = 1;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass;
	tag = kBypassId;
	parameters.addParameter (STR16 ("Bypass"), 0, stepCount, defaultVal, flags, tag);

	//---Delay parameter---
	stepCount = 0;
	defaultVal = 0.5;
	flags = ParameterInfo::kCanAutomate;
	tag = kDelayId;
	parameters.addParameter (STR16 ("Delay"), STR16 ("Beats"), stepCount, defaultVal, flags, tag);

	//---Feed parameter---
	stepCount = 0;
	defaultVal = 0.5;
	flags = ParameterInfo::kCanAutomate;
	tag = kFeedId;
	parameters.addParameter (STR16 ("FeedBack"), STR16 ("%"), stepCount, defaultVal, flags, tag);

	//---Cut parameter---
	stepCount = 0;
	defaultVal = 0.75;
	flags = ParameterInfo::kCanAutomate;
	tag = kCutId;
	parameters.addParameter (STR16 ("CutOff"), STR16 ("%"), stepCount, defaultVal, flags, tag);

	//---Mix parameter---
	stepCount = 0;
	defaultVal = 0.5;
	flags = ParameterInfo::kCanAutomate;
	tag = kMixId;
	parameters.addParameter (STR16 ("Mix"), STR16 ("D:W"), stepCount, defaultVal, flags, tag);

	//---Balanse parameter---
	stepCount = 0;
	defaultVal = 0.5;
	flags = ParameterInfo::kCanAutomate;
	tag = kBalanceId;
	parameters.addParameter (STR16 ("Balance"), STR16 ("L:R"), stepCount, defaultVal, flags, tag);

	//---PinPong L<->R parameter---
	stepCount = 1;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate|ParameterInfo::kIsWrapAround;
	tag = kPingPongId;
	parameters.addParameter (STR16 ("PingPong"), STR16 ("On|Off"), stepCount, defaultVal, flags, tag);

	//---Vibro Freq parameter---
	stepCount = 0;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate;
	tag = kVFreqId;
	parameters.addParameter (STR16 ("VibroFreq"), STR16 ("Beats"), stepCount, defaultVal, flags, tag);

	//---Vibro Densety parameter---
	stepCount = 0;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate;
	tag = kVDensId;
	parameters.addParameter (STR16 ("VibroDens"), STR16 ("%"), stepCount, defaultVal, flags, tag);

	//---Power Dist parameter---
	stepCount = 0;
	defaultVal = 0;
	flags = ParameterInfo::kCanAutomate;
	tag = kPowerId;
	parameters.addParameter (STR16 ("Owerdrive"), STR16 ("%"), stepCount, defaultVal, flags, tag);

	//---Aux parameter--
	MegalayParameter* auxParam = new MegalayParameter (STR16 ("AuxVol"), STR16 ("dB"),ParameterInfo::kCanAutomate, kAuxId);
	parameters.addParameter (auxParam);

	auxParam->setUnitID (kRootUnitId);

	//---Gain parameter--
	MegalayParameter* MegalayParam = new MegalayParameter (ParameterInfo::kCanAutomate, kGainId);
	parameters.addParameter (MegalayParam);

	MegalayParam->setUnitID (kRootUnitId);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::terminate  ()
{
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::setComponentState (IBStream* state)
{
	// we receive the current state of the component (processor part)
	// we read only the gain and bypass value...
	if (state)
	{
		float savedGain = 0.f;
		if (state->read (&savedGain, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedGain)
	#endif
		setParamNormalized (kGainId, savedGain);

		float savedAuxGain = 0.f;
		if (state->read (&savedAuxGain, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedAuxGain)
	#endif
		setParamNormalized (kAuxId, savedAuxGain);

		float savedDelay = 0.f;
		if (state->read (&savedDelay, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedDelay)
	#endif
		setParamNormalized (kDelayId, savedDelay);

		float savedFeed = 0.f;
		if (state->read (&savedFeed, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedFeed)
	#endif
		setParamNormalized (kFeedId, savedFeed);

		float savedCut = 0.f;
		if (state->read (&savedCut, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedCut)
	#endif
		setParamNormalized (kCutId, savedCut);

		float savedMix = 0.f;
		if (state->read (&savedMix, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedMix)
	#endif
		setParamNormalized (kMixId, savedMix);

		float savedBalance = 0.f;
		if (state->read (&savedBalance, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedBalance)
	#endif
		setParamNormalized (kBalanceId, savedBalance);

		float savedVibroFreq = 0.f;
		if (state->read (&savedVibroFreq, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedVibroFreq)
	#endif
		setParamNormalized (kVFreqId, savedVibroFreq);

		float savedVibroDens = 0.f;
		if (state->read (&savedVibroDens, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedVibroDens)
	#endif
		setParamNormalized (kVDensId, savedVibroDens);

		float savedPower = 0.f;
		if (state->read (&savedPower, sizeof (float)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedPower)
	#endif
		setParamNormalized (kPowerId, savedPower);

		int32 savedPingPong = 0.f;
		if (state->read (&savedPingPong, sizeof (int32)) != kResultOk)
		{
			return kResultFalse;
		}

	#if BYTEORDER == kBigEndian
		SWAP_32 (savedPingPong)
	#endif
		setParamNormalized (kPingPongId, savedPingPong ? 1 : 0);

		// jump the GainReduction
		//state->seek (sizeof (float), IBStream::kIBSeekCur);
	
		// read the bypass
		int32 bypassState;
		if (state->read (&bypassState, sizeof (bypassState)) == kResultTrue)
		{
		#if BYTEORDER == kBigEndian
			SWAP_32 (bypassState)
		#endif
			setParamNormalized (kBypassId, bypassState ? 1 : 0);
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::setState (IBStream* state)
{
	int8 byteOrder;
	if (state->read (&byteOrder, sizeof (int8)) != kResultTrue)
	{
		return kResultFalse;
	}
	
	READ_PARAMETER(savedMidiGain)
	READ_PARAMETER(savedMidiAuxGain)
	READ_PARAMETER(savedMidiDelay)
	READ_PARAMETER(savedMidiFeed)
	READ_PARAMETER(savedMidiCut)
	READ_PARAMETER(savedMidiMix)
	READ_PARAMETER(savedBalance)
	READ_PARAMETER(savedMidiVibroFreq)
	READ_PARAMETER(savedMidiVibroDens)
	READ_PARAMETER(savedMidiPower)
	READ_PARAMETER(savedMidiPingPong)
	READ_PARAMETER(savedMidiBypass)
	

	// if the byteorder doesn't match, byte swap the text array ...
	if (byteOrder != BYTEORDER)
	{
		SWAP_32(savedMidiGain)
		SWAP_32(savedMidiAuxGain)
		SWAP_32(savedMidiDelay)
		SWAP_32(savedMidiFeed)
		SWAP_32(savedMidiCut)
		SWAP_32(savedMidiMix)
		SWAP_32(savedBalance)
		SWAP_32(savedMidiVibroFreq)
		SWAP_32(savedMidiVibroDens)
		SWAP_32(savedMidiPower)
		SWAP_32(savedMidiPingPong)
		SWAP_32(savedMidiBypass)
	}

	iMidiGain =		  savedMidiGain;
    iMidiAuxGain =    savedMidiAuxGain;
    iMidiDelay =      savedMidiDelay;
    iMidiFeed =       savedMidiFeed;
    iMidiCut =        savedMidiCut;
    iMidiMix =        savedMidiMix;
    iBalance =        savedBalance;
    iMidiVibroFreq =  savedMidiVibroFreq;
    iMidiVibroDens =  savedMidiVibroDens;
    iMidiPower =      savedMidiPower;
    iMidiPingPong =   savedMidiPingPong;
	iMidiBypass =     savedMidiBypass;
	
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::getState (IBStream* state)
{
	// here we can save UI settings for example
	// as we save a Unicode string, we must know the byteorder when setState is called
	int8 byteOrder = BYTEORDER;
	if (state->write (&byteOrder, sizeof (int8)) != kResultTrue)
	{
		return kResultFalse;
	}
	
	int32 savedMidiGain =		iMidiGain;
    int32 savedMidiAuxGain =    iMidiAuxGain;
    int32 savedMidiDelay =      iMidiDelay;
    int32 savedMidiFeed =       iMidiFeed;
    int32 savedMidiCut =        iMidiCut;
    int32 savedMidiMix =        iMidiMix;
    int32 savedBalance =        iBalance;
    int32 savedMidiVibroFreq =  iMidiVibroFreq;
    int32 savedMidiVibroDens =  iMidiVibroDens;
    int32 savedMidiPower =      iMidiPower;
    int32 savedMidiPingPong =   iMidiPingPong;
	int32 savedMidiBypass =     iMidiBypass;
	
	state->write (&savedMidiGain, sizeof (int32));
	state->write (&savedMidiAuxGain, sizeof (int32));
	state->write (&savedMidiDelay, sizeof (int32));
	state->write (&savedMidiFeed, sizeof (int32));
	state->write (&savedMidiCut, sizeof (int32));
	state->write (&savedMidiMix, sizeof (int32));
	state->write (&savedBalance, sizeof (int32));
	state->write (&savedMidiVibroFreq, sizeof (int32));
	state->write (&savedMidiVibroDens, sizeof (int32));
	state->write (&savedMidiPower, sizeof (int32));
	state->write (&savedMidiPingPong, sizeof (int32));
	state->write (&savedMidiBypass, sizeof (int32));
	
	return kResultOk;
} 

//------------------------------------------------------------------------
tresult AMegalayController::receiveText (const char* text)
{
	// received from Component
	if (text)
	{
		fprintf (stderr, "[MegalayController] received: ");
        fprintf (stderr, "%s", text);
		fprintf (stderr, "\n");
	}
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::setParamNormalized (ParamID tag, ParamValue value)
{
	// called from host to update our parameters state
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string)
{
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, IMidiMapping::iid, IMidiMapping)
	return EditControllerEx1::queryInterface (iid, obj);
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalayController::getMidiControllerAssignment (int32 busIndex, int16 midiChannel,
																 CtrlNumber midiControllerNumber, ParamID& tag)
{
	// we support for the Gain parameter all Midi Channel
	if (busIndex == 0) {
		if (midiControllerNumber == iMidiGain)		{tag=kGainId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiAuxGain)	{tag=kAuxId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiDelay)		{tag=kDelayId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiFeed)		{tag=kFeedId; 		return kResultTrue;} 
		if (midiControllerNumber == iMidiCut)		{tag=kCutId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiMix)		{tag=kMixId; 		return kResultTrue;}
		if (midiControllerNumber == iBalance)		{tag=kBalanceId; 	return kResultTrue;}
		if (midiControllerNumber == iMidiVibroFreq)	{tag=kVFreqId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiVibroDens)	{tag=kVDensId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiPower)		{tag=kPowerId; 		return kResultTrue;}
		if (midiControllerNumber == iMidiPingPong)	{tag=kPingPongId;	return kResultTrue;}
		if (midiControllerNumber == iMidiBypass)	{tag=kBypassId; 	return kResultTrue;}
	}

	return kResultFalse;
}

}} // namespaces

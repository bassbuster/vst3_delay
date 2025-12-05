#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
//class AMegalayEditorView;

//------------------------------------------------------------------------
// AGainController
//------------------------------------------------------------------------
class AMegalayController: public EditControllerEx1, public IMidiMapping
{
public:
//------------------------------------------------------------------------
// create function required for plug-in factory,
// it will be called to create new instances of this controller
//------------------------------------------------------------------------
	AMegalayController();
	static FUnknown* createInstance (void* context)
	{
		return (IEditController*)new AMegalayController;
	}

	//---from IPluginBase--------
    tresult PLUGIN_API initialize (FUnknown* context) override;
    tresult PLUGIN_API terminate  () override;

	//---from EditController-----
    tresult PLUGIN_API setComponentState (IBStream* state) override;
    tresult PLUGIN_API setState (IBStream* state) override;
    tresult PLUGIN_API getState (IBStream* state) override;
    tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value) override;
    tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string) override;
    tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized) override;

	//---from ComponentBase-----
    tresult receiveText (const char* text) override;

	//---from IMidiMapping-----------------
    tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag) override;

	DELEGATE_REFCOUNT (EditController)
    tresult PLUGIN_API queryInterface (const char* iid, void** obj) override;

private:
	int32 iMidiGain;
	int32 iMidiAuxGain;
	int32 iMidiDelay;
	int32 iMidiFeed;
	int32 iMidiCut;
	int32 iMidiMix;
	int32 iBalance;
	int32 iMidiVibroFreq;
	int32 iMidiVibroDens;
	int32 iMidiPower;
	int32 iMidiPingPong;
	int32 iMidiBypass;
};

 }} // namespaces


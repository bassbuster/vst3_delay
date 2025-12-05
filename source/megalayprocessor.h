#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstunits.h"

#define FX_CHANNELS 2

namespace Steinberg {
namespace Vst {

const float pi = 3.1415926f;
//------------------------------------------------------------------------
// AMegalay: directly derived from the helper class AudioEffect
//------------------------------------------------------------------------
class AMegalay: public AudioEffect//, public IProgramListData
{
public:
	AMegalay ();
	virtual ~AMegalay ();	// dont forget virtual here

//------------------------------------------------------------------------
// create function required for plug-in factory,
// it will be called to create new instances of this plug-in
//------------------------------------------------------------------------
	static FUnknown* createInstance (void* context)
	{
		return (IAudioProcessor*)new AMegalay;
	}

//------------------------------------------------------------------------
// AudioEffect overrides:
//------------------------------------------------------------------------
	/** Called at first after constructor */
    tresult PLUGIN_API initialize (FUnknown* context) override;
	
	/** Called at the end before destructor */
    tresult PLUGIN_API terminate  () override;
	
	/** Switch the plug-in on/off */
    tresult PLUGIN_API setActive (TBool state) override;

	/** Here we go...the process call */
    tresult PLUGIN_API process (ProcessData& data) override;

	/** Test of a communication channel between controller and component */
    tresult receiveText (const char* text) override;
	
	/** For persistence */
    tresult PLUGIN_API setState (IBStream* state) override;
    tresult PLUGIN_API getState (IBStream* state) override;

	/** Will be called before any process call */
    tresult PLUGIN_API setupProcessing (ProcessSetup& newSetup) override;

	/** Bus arrangement managing: in this example the 'again' will be mono for mono input/output and stereo for other arrangements. */
    tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) override;

protected:
	// our model values
	float fVuPPMOld;
	float fGain;
	float fAuxGain;
	float fDelay;
	float fFeed;
	float fCut;
	float fMix;
	float fBalance;
	float fVibroFreq;
	float fVibroDens;
	float fPower;
	bool  bPingPong;
	bool  bBypass;

	//int32 currentProcessMode;

	float ** buffer;
	int32 bufferPos;
	int32 vibroPos;
	int32 bufferSize;
	int32 frequency;
	int32 delay;
	int32 cut;
	float Filter[FX_CHANNELS];

	double dTempo;
	double dSampleRate;
	int32 iMaxBeats;

	void PLUGIN_API setFreq (float aFreq);
	void PLUGIN_API setDelay (float aDelay);
	void PLUGIN_API setCut (float aCut);

	double PLUGIN_API aproxParamValue(int32 currentSampleIndex,double currentValue, int32 nextSampleIndex, double nextValue);
};

//---------------------------------------------------------------------------
template <class Value>
Value Sign(Value Val) {
  if (Val == 0.)  return 0;
  if (Val > 0.)  return 1;
  else return -1;
}


}} // namespaces


//------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.1.0
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/again/source/again.h
// Created by  : Steinberg, 04/2005
// Description : AGain Example for VST SDK 3.0
//               Simple gain plug-in with gain, bypass values and 1 midi input
//               and the same plug-in with sidechain 
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2010, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// This Software Development Kit may not be distributed in parts or its entirety  
// without prior written agreement by Steinberg Media Technologies GmbH. 
// This SDK must not be used to re-engineer or manipulate any technology used  
// in any Steinberg or Third-party application or software module, 
// unless permitted by law.
// Neither the name of the Steinberg Media Technologies nor the names of its
// contributors may be used to endorse or promote products derived from this 
// software without specific prior written permission.
// 
// THIS SDK IS PROVIDED BY STEINBERG MEDIA TECHNOLOGIES GMBH "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL STEINBERG MEDIA TECHNOLOGIES GMBH BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstunits.h"

#define FX_CHANNELS 2

#define GETPARAM_CHANGES(ParamQueue,Param,CurrentPosition,newParamValue,offsetParamValue,ParamPointIndex)\
					if(ParamQueue){\
						if (offsetParamValue == CurrentPosition) {\
							Param = newParamValue;\
							if (ParamQueue->getPoint (ParamPointIndex++,  offsetParamValue, newParamValue) != kResultTrue)\
								ParamQueue = 0;\
						}else\
						if (offsetParamValue > CurrentPosition) {\
							Param = aproxParamValue(CurrentPosition,Param,offsetParamValue,newParamValue);\
						}\
					}

#define GETPARAM_CHANGES_FUNC(ParamQueue,Param,CurrentPosition,newParamValue,offsetParamValue,ParamPointIndex,Func)\
					if(ParamQueue){\
						if (offsetParamValue == CurrentPosition) {\
							Func(newParamValue);\
							if (ParamQueue->getPoint (ParamPointIndex++,  offsetParamValue, newParamValue) != kResultTrue)\
								ParamQueue = 0;\
						}else\
						if (offsetParamValue > CurrentPosition) {\
							Func(aproxParamValue(CurrentPosition,Param,offsetParamValue,newParamValue));\
						}\
					}

#define GETPARAM_CHANGES_BOOL(ParamQueue,Param,CurrentPosition,newParamValue,offsetParamValue,ParamPointIndex)\
					if(ParamQueue){\
						if (offsetParamValue == CurrentPosition) {\
							Param = (newParamValue > 0.5f);\
							if (ParamQueue->getPoint (ParamPointIndex++,  offsetParamValue, newParamValue) != kResultTrue)\
								ParamQueue = 0;\
						}\
					}

#define READ_SAVED_PARAMETER(SavedParameter) \
	float SavedParameter = 0.f;\
	if (state->read (&SavedParameter, sizeof (float)) != kResultOk)\
	{\
		return kResultFalse;\
	}

#define READ_SAVED_PARAMETER_INT(SavedParameter) \
	int32 SavedParameter = 0.f;\
	if (state->read (&SavedParameter, sizeof (int32)) != kResultOk)\
	{\
		return kResultFalse;\
	}


#define READ_SAVED_PARAMETER_INT8(SavedParameter) \
uint8_t SavedParameter = 0.f;\
    if (state->read (&SavedParameter, sizeof (uint8_t)) != kResultOk)\
{\
        return kResultFalse;\
}

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
	tresult PLUGIN_API initialize (FUnknown* context);
	
	/** Called at the end before destructor */
	tresult PLUGIN_API terminate  ();
	
	/** Switch the plug-in on/off */
	tresult PLUGIN_API setActive (TBool state);

	/** Here we go...the process call */
	tresult PLUGIN_API process (ProcessData& data);

	/** Test of a communication channel between controller and component */
	tresult receiveText (const char* text);
	
	/** For persistence */
	tresult PLUGIN_API setState (IBStream* state);
	tresult PLUGIN_API getState (IBStream* state);

	/** Will be called before any process call */
	tresult PLUGIN_API setupProcessing (ProcessSetup& newSetup);

	/** Bus arrangement managing: in this example the 'again' will be mono for mono input/output and stereo for other arrangements. */
	tresult PLUGIN_API setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);


	//tresult PLUGIN_API programDataSupported (ProgramListID listId);
	//tresult PLUGIN_API getProgramData (ProgramListID listId, int32 programIndex, IBStream *data);
	//tresult PLUGIN_API setProgramData (ProgramListID listId, int32 programIndex, IBStream *data);
	//DELEGATE_REFCOUNT (AudioEffect)
	//tresult PLUGIN_API queryInterface (const char* iid, void** obj);
//------------------------------------------------------------------------
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
	//int32 iFxChannels;

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


//------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.1.0
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/again/source/again.cpp
// Created by  : Steinberg, 04/2005
// Description : AGain Example for VST SDK 3.0
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

#include "megalayprocessor.h"
#include "megalayparamids.h"
#include "megalaycids.h"	// for class ids

#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <stdio.h>
#include <math.h>

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// AGain Implementation
//------------------------------------------------------------------------
AMegalay::AMegalay ()
: fGain (1.f)
, fAuxGain (1.f)
, fVuPPMOld (0.f)
, fDelay (0.5f)
, fFeed (0.5f)
, fCut (0.25f)
, fMix (0.5f)
, fBalance (0.5f)
, fVibroFreq (0.f)
, fVibroDens (0.f)
, fPower (0.f)
//, currentProcessMode (-1) // -1 means not initialized
, bPingPong (false)
, bBypass (false)
, iMaxBeats (4)
, dTempo (120.f)
, dSampleRate (44100.f)
{
	// register its editor class (the same than used in againentry.cpp)
	vibroPos = 0;
	bufferPos = 0;
	setControllerClass (AMegalayControllerUID);
}

//------------------------------------------------------------------------
AMegalay::~AMegalay ()
{
	// nothing to do here yet..
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::initialize (FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//---create Audio In/Out busses------
	// we want a stereo Input and a Stereo Output
	addAudioInput  (STR16 ("Stereo In"),  SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);
	addAudioInput  (STR16 ("Stereo Aux In"),  SpeakerArr::kStereo, kAux, 0);

	//---create Midi In/Out busses (1 bus with only 1 channel)------
	addEventInput (STR16 ("Midi In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::terminate  ()
{
	// nothing to do here yet...except calling our parent terminate
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::setActive (TBool state)
{
	//if (state)
	//{
	//	sendTextMessage ("AMegalay::setActive (true)");
	//}
	//else
	//{
	//	sendTextMessage ("AMegalay::setActive (false)");
	//}

	SpeakerArrangement arr;
	if (getBusArrangement (kOutput, 0, arr) != kResultTrue)
		return kResultFalse;
	int32 numChannels = SpeakerArr::getChannelCount (arr);
	if (numChannels == 0)
		return kResultFalse;

	if (state)
	{
		dSampleRate = processSetup.sampleRate;
		//dTempo = processContext.tempo;
		buffer = (float**)malloc (FX_CHANNELS * sizeof (float*));
		bufferSize = dSampleRate / (dTempo / 60.0) * iMaxBeats;
		for (int32 channel = 0; channel < FX_CHANNELS; channel++)
		{
			buffer[channel] = (float*)malloc (bufferSize * sizeof (float));	// 1 second delay max
			memset (buffer[channel], 0, bufferSize * sizeof (float));
		}
		setFreq(fVibroFreq);
		setDelay(fDelay);
		for (int32 i = 0; i < FX_CHANNELS; i++)
		{
			Filter[i] = 0;
		}
		bufferPos = 0;
	}
	else
	{
		if (buffer)
		{
			for (int32 channel = 0; channel < FX_CHANNELS; channel++)
			{
				free (buffer[channel]);
			}
			free (buffer);
			buffer = 0;
		}
	}

	// reset the VuMeter value
	fVuPPMOld = 0.f;

	// call our parent setActive
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
void PLUGIN_API AMegalay::setFreq (float aFreq)
{
	float oFrequensy = frequency;
	fVibroFreq = aFreq;
	if (aFreq<0.5){
		//frequency = (fVibroFreq * 2.0 * ((bufferSize - 1.f)/ (float)iMaxBeats)) + 1;
		frequency = (exp((0.5 - fVibroFreq) * -9.0) * ((bufferSize - 1.f)/ (float)iMaxBeats)) + 1;
	}else{
		frequency = (((fVibroFreq - 0.5) * 2.0 * (iMaxBeats - 1)+1) * ((bufferSize - 1.f)/ (float)iMaxBeats))+1;
	}
	if (frequency < 500) { frequency = 500;}
	if (oFrequensy>0) {vibroPos = ((float)frequency /(float)oFrequensy) * vibroPos; }else{ vibroPos = 0; }

}
//------------------------------------------------------------------------
void PLUGIN_API AMegalay::setDelay (float aDelay)
{
	//float oDelay = delay;
	fDelay = aDelay;
	if (aDelay<0.5) {
	  delay = (aDelay * 2.0 * ((bufferSize - 1.f)/ (float)iMaxBeats)) + 1;
	}else{
	  delay = (((aDelay - 0.5) * 2.0 * (iMaxBeats - 1)+1) * ((bufferSize - 1.f)/ (float)iMaxBeats)) + 1;
	}
}
//------------------------------------------------------------------------
void PLUGIN_API AMegalay::setCut (float aCut)
{
	//float oDelay = delay;
	fCut = aCut;
	cut = (fCut * 10.0);
}

//------------------------------------------------------------------------
double PLUGIN_API AMegalay::aproxParamValue(int32 currentSampleIndex,double currentValue, int32 nextSampleIndex, double nextValue)
{
	//double x1 = currentSampleIndex-1; // position of last point related to current buffer
	//double y1 = currentValue; // last transmitted value

	//double x2 = nextSampleIndex;
	//double y2 = nextValue;

	double slope = (nextValue - currentValue) / (double)(nextSampleIndex - currentSampleIndex + 1);
	double offset = currentValue - (slope * (double)(currentSampleIndex - 1));

	return (slope * (double)currentSampleIndex) + offset; // bufferTime is any position in buffer


}
//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::process (ProcessData& data)
{
	// finally the process function
	// In this example there are 4 steps:
	// 1) Read inputs parameters coming from host (in order to adapt our model values)
	// 2) Read inputs events coming from host (we apply a gain reduction depending of the velocity of pressed key)
	// 3) Process the gain of the input buffer to the output buffer
	// 4) Write the new VUmeter value to the output Parameters queue

	if ((data.processContext)&& ((data.processContext->sampleRate != dSampleRate)||(dTempo != data.processContext->tempo))) {
		dSampleRate = data.processContext->sampleRate;
		dTempo = data.processContext->tempo;
		bufferSize = dSampleRate / (dTempo / 60.0) * iMaxBeats;
		for (int32 channel = 0; channel < FX_CHANNELS; channel++)
		{
			buffer[channel] = (float *)realloc (buffer[channel],bufferSize * sizeof (float));	
			//memset (buffer[channel], 0, bufferSize * sizeof (float));
		}
		bufferPos = 0;
		setFreq(fVibroFreq);
		setDelay(fDelay);
	}




	//---1) Read inputs parameter changes-----------
	IParameterChanges* paramChanges = data.inputParameterChanges;
	
	IParamValueQueue* GainQueue = 0;		int32 GainPointIndex = 0;		int32 offsetGainChange = 0;			double newGainValue = fGain;
	IParamValueQueue* BypassQueue = 0;      int32 BypassPointIndex = 0;     int32 offsetBypassChange = 0;       double newBypassValue;
	IParamValueQueue* PingPongQueue = 0;    int32 PingPongPointIndex = 0;   int32 offsetPingPongChange = 0;     double newPingPongValue;
	IParamValueQueue* AuxQueue = 0;         int32 AuxPointIndex = 0;        int32 offsetAuxChange = 0;          double newAuxValue = fAuxGain;
	IParamValueQueue* DelayQueue = 0;       int32 DelayPointIndex = 0;      int32 offsetDelayChange = 0;        double newDelayValue = fDelay;
	IParamValueQueue* FeedQueue = 0;        int32 FeedPointIndex = 0;       int32 offsetFeedChange = 0;         double newFeedValue = fFeed;
	IParamValueQueue* CutQueue = 0;         int32 CutPointIndex = 0;        int32 offsetCutChange = 0;          double newCutValue = fCut;
	IParamValueQueue* MixQueue = 0;         int32 MixPointIndex = 0;        int32 offsetMixChange = 0;          double newMixValue = fMix;
	IParamValueQueue* BalanceQueue = 0;     int32 BalancePointIndex = 0;    int32 offsetBalanceChange = 0;      double newBalanceValue = fBalance;
	IParamValueQueue* VFreqQueue = 0;       int32 VFreqPointIndex = 0;      int32 offsetVFreqChange = 0;        double newVFreqValue = fVibroFreq;
	IParamValueQueue* VDensQueue = 0;       int32 VDensPointIndex = 0;      int32 offsetVDensChange = 0;        double newVDensValue = fVibroDens;
	IParamValueQueue* PowerQueue = 0;       int32 PowerPointIndex = 0;      int32 offsetPowerChange = 0;        double newPowerValue = fPower;
	
	if (bBypass) newBypassValue = 1.f; 		else  newBypassValue = 0.f;
	if (bPingPong) newPingPongValue = 1.f; 	else  newPingPongValue = 0.f;
	if (paramChanges)
	{
		int32 numParamsChanged = paramChanges->getParameterCount ();
		// for each parameter which are some changes in this audio block:
		for (int32 i = 0; i < numParamsChanged; i++)
		{
			IParamValueQueue* paramQueue = paramChanges->getParameterData (i);
			if (paramQueue)
			{
				switch (paramQueue->getParameterId ())
				{
				case kGainId: 		GainQueue = paramQueue; 	paramQueue->getPoint (GainPointIndex++,  offsetGainChange, newGainValue); 				break;
				case kBypassId:		BypassQueue = paramQueue;	paramQueue->getPoint (BypassPointIndex++,  offsetBypassChange, newBypassValue);			break;
				case kPingPongId:	PingPongQueue = paramQueue;	paramQueue->getPoint (PingPongPointIndex++,  offsetPingPongChange, newPingPongValue);	break;
				case kAuxId:		AuxQueue = paramQueue;		paramQueue->getPoint (AuxPointIndex++,  offsetAuxChange, newAuxValue);					break;
				case kDelayId:		DelayQueue = paramQueue;	paramQueue->getPoint (DelayPointIndex++,  offsetDelayChange, newDelayValue);			break;
				case kFeedId:		FeedQueue = paramQueue;		paramQueue->getPoint (FeedPointIndex++,  offsetFeedChange, newFeedValue);				break;
				case kCutId:		CutQueue = paramQueue;		paramQueue->getPoint (CutPointIndex++,  offsetCutChange, newCutValue);					break;
				case kMixId:		MixQueue = paramQueue;		paramQueue->getPoint (MixPointIndex++,  offsetMixChange, newMixValue);					break;
				case kBalanceId:	BalanceQueue = paramQueue;	paramQueue->getPoint (BalancePointIndex++,  offsetBalanceChange, newBalanceValue);		break;
				case kVFreqId:		VFreqQueue = paramQueue;	paramQueue->getPoint (VFreqPointIndex++,  offsetVFreqChange, newVFreqValue);			break;
				case kVDensId:		VDensQueue = paramQueue;	paramQueue->getPoint (VDensPointIndex++,  offsetVDensChange, newVDensValue);			break;
				case kPowerId:		PowerQueue = paramQueue;	paramQueue->getPoint (PowerPointIndex++,  offsetPowerChange, newPowerValue);			break;
				}
			}
		}
	}

	//---2) Read input events-------------
	/*IEventList* eventList = data.inputEvents;
	if (eventList) 
	{
		int32 numEvent = eventList->getEventCount ();
		for (int32 i = 0; i < numEvent; i++)
		{

			Event event;
			if (eventList->getEvent (i, event) == kResultOk)
			{
				switch (event.type)
				{
					//----------------------
					case Event::kNoteOnEvent:
						// use the velocity as gain modifier
						fGainReduction = event.noteOn.velocity;
						break;
					
					//----------------------
					case Event::kNoteOffEvent:
						// noteOff reset the reduction
						fGainReduction = 0.f;
						break;

				}
			}
		}
	} */
		
	//-------------------------------------
	//---3) Process Audio---------------------
	//-------------------------------------
	if (data.numInputs == 0 || data.numOutputs == 0)
	{
		// nothing to do
		return kResultOk;
	}

	// (simplification) we suppose in this example that we have the same input channel count than the output
	//int32 numChannels = data.inputs[0].numChannels;
	int32 numChannels = data.outputs[0].numChannels;

	//---get audio buffers----------------
	float** in  = data.inputs[0].channelBuffers32;
	float** out = data.outputs[0].channelBuffers32;
	float** auxIn  = 0;
	bool auxActive = false;
	if (getAudioInput (1)->isActive ())
	{
		auxIn = data.inputs[1].channelBuffers32;
		auxActive = true;
	}

	//---check if silence---------------
	// normally we have to check each channel (simplification)
	if (data.inputs[0].silenceFlags != 0)
	{
		// mark output silence too
		data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
		
		// the plug-in has to be sure that if it sets the flags silence that the output buffer are clear
		int32 sampleFrames = data.numSamples;
		for (int32 i = 0; i < numChannels; i++)
		{
			// dont need to be cleared if the buffers are the same (in this case input buffer are already cleared by the host)
			//if (in[i] != out[i])
			//{
				memset (out[i], 0, sampleFrames * sizeof (float));
			//}
		}

		// nothing to do at this point
		return kResultOk;
	}

	// mark our outputs has not silent
	data.outputs[0].silenceFlags = 0;

	//---in bypass mode outputs should be like inputs-----
	//if (bBypass)
	//{
	//	int32 sampleFrames = data.numSamples;
	//	for (int32 i = 0; i < numChannels; i++)
	//	{
	//		// dont need to be copied if the buffers are the same
	//		//if (in[i] != out[i])
	//		//{
	//			memcpy (out[i], in[i], sampleFrames * sizeof (float));
	//		//}
	//	}
	//	// in this example we dont update the VuMeter in Bypass
	//}
	//else
	//{
		float fVuPPM = 0.f;
		float inpMix = sqrt(1.0 - fMix);
		float outMix = sqrt(fMix);

		//---apply gain factor----------
		
		// if the applied gain is nearly zero, we could say that the outputs are zeroed and we set the silence flags. 
		//if (fGain < 0.0000001)
		//{
		//	int32 sampleFrames = data.numSamples;
		//	for (int32 i = 0; i < numChannels; i++)
		//	{
		//		memset (out[i], 0, sampleFrames * sizeof (float));
		//	}
		//	data.outputs[0].silenceFlags = (1 << numChannels) - 1;  // this will set to 1 all channels
		//}
		//else
		//{
				int32 sampleFrames = data.numSamples;
				int32 sampleCounter = 0;
				
				float* ptrIn[FX_CHANNELS];
				float* ptrOut[FX_CHANNELS];
				float* ptrAux[FX_CHANNELS];


				int32 delayPos = bufferPos-delay;
				if (delayPos<0) {delayPos = bufferSize + delayPos;}
				
				for (int32 i = 0; i < FX_CHANNELS; i++)
				{
					ptrIn[i]  = in[i];
					ptrOut[i]  = out[i];
					if (auxActive) {
						ptrAux[i] = auxIn[i];
					}
				}
				float tmp[FX_CHANNELS];
				while (--sampleFrames >= 0)
				{
					///////////////////////////////////////////////////////////////////////////////////////////////////////
				
					GETPARAM_CHANGES(GainQueue,fGain,sampleCounter,newGainValue,offsetGainChange,GainPointIndex)
					GETPARAM_CHANGES(AuxQueue,fAuxGain,sampleCounter,newAuxValue,offsetAuxChange,AuxPointIndex)
					GETPARAM_CHANGES_FUNC(DelayQueue,fDelay,sampleCounter,newDelayValue,offsetDelayChange,DelayPointIndex,setDelay)
					GETPARAM_CHANGES(FeedQueue,fFeed,sampleCounter,newFeedValue,offsetFeedChange,FeedPointIndex)
					GETPARAM_CHANGES_FUNC(CutQueue,fCut,sampleCounter,newCutValue,offsetCutChange,CutPointIndex,setCut)
					GETPARAM_CHANGES(MixQueue,fMix,sampleCounter,newMixValue,offsetMixChange,MixPointIndex)
					GETPARAM_CHANGES(BalanceQueue,fBalance,sampleCounter,newBalanceValue,offsetBalanceChange,BalancePointIndex)
					GETPARAM_CHANGES_FUNC(VFreqQueue,fVibroFreq,sampleCounter,newVFreqValue,offsetVFreqChange,VFreqPointIndex,setFreq)
					GETPARAM_CHANGES(VDensQueue,fVibroDens,sampleCounter,newVDensValue,offsetVDensChange,VDensPointIndex)
					GETPARAM_CHANGES(PowerQueue,fPower,sampleCounter,newPowerValue,offsetPowerChange,PowerPointIndex)
					GETPARAM_CHANGES_BOOL(BypassQueue,bBypass,sampleCounter,newBypassValue,offsetBypassChange,BypassPointIndex)
					GETPARAM_CHANGES_BOOL(PingPongQueue,bPingPong,sampleCounter,newPingPongValue,offsetPingPongChange,PingPongPointIndex)

					///////////////////////////////////////////////////////////////////////////////////////////////////////
					
					if (!bBypass){ 
						float vibroSin[FX_CHANNELS];
						float vibroK[FX_CHANNELS];
						for (int32 i = 0; i < FX_CHANNELS; i++)
						{
							if (frequency>0) {
								if ((!bPingPong)&&(i>0)) { vibroK[i] = vibroK[i-1]; }
								else {
									vibroSin[i] = sin(((float)vibroPos)/((float)frequency) * 2.f * pi+(float)i * pi * (fBalance-0.5f) * 4.f / (float)FX_CHANNELS);
									vibroK[i] = 1.f - (0.5f + 0.5f*sqrt(sqrt(fabs(vibroSin[i])))*Sign(vibroSin[i]))*fVibroDens;
									if (vibroK[i]<0.05f) {
										vibroK[i] = 0.f;
									}
								}
							}else{
								vibroK[i] = 1.0;
							}
	
							if (auxActive) {
								tmp[i] = (*(ptrIn[i]++)) + (*(ptrAux[i]++)) * fAuxGain;
							}else{
								tmp[i] = (*(ptrIn[i]++));
							}
							
							Filter[i] = ((cut) * Filter[i] + *(buffer[i]+delayPos))/(cut+1);
						}
						for (int32 i = 0; i < FX_CHANNELS; i++)
						{
							#if ((FX_CHANNELS%2) == 0)
							if (!bPingPong)
							{
							#endif
								if (fBalance<=0.5)
								{
									if ((i%2)==0){
										(*(buffer[i]+bufferPos)) = tmp[i] + Filter[i]*fFeed;
									}else{
										(*(buffer[i]+bufferPos)) = tmp[i] * fBalance * 2.0 + Filter[i]*fFeed;
									}
								}else{
									if ((i%2)==0){
										(*(buffer[i]+bufferPos)) = tmp[i] * (1.0 - fBalance) * 2.0 + Filter[i]*fFeed;
									}else{
										(*(buffer[i]+bufferPos)) = tmp[i] + Filter[i]*fFeed;
									}
								}
							#if ((FX_CHANNELS%2) == 0)
							}else{
								if (fBalance<=0.5)
								{
									if ((i%2)==0){
										(*(buffer[i]+bufferPos)) = tmp[i] + Filter[i+1]*fFeed;
									}else{
										(*(buffer[i]+bufferPos)) = tmp[i] * fBalance * 2.0 + Filter[i-1]*fFeed;
									}
								}else{
									if ((i%2)==0){
										(*(buffer[i]+bufferPos)) = tmp[i] * (1.0 - fBalance) * 2.0 + Filter[i+1]*fFeed;
									}else{
										(*(buffer[i]+bufferPos)) = tmp[i] + Filter[i-1]*fFeed;
									}
								}
							}
							#endif
	
							float tmpOut = (tmp[i] * inpMix + Filter[i] * outMix);
	
							//owerdrive
							if (fPower>0.001) {
								if (tmpOut>0)
								{
									tmpOut = tmpOut*(1.f - fPower)  +  sqrt(tmpOut)*fPower;
								}else{
									tmpOut = tmpOut*(1.f - fPower)  - sqrt(-tmpOut)*fPower;
								}
							}
	
							// apply gain
							(*(ptrOut[i]++)) = tmpOut * vibroK[i] * fGain;
	
							// check only positive values of first channel
							if (tmp[i] > fVuPPM)
							{
								fVuPPM = tmp[i];
							}
						}
						if (++vibroPos >= frequency){ vibroPos = 0;}
						if (++bufferPos >= bufferSize){bufferPos = 0;}
						if (++delayPos >= bufferSize){delayPos = 0;}
					}
					else{
						for (int32 i = 0; i < FX_CHANNELS; i++)
						{
							tmp[i] = (*(ptrIn[i]++));
							(*(ptrOut[i]++)) = tmp[i];
							// check only positive values of first channel
							if (tmp[i] > fVuPPM)
							{
								fVuPPM = tmp[i];
							}
						}
					}
					sampleCounter++;
				}
				
				
			//}
		//}

		//---3) Write outputs parameter changes-----------
		IParameterChanges* paramOutChanges = data.outputParameterChanges;
		// a new value of VuMeter will be send to the host 
		// (the host will send it back in sync to our controller for updating our editor)
		if (paramOutChanges && fVuPPMOld != fVuPPM)
		{
			int32 index = 0;
			IParamValueQueue* paramQueue = paramOutChanges->addParameterData (kVuPPMId, index);
			if (paramQueue)
			{
				int32 index2 = 0;
				paramQueue->addPoint (0, fVuPPM, index2); 
			}
		}
		fVuPPMOld = fVuPPM;
	//}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult AMegalay::receiveText (const char* text)
{
	// received from Controller
	fprintf (stderr, "[Megalay] received: ");
    fprintf (stderr, "%s", text);
	fprintf (stderr, "\n");

	//bHalfGain = !bHalfGain;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::setState (IBStream* state)
{
	// called when we load a preset, the model has to be reloaded
	READ_SAVED_PARAMETER(savedGain)
	READ_SAVED_PARAMETER(savedAuxGain)
	READ_SAVED_PARAMETER(savedDelay)
	READ_SAVED_PARAMETER(savedFeed)
	READ_SAVED_PARAMETER(savedCut)
	READ_SAVED_PARAMETER(savedMix)
	READ_SAVED_PARAMETER(savedBalance)
	READ_SAVED_PARAMETER(savedVFreq)
	READ_SAVED_PARAMETER(savedVDens)
	READ_SAVED_PARAMETER(savedPower)
    READ_SAVED_PARAMETER_INT8(savedPing)
    READ_SAVED_PARAMETER_INT8(savedBypass)

#if BYTEORDER == kBigEndian
	SWAP_32 (savedGain)
	SWAP_32 (savedAuxGain)
	SWAP_32 (savedDelay)
	SWAP_32 (savedFeed)
	SWAP_32 (savedCut)
	SWAP_32 (savedMix)
	SWAP_32 (savedBalance)
	SWAP_32 (savedVFreq)
	SWAP_32 (savedVDens)
	SWAP_32 (savedPower)
    // SWAP_32 (savedPing)
    // SWAP_32 (savedBypass)
#endif
	
	fGain = savedGain;
	fAuxGain = savedAuxGain;
	fDelay = savedDelay;
	fFeed = savedFeed;
	fCut = savedCut;
	fMix = savedMix;
	fBalance = savedBalance;
	fVibroFreq = savedVFreq;
	fVibroDens = savedVDens;
	fPower = savedPower;
	bPingPong = savedPing > 0;
	bBypass = savedBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::getState (IBStream* state)
{
	// here we need to save the model

	float toSaveGain = fGain;
	float toSaveAuxGain = fAuxGain;
	float toSaveDelay = fDelay;
	float toSaveFeed = fFeed;
	float toSaveCut = fCut;
	float toSaveMix = fMix;
	float toSaveBalance = fBalance;
	float toSaveVFreq = fVibroFreq;
	float toSaveVDens = fVibroDens;
	float toSavePower = fPower;
    uint8_t toSavePingPong = bPingPong ? 1 : 0;
    uint8_t toSaveBypass = bBypass ? 1 : 0;

#if BYTEORDER == kBigEndian
	SWAP_32 (toSaveGain)
	SWAP_32 (toSaveAuxGain)
	SWAP_32 (toSaveDelay)
	SWAP_32 (toSaveFeed)
	SWAP_32 (toSaveCut)
	SWAP_32 (toSaveMix)
	SWAP_32 (toSaveBalance)
	SWAP_32 (toSaveVFreq)
	SWAP_32 (toSaveVDens)
	SWAP_32 (toSavePower)
    // SWAP_32 (toSavePingPong)
    // SWAP_32 (toSaveBypass)
#endif

	state->write (&toSaveGain, sizeof (float));
	state->write (&toSaveAuxGain, sizeof (float));
	state->write (&toSaveDelay, sizeof (float));
	state->write (&toSaveFeed, sizeof (float));
	state->write (&toSaveCut, sizeof (float));
	state->write (&toSaveMix, sizeof (float));
	state->write (&toSaveBalance, sizeof (float));
	state->write (&toSaveVFreq, sizeof (float));
	state->write (&toSaveVDens, sizeof (float));
	state->write (&toSavePower, sizeof (float));
    state->write (&toSavePingPong, sizeof (uint8_t));
    state->write (&toSaveBypass, sizeof (uint8_t));

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API AMegalay::setupProcessing (ProcessSetup& newSetup)
{
	// called before the process call, always in a disable state (not active)
	
	// here we keep a trace of the processing mode (offline,...) for example.
	//currentProcessMode = newSetup.processMode;

	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
/*tresult PLUGIN_API AMegalay::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	if (numIns > 0 && numOuts > 0)
	{
		// the host wants Mono => Mono
		if (inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
			return kResultOk;
		else
		{
			AudioBus* bus = FCast<AudioBus> (audioInputs.at (0));
			if (bus)
			{
				// check if we are Mono => Mono, if not we need to recreate the busses
				if (bus->getArrangement () != SpeakerArr::kStereo)
				{
					removeAudioBusses ();
					addAudioInput  (STR16 ("Mono In"),  SpeakerArr::kStereo);
					addAudioOutput (STR16 ("Mono Out"), SpeakerArr::kStereo);
					addAudioInput  (STR16 ("Stereo Aux In"),  SpeakerArr::kStereo, kAux, 0);
				}
				return kResultOk;
			}
		}
	}
	return kResultFalse;
}*/

tresult PLUGIN_API AMegalay::setBusArrangements (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
	if ((numIns == 1 || numIns == 2) && numOuts == 1)
	{
		if (SpeakerArr::getChannelCount (inputs[0]) == 2 && SpeakerArr::getChannelCount (outputs[0]) == 2)
		{
			AudioBus* bus = FCast<AudioBus> (audioInputs.at (0));
			if (bus)
			{
				if (bus->getArrangement () != inputs[0])
				{
					removeAudioBusses ();
					addAudioInput  (STR16 ("Stereo In"),  inputs[0]);
					addAudioOutput (STR16 ("Stereo Out"), outputs[0]);

					// recreate the Stereo SideChain input bus
					if ((numIns == 2) && (SpeakerArr::getChannelCount (inputs[1]) == 2))
						addAudioInput  (STR16 ("Stereo Aux In"), inputs[1], kAux, 0);
				}
			}
			return kResultTrue;
		}
	}
	return kResultFalse;
}

/*tresult PLUGIN_API AMegalay::programDataSupported (ProgramListID listId)
{
	if (listId == 0) {
		 return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API AMegalay::getProgramData (ProgramListID listId, int32 programIndex, IBStream *data)
{
	if (listId == 0) {
		 return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API AMegalay::setProgramData (ProgramListID listId, int32 programIndex, IBStream *data)
{
	if (listId == 0) {
		 return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API AMegalay::queryInterface (const char* iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, IProgramListData::iid, IProgramListData)
	return AudioEffect::queryInterface (iid, obj);
}*/


}} // namespaces

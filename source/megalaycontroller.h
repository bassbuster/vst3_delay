//------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 3.1.0
//
// Category    : Examples
// Filename    : public.sdk/samples/vst/again/source/againcontroller.h
// Created by  : Steinberg, 04/2005
// Description : AGain Editor Example for VST 3.0
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

#include "public.sdk/source/vst/vsteditcontroller.h"

#define READ_PARAMETER(SavedParameter) \
	int32 SavedParameter = 0.f;\
	if (state->read (&SavedParameter, sizeof (int32)) != kResultOk)\
	{\
		return kResultFalse;\
	}

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
	tresult PLUGIN_API initialize (FUnknown* context);
	tresult PLUGIN_API terminate  ();

	//---from EditController-----
	tresult PLUGIN_API setComponentState (IBStream* state);
	//IPlugView* PLUGIN_API createView (const char* name);
	tresult PLUGIN_API setState (IBStream* state);
	tresult PLUGIN_API getState (IBStream* state);
	tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value);
	tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string);
	tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized);
	//void editorDestroyed (AMegalayEditorView* editor) {} // nothing to do here
	//void editorAttached (AMegalayEditorView* editor);
	//void editorRemoved (AMegalayEditorView* editor);

	//int32 PLUGIN_API getProgramListCount ();
	//tresult PLUGIN_API getProgramListInfo (int32 listIndex, ProgramListInfo& info /*out*/);
	//tresult PLUGIN_API getProgramName (ProgramListID listId, int32 programIndex, String128 name /*out*/);
	//tresult PLUGIN_API getProgramInfo (ProgramListID listId, int32 programIndex, CString attributeId, String128 attributeValue);

	//---from ComponentBase-----
	tresult receiveText (const char* text);

	//---from IMidiMapping-----------------
	tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag);

	DELEGATE_REFCOUNT (EditController)
	tresult PLUGIN_API queryInterface (const char* iid, void** obj);

	//---Internal functions-------
	//void addDependentView (AMegalayEditorView* view);
	//void removeDependentView (AMegalayEditorView* view);

	//void setDefaultMessageText (String128 text);
	//TChar* getDefaultMessageText ();
//------------------------------------------------------------------------

private:
	//TArray <AMegalayEditorView*> viewsArray;
	//String128 defaultMessageText;
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


/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 4.0.2

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright (c) 2015 - ROLI Ltd.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "ModeSelect.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
ModeSelect::ModeSelect ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    addAndMakeVisible (fServer = new TextButton ("server"));
    fServer->setButtonText (TRANS("Server"));
    fServer->addListener (this);

    addAndMakeVisible (Client = new TextButton ("client"));
    Client->setButtonText (TRANS("Client"));
    Client->addListener (this);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

ModeSelect::~ModeSelect()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    fServer = nullptr;
    Client = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void ModeSelect::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (Colours::white);

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void ModeSelect::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    fServer->setBounds (40, 56, 150, 24);
    Client->setBounds (216, 56, 150, 24);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void ModeSelect::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]
    int retval;
    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == fServer)
    {
        //[UserButtonCode_fServer] -- add your button handler code here..
        retval = 0;
        //[/UserButtonCode_fServer]
    }
    else if (buttonThatWasClicked == Client)
    {
        //[UserButtonCode_Client] -- add your button handler code here..
        retval = 1;
        //[/UserButtonCode_Client]
    }

    //[UserbuttonClicked_Post]
    if (DialogWindow* dw = this->findParentComponentOfClass<DialogWindow>())
    {
        dw->exitModalState(retval);
    }
    //[/UserbuttonClicked_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Introjucer information section --

    This is where the Introjucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="ModeSelect" componentName=""
                 parentClasses="public Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ffffffff"/>
  <TEXTBUTTON name="server" id="3264acc4b45a479b" memberName="fServer" virtualName=""
              explicitFocusOrder="0" pos="40 56 150 24" buttonText="Server"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
  <TEXTBUTTON name="client" id="cba980a4c3e4028e" memberName="Client" virtualName=""
              explicitFocusOrder="0" pos="216 56 150 24" buttonText="Client"
              connectedEdges="0" needsCallback="1" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

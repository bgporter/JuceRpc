/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#ifndef MAINCOMPONENT_H_INCLUDED
#define MAINCOMPONENT_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

#include "Controller.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainContentComponent   : public Component
                             , public ChangeListener
{
public:
    //==============================================================================
    MainContentComponent();
    ~MainContentComponent();

    void paint (Graphics&) override;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster* src) override;

    void SetController(Controller* controller);

    void SetText(const String& txt);

    void SetTreeText(const String& txt, int index=0);
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)

    String fText;

    String fTreeText;

    String fTree2Text;

    Controller* fController;
};


#endif  // MAINCOMPONENT_H_INCLUDED

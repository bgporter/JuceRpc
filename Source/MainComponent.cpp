/*
  Copyright 2016 Art & Logic Software Development. 
 */



#include "MainComponent.h"
#include "RpcException.h"

//==============================================================================
MainContentComponent::MainContentComponent()
:   fText("howdy.")
{
    setSize (600, 400);
}

MainContentComponent::~MainContentComponent()
{
}

void MainContentComponent::paint (Graphics& g)
{
    g.fillAll (Colour (0xff001F36));

    g.setFont (Font (16.0f));
    g.setColour (Colours::white);
    g.drawText (fText, 10, 10, this->getWidth(), 30, Justification::centred, true);
    g.drawMultiLineText(fTreeText, 10, 50, this->getWidth()-10);
    g.drawMultiLineText(fTree2Text, 10, 250, this->getWidth()-10);

}

void MainContentComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}


void MainContentComponent::changeListenerCallback(ChangeBroadcaster* src) 
{
    ClientController* client = dynamic_cast<ClientController*>(fController);
    if (client)
    {
        DBG("Calling StringFn");
        try
        {
            String newText = fController->StringFn("from server");
            this->SetText(newText);
        }
        catch (const RpcException<Controller::kTimeout>& e)
        {
            this->SetText("TIMEOUT error");
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        ValueTree tree = fController->GetTree(i);

        if (ValueTree::invalid != tree)
        {
            this->SetTreeText(tree.toXmlString(), i);
        }
        else
        {
            this->SetTreeText("ERROR getting tree at index " + String(i));
        }
    }

}

void MainContentComponent::SetController(Controller* controller)
{
    fController = controller;
    fController->addChangeListener(this);
}

void MainContentComponent::SetText(const String& txt)
{
    fText = txt;
    this->repaint();
}

void MainContentComponent::SetTreeText(const String& txt, int index)
{
    if (0 == index)
    {
        fTreeText = txt;
    }
    else if (1 == index)
    {
        fTree2Text = txt;
    }

    this->repaint();
}

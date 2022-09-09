/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RotaryKnob.h"
#include "LookAndFeel.h"

//==============================================================================
/**
*/
class JX11AudioProcessorEditor  : public juce::AudioProcessorEditor,
                                  private juce::Button::Listener, juce::Timer
{
public:
    JX11AudioProcessorEditor (JX11AudioProcessor&);
    ~JX11AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    JX11AudioProcessor& audioProcessor;

    LookAndFeel globalLNF;

    using APVTS = juce::AudioProcessorValueTreeState;
    using SliderAttachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    RotaryKnob outputLevelKnob;
    SliderAttachment outputLevelAttachment { audioProcessor.apvts, ParameterID::outputLevel.getParamID(), outputLevelKnob.slider };

    RotaryKnob filterResoKnob;
    SliderAttachment filterResoAttachment { audioProcessor.apvts, ParameterID::filterReso.getParamID(), filterResoKnob.slider };

    juce::TextButton polyModeButton;
    ButtonAttachment polyModeAttachment { audioProcessor.apvts, ParameterID::polyMode.getParamID(), polyModeButton };

    juce::TextButton midiLearnButton;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JX11AudioProcessorEditor)
};

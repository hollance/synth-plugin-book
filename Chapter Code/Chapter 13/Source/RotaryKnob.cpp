#include <JuceHeader.h>
#include "RotaryKnob.h"

static constexpr int labelHeight = 15;
static constexpr int textBoxHeight = 20;

RotaryKnob::RotaryKnob()
{
    slider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, textBoxHeight);
    slider.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    addAndMakeVisible(slider);

    setBounds(0, 0, 100, 120);
}

RotaryKnob::~RotaryKnob()
{
}

void RotaryKnob::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(15.0f);
    g.setColour(juce::Colours::white);

    auto bounds = getLocalBounds();
    g.drawText(label, juce::Rectangle<int>{ 0, 0, bounds.getWidth(), labelHeight }, juce::Justification::centred);

    /*
    g.setColour(juce::Colours::red);
    g.drawRect(getLocalBounds(), 1);

    g.setColour(juce::Colours::yellow);
    g.drawRect(0, labelHeight, bounds.getWidth(), bounds.getHeight() - labelHeight - textBoxHeight, 1);

    g.setColour(juce::Colours::green);
    g.drawRect(0, 0, bounds.getWidth(), labelHeight, 1);
    */
}

void RotaryKnob::resized()
{
    auto bounds = getLocalBounds();
    slider.setBounds(0, labelHeight, bounds.getWidth(), bounds.getHeight() - labelHeight);
}

#include "LookAndFeel.h"

LookAndFeel::LookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(30, 60, 90));

    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0, 0, 0));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(90, 180, 240));
    setColour(juce::Slider::thumbColourId, juce::Colour(255, 255, 255));

    setColour(juce::TextButton::buttonColourId, juce::Colour(15, 30, 45));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(90, 180, 240));
    setColour(juce::TextButton::textColourOffId, juce::Colour(180, 180, 180));
    setColour(juce::TextButton::textColourOnId, juce::Colour(255, 255, 255));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(180, 180, 180));

    juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor(BinaryData::LatoMedium_ttf, BinaryData::LatoMedium_ttfSize);
    setDefaultSansSerifTypeface(typeface);
}

void LookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int /*height*/, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto outlineColor = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
    auto fillColor = slider.findColour(juce::Slider::rotarySliderFillColourId);
    auto dialColor = slider.findColour(juce::Slider::thumbColourId);

    auto bounds = juce::Rectangle<int>(x, y, width, width).toFloat()
                              .withTrimmedLeft(16.0f).withTrimmedRight(16.0f)
                              .withTrimmedTop(0.0f).withTrimmedBottom(8.0f);

    auto radius = bounds.getWidth() / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 6.0f;
    auto arcRadius = radius - lineW / 2.0f;

    auto arg = toAngle - juce::MathConstants<float>::halfPi;
    auto dialW = 3.0f;
    auto dialRadius = arcRadius - 6.0f;

    auto center = bounds.getCentre();
    auto strokeType = juce::PathStrokeType(lineW, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::butt);

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(outlineColor);
    g.strokePath(backgroundArc, strokeType);

    if (slider.isEnabled()) {
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, toAngle, true);
        g.setColour(fillColor);
        g.strokePath(valueArc, strokeType);
    }

    juce::Point<float> thumbPoint(center.x + dialRadius * std::cos(arg),
                                  center.y + dialRadius * std::sin(arg));
    g.setColour(dialColor);
    g.drawLine(center.x, center.y, thumbPoint.x, thumbPoint.y, dialW);
    g.fillEllipse(juce::Rectangle<float>(dialW, dialW).withCentre(thumbPoint));
    g.fillEllipse(juce::Rectangle<float>(dialW, dialW).withCentre(center));
}

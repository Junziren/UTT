#include "AboutComponent.h"

AboutComponent::AboutComponent()
{
    // Semi-transparent dark backdrop
    setOpaque(false);

    titleLabel.setText("UTT - Utility Toolbox", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    devLabel.setText("Developer: UnpureBloom", juce::dontSendNotification);
    devLabel.setFont(14.0f);
    devLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    devLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(devLabel);

    bilibiliLink.setButtonText("Bilibili: space.bilibili.com/227573145");
    bilibiliLink.setURL(juce::URL("https://space.bilibili.com/227573145"));
    bilibiliLink.setFont(13.0f, false, juce::Font::underlined);
    bilibiliLink.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xff70c0ff));
    bilibiliLink.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bilibiliLink);

    licenseLabel.setText(
        "License: GNU General Public License v3.0\n"
        "This plugin is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 3 of the License, or\n"
        "(at your option) any later version.\n"
        "\n"
        "Built with JUCE (https://juce.com) under GPLv3.",
        juce::dontSendNotification);
    licenseLabel.setFont(11.0f);
    licenseLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    licenseLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(licenseLabel);

    versionLabel.setText("v0.2.0", juce::dontSendNotification);
    versionLabel.setFont(12.0f);
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff888888));
    versionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(versionLabel);

    closeLabel.setText("Click anywhere to close", juce::dontSendNotification);
    closeLabel.setFont(10.0f);
    closeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    closeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(closeLabel);
}

void AboutComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xdd111111));
    auto bounds = getLocalBounds().toFloat().reduced(40, 40);
    g.setColour(juce::Colour(0xee222222));
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(juce::Colour(0xff444444));
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);
}

void AboutComponent::resized()
{
    auto bounds = getLocalBounds().reduced(60, 60);
    titleLabel.setBounds(bounds.removeFromTop(36));
    bounds.removeFromTop(8);
    versionLabel.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(12);
    devLabel.setBounds(bounds.removeFromTop(24));
    bounds.removeFromTop(4);
    bilibiliLink.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(16);
    licenseLabel.setBounds(bounds.removeFromTop(120));
    bounds.removeFromTop(12);
    closeLabel.setBounds(bounds.removeFromTop(16));
}
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "gui/UTTWebView.h"
#include "gui/FallbackPanel.h"

namespace
{
constexpr int defaultWidth  = 1100;
constexpr int defaultHeight = 720;
constexpr int minWidth      = 900;
constexpr int minHeight     = 640;
constexpr int maxWidth      = 1600;
constexpr int maxHeight     = 1100;
} // namespace

UTTEditor::UTTEditor (UTTProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setResizable (true, true);
    getConstrainer()->setMinimumSize (minWidth, minHeight);
    getConstrainer()->setMaximumSize (maxWidth, maxHeight);
    setSize (defaultWidth, defaultHeight);

    rebuild();
}

UTTEditor::~UTTEditor() = default;

void UTTEditor::rebuild()
{
    webView.reset();
    fallback.reset();

    const auto webviewOptions = juce::WebBrowserComponent::Options()
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2);

    if (juce::WebBrowserComponent::areOptionsSupported (webviewOptions))
    {
        webView = std::make_unique<UTTWebView> (processor, *this);
        addAndMakeVisible (*webView);
        webView->loadInterface();
        webView->setBridgeActive (true);
    }
    else
    {
        // No WebView2 runtime: minimal native panel, audio unaffected
        fallback = std::make_unique<FallbackPanel> (processor);
        addAndMakeVisible (*fallback);
    }
}

void UTTEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffE8E5DB));
}

void UTTEditor::resized()
{
    auto bounds = getLocalBounds();
    if (webView != nullptr)
        webView->setBounds (bounds);
    else if (fallback != nullptr)
        fallback->setBounds (bounds);
}

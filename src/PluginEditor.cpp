#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "gui/UTTWebView.h"
#include "gui/FallbackPanel.h"

namespace
{
constexpr int defaultWidth  = 1000;
constexpr int defaultHeight = 700;
constexpr int minWidth      = 900;
constexpr int minHeight     = 640;
constexpr int maxWidth      = 1400;
constexpr int maxHeight     = 980;
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
    const auto generation = ++webUIGeneration;
    webView.reset();
    fallback.reset();

    auto safeThis = juce::Component::SafePointer<UTTEditor> (this);
    fallback = std::make_unique<FallbackPanel> (processor, "LOADING",
        "WAITING FOR WEBVIEW2 UI",
        [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->rebuild();
        });
    addAndMakeVisible (*fallback);

    webView = std::make_unique<UTTWebView> (processor, *this,
        [safeThis, generation] (UTTWebView::SurfaceState state, const juce::String& reason)
        {
            if (safeThis != nullptr && safeThis->webUIGeneration == generation)
            {
                const auto surfaceState = state == UTTWebView::SurfaceState::unavailable
                    ? "UNAVAILABLE" : "LOAD ERROR";
                safeThis->showFallback (surfaceState, reason);
            }
        },
        [safeThis, generation]
        {
            if (safeThis != nullptr && safeThis->webUIGeneration == generation)
                safeThis->showWebView();
        });
    addChildComponent (*webView);
    webView->setVisible (false);
    webView->loadInterface();

    // setSize() runs before rebuild() in the constructor, so the newly-created
    // child would otherwise keep its default 0x0 bounds until the host resizes.
    resized();
}

void UTTEditor::showWebView()
{
    if (webView == nullptr || ! webView->isReady())
        return;

    fallback.reset();
    webView->setVisible (true);
    resized();
}

void UTTEditor::showFallback (const juce::String& surfaceState, const juce::String& reason)
{
    webView.reset();
    fallback.reset();

    auto safeThis = juce::Component::SafePointer<UTTEditor> (this);
    fallback = std::make_unique<FallbackPanel> (processor, surfaceState, reason,
        [safeThis]
        {
            if (safeThis != nullptr)
                safeThis->rebuild();
        });
    addAndMakeVisible (*fallback);
    resized();
}

void UTTEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffFAFAF8));
}

void UTTEditor::resized()
{
    auto bounds = getLocalBounds();
    if (webView != nullptr)
        webView->setBounds (bounds);
    else if (fallback != nullptr)
        fallback->setBounds (bounds);
}

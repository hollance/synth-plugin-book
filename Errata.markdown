# Errata for Code Your Own Synth Plug-Ins With C++ and JUCE

This is a list of known mistakes and bugs in the book and/or the accompanying source code.

## Automation sounds different in realtime vs. offline rendering

If you add automation to JX11's parameters in your DAW and then render or bounce the track to a file in offline mode, the automation sounds incorrect.

I was a bit too clever with optimizing the way the synth handles changes to the plug-in parameters. Instead of always reading the parameter values, `JX11PluginProcessor`'s `processBlock` does this:

```c++
bool expected = true;
if (parametersChanged.compare_exchange_strong(expected, false)) {
    update();
}
```

The `parametersChanged` boolean is set to true by the APVTS listener whenever any of the plug-in parameters has changed. In response, the `update` method will read all the parameter values and passes them on to the synth.

This works fine in realtime mode, or when offline rendering at 1x speed, but fails when rendering as fast as possible. It turns out that the APVTS listener runs off a timer, and this timer is too slow to keep up with offline rendering.

The solution is to remove the `parametersChanged` check and simply call `update()` every time `processBlock` is called.

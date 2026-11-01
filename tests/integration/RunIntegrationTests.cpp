// FROZEN harness — runs every juce::UnitTest in category "MicroGuitar"; non-zero exit on failure.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;   // processors/editors need the message manager
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTestsInCategory ("MicroGuitar");

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    std::printf ("MicroGuitar integration tests: %d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}

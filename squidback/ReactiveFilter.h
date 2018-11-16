//
// Created by giano on 9/24/18.
//

#ifndef FEEDBACKGENERATOR_REACTIVEFILTER_H
#define FEEDBACKGENERATOR_REACTIVEFILTER_H


#include "SuperpoweredFX.h"
#include <sys/types.h>
#include "SuperpoweredNBandEQ.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "scales.h"
#include <cstdlib>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include "SuperpoweredSimple.h"


struct reactiveFilterInternals;


class ReactiveFilter : public SuperpoweredFX {

public:
    int currentPeakIndex;

    // PARAMETERS
    bool memsetGlitch=false;
    float maxGain;
    float inertia;
    float plasticity;
    float lopass;
    float masterGain;
    float peakThreshold;


    /**
     @brief Sets the spectrum bands for both analysis and filtering.

     @param div The octave division: can be 1,2,3,4,6,12,24,48 (equal-temperament) or 7,13,21,43 (just-intonation).
    */
    void setFilterBands(int div);

/**
 @brief Turns the effect on/off.
 */
    void enable(bool flag);

    void reset();

    void setSamplerate(unsigned int samplerate);

/**
 @brief Create an eq instance.

 Enabled is false by default, use enable(true) to enable. Example: SuperpoweredNBandEQ eq = new SuperpoweredNBandEQ(44100, ...);

 @param samplerate 44100, 48000, etc.
 @param frequencies 0-terminated list of frequency bands.
*/
    ReactiveFilter(unsigned int samplerate);
    ~ReactiveFilter();


/**
 @brief Processes the audio.

 It's not locked when you call other methods from other threads, and they not interfere with process() at all.

 @return Put something into output or not.

 @param input 32-bit interleaved stereo input buffer. Can point to the same location with output (in-place processing).
 @param output 32-bit interleaved stereo output buffer. Can point to the same location with input (in-place processing).
 @param numberOfSamples Should be 32 minimum.
*/
    bool process(float *input, float *output, unsigned int numberOfSamples);


    int getNumBands();
    float *getBands();
    float *getFilterDbs(bool applyMasterGain);
    void adjustLopass(float newLopass, bool clear);


        private:
    reactiveFilterInternals *internals;
    reactiveFilterInternals *nextInternals;


    /*static SuperpoweredNBandEQ *correctionFilter;*/


    ReactiveFilter(const ReactiveFilter&);
    ReactiveFilter& operator=(const ReactiveFilter&);

    /**
    @brief Clears all the internals params and filters.
    */

    void updateInternals();

    /**
    @brief Adapts a filter configuration to the present settings.

     @param oldBands frenquencies of the old bands.
     @param oldDecibels filter gains for the old bands.
     @param oldNumBands size of oldBands.
    */
    void adaptFilter(float *oldBands, float *oldDecibels, int oldNumBands);

    void setFilterPrecision(int div,reactiveFilterInternals *newInternals);
    void setFilterET(int div,reactiveFilterInternals *newInternals);
    void setFilterPartch(int div, reactiveFilterInternals *newInternals);

    float calcAcceleration(int i,float ampDb);
    void updateFilterAvgPeakNoGain();

};


#endif //FEEDBACKGENERATOR_REACTIVEFILTER_H

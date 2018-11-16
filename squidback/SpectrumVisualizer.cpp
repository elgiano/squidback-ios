//
// Created by giano on 9/15/18.
//

#include "SuperpoweredBandpassFilterbank.h"
#include <algorithm>
#include <string.h>
#include "scales.h"

static SuperpoweredBandpassFilterbank *visualFilterbank;
static float *visualMagnitudes;
static int numVisualBands;
static float *visualBands;
static float *visualBw;
static int cyclesSinceLastVisual = 0;


static void initVisualizer(int octaveDivision, int sampleRate){

    numVisualBands = (int) 448/(48/octaveDivision);

    visualBw  = new float[numVisualBands];
    std::fill(visualBw,visualBw+numVisualBands, 1.0f/octaveDivision);

    visualBands = new float[numVisualBands];
    for(int i=0;i<numVisualBands;i+=1){
        visualBands[i] = octToneScale[i*48/octaveDivision];
    }

    visualMagnitudes = new float[numVisualBands];

    visualFilterbank = new SuperpoweredBandpassFilterbank(numVisualBands,visualBands,visualBw,(unsigned int)sampleRate);

    cyclesSinceLastVisual = 0;


}

static void processVisualizer(float *floatBuffer, unsigned int numberOfFrames){
    float p,s;
    cyclesSinceLastVisual +=1;
    visualFilterbank->process(floatBuffer,visualMagnitudes,&p,&s,numberOfFrames);

}

static float *getVisualSpectrum(){

    // copy and reset
    int cycles = cyclesSinceLastVisual;

    float *visualMagCopy = new float[numVisualBands];
    memcpy(visualMagCopy,visualMagnitudes,sizeof(float)*numVisualBands);

    memset(visualMagnitudes,0,sizeof(float)*numVisualBands);
    cyclesSinceLastVisual = 0;

    // average frames since last poll
    for(int i=0;i<numVisualBands;i++){
        visualMagCopy[i] = visualMagCopy[i]/cycles;
    }

    return visualMagCopy;
}

static void destroyVisualizer(){
    delete visualFilterbank;
    free(visualMagnitudes);
    free(visualBands);
    free(visualBw);
}

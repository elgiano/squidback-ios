#import "Squidback-Bridging-Header.h"
#import "SuperpoweredIOSAudioIO.h"
#include "SuperpoweredBandpassFilterbank.h"
#include "SuperpoweredSimple.h"

#include "SpectrumVisualizer.cpp"
#include "ReactiveFilter.h"
#include "ReactiveFilterController.h"



@implementation Superpowered {
    SuperpoweredIOSAudioIO *audioIO;
    ReactiveFilter *reactiveFilter;
    
    unsigned int samplerate;
    float fade;
    bool fadeIn;
    bool fadeOut;
}


static bool audioProcessing(void *clientdata, float **buffers, unsigned int inputChannels, unsigned int outputChannels, unsigned int numberOfSamples, unsigned int samplerate, uint64_t hostTime) {
    __unsafe_unretained Superpowered *self = (__bridge Superpowered *)clientdata;
    if (samplerate != self->samplerate) {
        self->samplerate = samplerate;
        visualFilterbank->setSamplerate(samplerate);
        self->reactiveFilter->setSamplerate(samplerate);
    };
    

    // Mix the non-interleaved input to interleaved.
    float interleaved[numberOfSamples * 2 + 16];
    
    SuperpoweredInterleave(buffers[0], buffers[1], interleaved, numberOfSamples);
    
    processVisualizer(interleaved, numberOfSamples);

    
    self->reactiveFilter->process(interleaved, interleaved, numberOfSamples);
    
    // fade
    if(self->fadeIn || self->fadeOut){
        float oldFade = self->fade;
        if(self->fadeIn){
            self->fade += 0.005;
            if(self->fade>=1){
                self->fade = 1;
                self->fadeIn = false;
            }
        }else if(self->fadeOut){
            self->fade -= 0.001;
            if(self->fade<=0){
                self->fade = 0;
                //self->fadeOut = false;
                [self->audioIO stop];
                destroyVisualizer();
                delete self->reactiveFilter;
            }

        }
        
        SuperpoweredVolume(interleaved,interleaved,oldFade,self->fade,numberOfSamples);
        
    }
    
    SuperpoweredDeInterleave(interleaved, buffers[0], buffers[1], numberOfSamples);
    
    return true;
}

- (id)init {
    self = [super init];
    if (!self) return nil;
    samplerate = 44100;
    

        audioIO = [[SuperpoweredIOSAudioIO alloc] initWithDelegate:(id<SuperpoweredIOSAudioIODelegate>)self preferredBufferSize:12 preferredSamplerate:44100 audioSessionCategory:AVAudioSessionCategoryPlayAndRecord channels:2 audioProcessingCallback:audioProcessing clientdata:(__bridge void *)self];
    
    return self;
}

-(bool) isPlaying{
    if (audioIO){
        return audioIO.started;
    }else{
        return false;
    }
}
-(void)initAudio{
    initVisualizer(12, samplerate);
    
    reactiveFilter = new ReactiveFilter((unsigned) samplerate);
    reactiveFilter->setFilterBands(12); // TODO: remove
    reactiveFilter->enable(true);
    
    fade = 0;
    fadeIn = true;
    fadeOut = false;

    
    [audioIO start];
}

- (void) stopAudio{
    fadeOut = true;
    fadeIn = false;
    /*[audioIO stop];
    destroyVisualizer();
    delete reactiveFilter;*/
}

- (void)dealloc {
    //[self stopAudio];
    [audioIO stop];
    destroyVisualizer();
    delete reactiveFilter;
    
    audioIO = nil;
}

-(float)getFade{
    return self->fade;
}

- (void)interruptionStarted {}
- (void)interruptionEnded {}
- (void)recordPermissionRefused {}
- (void)mapChannels:(multiOutputChannelMap *)outputMap inputMap:(multiInputChannelMap *)inputMap externalAudioDeviceName:(NSString *)externalAudioDeviceName outputsAndInputs:(NSString *)outputsAndInputs {}

/*
 It's important to understand that the audio processing callback and the screen update (getFrequencies) are never in sync. 
 More than 1 audio processing turns may happen between two consecutive screen updates.
 */

- (void)getSpectrum:(float *)freqs {
    memcpy(freqs,getVisualSpectrum(),numVisualBands*sizeof(float));
    //memcpy(freqs,reactiveFilter->getAnalMagnitudes(),reactiveFilter->getNumBands()*sizeof(float));

}
- (int)getNumVisualBands {
    return numVisualBands;
    //return reactiveFilter->getNumBands();
}
- (void)getSpectrumFrequencies:(float *)freqs {

    memcpy(freqs,visualBands,numVisualBands*sizeof(float));
    //memcpy(freqs,reactiveFilter->getBands(),reactiveFilter->getNumBands()*sizeof(float));

}

- (void)getFilterDb:(float *)freqs {
    memcpy(freqs,reactiveFilter->getFilterDbs(false),reactiveFilter->getNumBands()*sizeof(float));
}
- (void)getCorrectionDb:(float *)freqs{
    int numBands = reactiveFilter->getNumBands();
    /*std::vector<float> correction = reactiveFilter->controller->getPersistentPeakCorrectionNoGain();
    for(int i=0;i<numBands && i<correction.size();i++){
        freqs[i] = correction[i];
    };*/
    memcpy(freqs,reactiveFilter->controller->getPersistentPeakCorrectionNoGainPointer(),sizeof(float)*numBands);
}

- (int)getNumFilterBands {
    return reactiveFilter->getNumBands();
}
- (void)getFilterFrequencies:(float *)freqs {
    
    memcpy(freqs,reactiveFilter->getBands(),reactiveFilter->getNumBands()*sizeof(float));
}
-(float) getMasterGain{
    if(reactiveFilter) {
        return reactiveFilter->masterGain;
    }else{
        return 0;
    }
}
-(float) setMaxGain:(float) perc{
    if(reactiveFilter) {
        reactiveFilter->maxGain = perc*98+2;
        reactiveFilter->adjustLopass(-1, true);
        return reactiveFilter->maxGain;
    }else{
        return 0;
    }
}
-(void) setPlasticity:(float) perc{
    if(reactiveFilter) reactiveFilter->plasticity = (float) pow(10000000,perc)*0.01f;
}
-(void) setInertia:(float) perc{
    if(reactiveFilter) reactiveFilter->inertia = (float) pow(1/0.001,1-perc)*0.001f -0.001f;;
}
-(void) setPeakThr:(float) perc{
    if(reactiveFilter) reactiveFilter->peakThreshold = (1-perc)*(-30);
}

-(void) setFilterBw:(int) perc{
    // int perc must go from 0 to 11
    if(perc >= 0 && perc< 12 ){
        //clearFilter();
        if(reactiveFilter) reactiveFilter->setFilterBands(filterPrecisions[perc]);
    }
}

-(void) setLopass: (float) perc{
    float freq = (float) pow(22000/5000,(perc))*5000;
    //std::cout << "hipass: " << freq << std::endl;
     if(reactiveFilter) reactiveFilter->adjustLopass( freq, true);
}
-(void) setHipass: (float) perc{
    
    float freq = (float) pow(1000/20,(perc))*20;
    //std::cout << "hipass: " << freq << std::endl;
    if(reactiveFilter) reactiveFilter->adjustHipass( freq, true);
}
-(void) setMemsetGlitch: (bool) sw{
    if(reactiveFilter) reactiveFilter->memsetGlitch = sw;

}

-(float) getPeakness{
    if(reactiveFilter) return reactiveFilter->peakness;
    return 0;
}
-(float) getPeak{
    if(reactiveFilter) return reactiveFilter->peakThreshold;
    return 0;
}
-(float) getAverage{
    if(reactiveFilter) return reactiveFilter->average;
    return 0;
}
-(float) getLimiterCorrection{
    if(reactiveFilter) return reactiveFilter->limiterCorrection;
    return 0;
}
    
-(void) printTrackings{
    if(reactiveFilter) reactiveFilter->controller->printAll();
}

@end

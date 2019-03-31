//
// Created by giano on 9/24/18.
//


#include "ReactiveFilter.h"
#include "ReactiveFilterController.h"



typedef struct reactiveFilterInternals {

    unsigned int analNumBands = 0;
    float *analBands = NULL;
    float *analWidths = NULL;
    float *analMagnitudes = NULL;
    float *gainSpeeds = NULL;
    float *lastCycleGain = NULL;
    unsigned int newSamplerate = 0;
    SuperpoweredNBandEQ *filter = NULL;
    SuperpoweredBandpassFilterbank *analFilterbank = NULL;

} reactiveFilterInternals;

/* UTILS */

float ampdb(float amp){
    return amp>0 ? 20*(float)log10(amp) : -80;
}
float dbAmp(float amp){
    return (float) pow(10,amp/20);
}

unsigned indexOfClosestTo( float* nums, unsigned nNums, float target )
{
    unsigned indexOfClosestTo = 0 ;

    unsigned index = 1 ;
    while ( index < nNums )
    {
        if ( fabs(target-nums[index]) < fabs(target-nums[indexOfClosestTo]) )
            indexOfClosestTo = index ;

        ++index ;
    }

    return indexOfClosestTo ;
}

reactiveFilterInternals *getNewInternals(){
    reactiveFilterInternals *newInternals = new reactiveFilterInternals;
    newInternals->analNumBands = 0;
    newInternals->analBands = NULL;
    newInternals->analWidths = NULL;
    newInternals->analMagnitudes = NULL;
    newInternals->gainSpeeds = NULL;
    newInternals->lastCycleGain = NULL;
    newInternals->newSamplerate = 0;

    return newInternals;
};

void freeInternals(reactiveFilterInternals *toFree){
    delete toFree->analBands;
    delete toFree->analWidths;
    delete toFree->analMagnitudes;
    delete toFree->gainSpeeds;
    delete toFree->lastCycleGain;
    delete toFree;

};

/* CONSTRUCTOR */

ReactiveFilter::ReactiveFilter(unsigned int samplerate) {

    internals = getNewInternals();
    nextInternals = NULL;

    internals->newSamplerate = samplerate;
    enabled = false;
    currentPeakIndex = -1;
    masterGain = 0;

    //init parameters
    lopass = 15000;
    hipass = 20;
    maxGain = 30;
    plasticity = 1000;
    peakThreshold = 0.1;
    inertia = 0.001;
    
    // initialize limiter
    limiter = new SuperpoweredLimiter((unsigned int)samplerate);
    limiter->enable(true);
    
    controller = new ReactiveFilterController();
    controller->target = this;

}

/* DESTRUCTOR */

ReactiveFilter::~ReactiveFilter() {
    if(internals!=NULL) freeInternals(internals);
    if(nextInternals!=NULL) freeInternals(nextInternals);
    delete limiter;

}

void ReactiveFilter::reset(){

}

void ReactiveFilter::setSamplerate(unsigned int samplerate) {
    // This method can be called from any thread. Setting the sample rate of all filters must be synchronous, in the audio processing thread.
    internals->newSamplerate = samplerate;
    limiter->setSamplerate(samplerate);

}

/* ACCESSORS */

int ReactiveFilter::getNumBands() {
    if(!internals->analBands) return 0;
    return internals->analNumBands;
}

float *ReactiveFilter::getBands() {
    if(internals->analBands) return internals->analBands;
    return (new float[1]);
}

float *ReactiveFilter::getFilterDbs(bool applyMasterGain) {
    if(internals->filter){
        if(!applyMasterGain) return internals->filter->decibels;

        float *filterDb = new float[internals->analNumBands];
        for(int i = 0; i<internals->analNumBands;i++){
            filterDb[i] = internals->filter->decibels[i]+masterGain;
        }
        return filterDb;
    }else{
        return (new float[10]);
    }
}

float ReactiveFilter::getTotalCorrection(bool removePersistent) {
    if(internals->filter){
        
        float filterDb = 0;
        for(int i = 0; i<internals->analNumBands;i++){
            filterDb += internals->filter->decibels[i];
        }
        if(removePersistent){
            std::vector<float> corrections = controller->getPersistentPeakCorrectionNoGain();
            filterDb += std::accumulate(corrections.begin(), corrections.end(), 0.0f);
        }

        return filterDb;
    }else{
        return 0;
    }
}

void ReactiveFilter::adjustLopass(float newLopass, bool clear){
    float oldLopass = lopass;

    if(newLopass>0){
        lopass = newLopass;
    }

    if(clear && internals->filter) {
        // clear bands between old and new lopass
        for (unsigned int i = 0; i < internals->analNumBands; i++) {
            if (internals->analBands[i] >= oldLopass && internals->analBands[i] <= lopass) {
                internals->filter->setBand(i, 0);
            }
        }
    }
}

void ReactiveFilter::adjustHipass(float newHipass, bool clear){
    float oldHipass = hipass;
    
    if(newHipass>0){
        hipass = newHipass;
    }
    
    if(clear && internals->filter) {
        // clear bands between old and new hipass
        for (unsigned int i = 0; i < internals->analNumBands; i++) {
            if (internals->analBands[i] <= oldHipass && internals->analBands[i] >= hipass) {
                internals->filter->setBand(i, 0);
            }
        }
    }
    
}

/* BANDS SETTINGS */

void ReactiveFilter::incrementBand(int i, float val){
    internals->filter->setBand(i, internals->filter->decibels[i] + val);
}

float ReactiveFilter::getBand(int i){
    if(internals->analBands && i< internals->analNumBands) return internals->analBands[i];
    return 0;
}


// Now setFilterBands creates a new internal struct
// nextInternals will be swapped with internals synchronously in the audio thread

void ReactiveFilter::setFilterBands(int div){

    reactiveFilterInternals *newInternals = getNewInternals();

    newInternals->newSamplerate = internals->newSamplerate;

    setFilterPrecision(div,newInternals);

    newInternals->analMagnitudes = new float[newInternals->analNumBands];
    newInternals->gainSpeeds = new float[newInternals->analNumBands];
    newInternals->lastCycleGain = new float[newInternals->analNumBands];

    memset(newInternals->analMagnitudes,0,sizeof(float)*newInternals->analNumBands);
    // memset glitch: don't clean up -> get random stuff in there
    if(!memsetGlitch){
        memset(newInternals->gainSpeeds,0,sizeof(float)*newInternals->analNumBands);
        memset(newInternals->lastCycleGain,0,sizeof(float)*newInternals->analNumBands);
    }


    // init anal filter
    newInternals->analFilterbank = new SuperpoweredBandpassFilterbank(newInternals->analNumBands,newInternals->analBands,newInternals->analWidths,newInternals->newSamplerate);

    // init filter
    newInternals->filter = new SuperpoweredNBandEQ(newInternals->newSamplerate, newInternals->analBands);
    //correctionFilter = new SuperpoweredNBandEQ((unsigned int) sampleRate, analBands);
    newInternals->filter->reset();
    newInternals->filter->enable(true);


    //filter->enable(true);
    //correctionFilter->enable(true);
    //pthread_mutex_lock( &filterMutex );
    nextInternals = newInternals;
    //pthread_mutex_unlock( &filterMutex );
    


}

void ReactiveFilter::setFilterET(int div,reactiveFilterInternals *newInternals){

    //__android_log_print(ANDROID_LOG_INFO, "ET", "div: %d", div);

    newInternals->analNumBands = (unsigned) 448 * div/48;

    newInternals->analWidths = new float[newInternals->analNumBands];
    std::fill(newInternals->analWidths,newInternals->analWidths+newInternals->analNumBands, 1.0f/div);

    newInternals->analBands = new float[newInternals->analNumBands+1];
    for(int i=0;i<newInternals->analNumBands;i+=1){
        newInternals->analBands[i] = octToneScale[i*48/div];
    }
    newInternals->analBands[newInternals->analNumBands] = 0;
}

void ReactiveFilter::setFilterPartch(int div,reactiveFilterInternals *newInternals){

    //__android_log_print(ANDROID_LOG_INFO, "partch", "div: %d", div);

    float *scale;
    switch(div){
        case 48:
            newInternals->analNumBands = 400;
            newInternals->analWidths = new float[newInternals->analNumBands];
            scale = partchScale;
            div = 43;
            break;
        case 24:
            newInternals->analNumBands = 288;
            newInternals->analWidths = new float[newInternals->analNumBands];
            scale = partchScale7;
            div = 31;
            break;
        case 12:
            newInternals->analNumBands = 176;
            newInternals->analWidths = new float[newInternals->analNumBands];
            scale = partchScale5;
            div = 19;
            break;
        case 6:
            newInternals->analNumBands = 64;
            newInternals->analWidths = new float[newInternals->analNumBands];
            scale = partchScale3;
            div = 7;
            break;
        default: return setFilterET(1,newInternals);
    }

    newInternals->analBands = new float[newInternals->analNumBands+1];
    memcpy(newInternals->analBands,scale,sizeof(float)*newInternals->analNumBands);
    newInternals->analBands[newInternals->analNumBands] = 0;

    // log2f is broken in Android, so we use log(x) / log(2)
    const float log2fdiv = 1.0f / logf(2.0f);

    for(int i=0;i<newInternals->analNumBands;i+=1){
        newInternals->analWidths[i]  =
                (newInternals->analBands[i + 1] > newInternals->analBands[i]) ?
                logf(newInternals->analBands[i + 1] / newInternals->analBands[i]) :
                logf(20000.0f / newInternals->analBands[i]);
        newInternals->analWidths[i] *= log2fdiv;
        newInternals->analWidths[i] = 1.0f/div;

    }
}

// Dispatch between ET and JI

void ReactiveFilter::setFilterPrecision(int div, reactiveFilterInternals *newInternals){

    //__android_log_print(ANDROID_LOG_INFO, "filterPrec", "div: %d", div);

    switch(div){
        case 48: setFilterET(48,newInternals);break;
        case 43: setFilterPartch(48,newInternals);break;
        case 31: setFilterPartch(24,newInternals);break;
        case 24: setFilterET(24,newInternals);break;
        case 19: setFilterPartch(12,newInternals);break;
        case 12: setFilterET(12,newInternals);break;
        case 7: setFilterPartch(6,newInternals);break;
        case 6:
        case 4:
        case 3:
        case 2: setFilterET(div,newInternals);break;
        default: return setFilterET(1,newInternals);
    }
}

void ReactiveFilter::adaptFilter(float *oldBands, float *oldDecibels, int oldNumBands){
    // for each old bands, find the most similar new band
    float *newGains = new float[internals->analNumBands];
    int *averages = new int[internals->analNumBands];

    for(int i=0;i<oldNumBands;i++){
        float f0 = oldBands[i];
        float f1;
        if((i+1)<oldNumBands){
            f1 = oldBands[i];
        }else{
            f1 = 20000.0f;
        }
        // find how many new freqs fall in the interval
        int nf0_i = indexOfClosestTo(internals->analBands, internals->analNumBands,f0);
        int nf1_i = indexOfClosestTo(internals->analBands, internals->analNumBands,f1);
        for(int j=nf0_i;j<=nf1_i;j++){
            newGains[j] += oldDecibels[i];
            averages[j]++;
        }

    }
    for(unsigned i=0;i<internals->analNumBands;i++){
        if(averages[i]>0)
            internals->filter->setBand(i,newGains[i]);
    }
}



void ReactiveFilter::enable(bool flag) {
    // This method can be called from any thread. Switching all filters must be synchronous, in the audio processing thread.
    enabled = flag;
}

// update internals from nextInternals, adapt and free the old pointers
void ReactiveFilter::updateInternals() {
    if(nextInternals!=NULL){
        reactiveFilterInternals *tmp = internals;
        internals = nextInternals;
        nextInternals = NULL;
        // adapt old configuration if present
        if(tmp!=NULL && tmp->filter!=NULL){
            adaptFilter(tmp->analBands,tmp->filter->decibels,tmp->analNumBands);
        }
        freeInternals(tmp);
        lastPeakCorrections = controller->getPersistentPeakCorrectionNoGain();

    }
}

// AUDIO PROCESS

bool ReactiveFilter::process(float *input, float *output, unsigned int numberOfSamples) {

    if (!input || !output || !numberOfSamples ) return false; // Some safety.

    updateInternals();

    // Change the sample rate of all filters at once if needed.
    /*unsigned int newSamplerate = __sync_fetch_and_and(&internals->newSamplerate, 0);
    if (newSamplerate > 0) {
        internals->analFilterbank->setSamplerate(newSamplerate);
        internals->filter->setSamplerate(newSamplerate);
    }*/

    if (!internals->filter || !internals->analFilterbank ) return false; // Some safety.
    
    controller->trackValue("inVolDb", ampdb(SuperpoweredPeak(input, numberOfSamples)));

    // anal here: get bands magnitude
    float peak,sum;
    internals->analFilterbank->processNoAdd(input,internals->analMagnitudes,&peak,&sum,numberOfSamples);
    //correctExpectations();
    

    // update filter and process input->output
    updateFilterAvgPeakNoGain();

    SuperpoweredVolumeAdd(input,output,dbAmp(masterGain),dbAmp(masterGain), numberOfSamples);
    internals->filter->process(output,output, numberOfSamples);
    
    // limit and out
    limiter->process(output, output, numberOfSamples);
    
    limiterCorrection = limiter->getGainReductionDb();

    controller->trackValue("outVolDb", ampdb(SuperpoweredPeak(output, numberOfSamples)));
    controller->trackValue("averageDb", average);
    //controller->trackValue("peakDb", this->peak);
    //controller->trackValue("peakness", peakness);
    //controller->trackValue("limiterCorrection", limiterCorrection);
    //controller->trackValue("filterVariation", filterVariation);

    if(internals->analMagnitudes[currentPeakIndex] > peakThreshold)
        controller->trackValue("peakI", currentPeakIndex);

    //std::cout << limiterCorrection << std::endl;
    /*auto now = std::chrono::system_clock::now();
    std::cout << (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastUpdate)).count() << std::endl;
    lastUpdate = now;*/

    controller->adjustControls();
    //controller->printAll();
    return true;

}


// FILTER BEHAVIOUR

float ReactiveFilter::calcAcceleration(int i,float ampDb){
    float oldSpeed = internals->gainSpeeds[i];
    float thisSpeed = ampDb - internals->lastCycleGain[i];
    float acceleration = thisSpeed - oldSpeed;


    acceleration *= inertia;

    internals->gainSpeeds[i] = oldSpeed + acceleration;
    internals->lastCycleGain[i] = internals->lastCycleGain[i] + (oldSpeed + acceleration);

    return internals->lastCycleGain[i];
}

void ReactiveFilter::updateFilterAvgPeakNoGain() {

    float newGain,averageDb=0;
    float totGain = 0;
    float numGainedBands = 0;
    
    float variation = 0;

    // detect peak
    float *peakDb = std::max_element(internals->analMagnitudes, internals->analMagnitudes+internals->analNumBands);
    float *antipeakDb = std::min_element(internals->analMagnitudes, internals->analMagnitudes+internals->analNumBands);
    currentPeakIndex = (unsigned int) std::distance(
            internals->analMagnitudes,
            peakDb
    );

    // calc average dbs
    /*for(unsigned int i=0; i<internals->analNumBands; i++){
        averageDb += internals->analMagnitudes[i];
    };*/
    averageDb = ampdb(*peakDb)/2+ampdb(*antipeakDb)/2 ;//ampdb(averageDb/internals->analNumBands);
    
    // calc peakness
    peak = ampdb(*peakDb);
    average = averageDb;
    peakness = dbAmp(peak)/dbAmp(averageDb);
    
    std::vector<float> corrections = controller->getPersistentPeakCorrectionNoGain();

    for(unsigned int i=0; i<internals->analNumBands; i++){

        // lowpass-hipass
        if(internals->analBands[i]>=lopass || internals->analBands[i]<=hipass ){
            newGain = -maxGain-80;
        }else{
        
            float magDb = ampdb(internals->analMagnitudes[i]);

            float peakDelta = peakThreshold/*std::max(averageDb,peakThreshold)*/ - magDb;
            float avgDelta = std::min(averageDb,peakThreshold) - magDb;


            //newGain = filter->decibels[i] + (float) (sign(peakDelta+avgDelta)*sqrt(pow(peakDelta,2)+pow(avgDelta,2))/plasticity) ;
            newGain =  peakDelta;//+avgDelta;// * (peakDelta*avgDelta>0?1:-1);//(peakDelta+avgDelta) ;
            //if(peakDelta*avgDelta>0 && newGain > 0){ newGain *= -1;}
            //if(magDb<=avgDelta){newGain *= -1;}

            //float inertiaGain = calcAcceleration(i,newGain);
            //newGain = inertiaGain;

            //newGain = internals->filter->decibels[i] + (newGain/plasticity);
            newGain = (newGain-internals->filter->decibels[i]);
            
            //newGain /= /*(newGain>=0?inertia:plasticity)*/(5000/abs(corrections[i]+0.01))/**(newGain>=0?inertia/plasticity:plasticity/inertia)*/;
            newGain /= (newGain>=0?(pow(2,abs(corrections[i]+0.01))):(100000/pow(2,abs(corrections[i]+0.01))));
            
            
            newGain += internals->filter->decibels[i];
            
        
            // lowest limit
            if(newGain<-(maxGain+80)){newGain = -(maxGain+80);}
        
        
            // don't give gain
            if(newGain>0){
                totGain += newGain;
                numGainedBands += 1;
                newGain=0;
            }
        
            // apply persistent correction from controller
            newGain += (corrections[i]-lastPeakCorrections[i]);
            
            // NaN filter
            if(isnan(newGain)){newGain = 0;}
        }
        // don't give gain
        if(newGain>corrections[i]){
            newGain=corrections[i];
        }
        
        variation += abs(newGain - internals->filter->decibels[i]);

        internals->filter->setBand(i, newGain);


    };

    // masterGain corrections:
    // raise masterGain to help freqs below the average
    if(numGainedBands>0) {
        totGain = totGain / numGainedBands ;
        //masterGain += totGain;
        //masterGain += peakThreshold - ampdb(peakLevel);

    }else{
        //masterGain += peakThreshold - averageDb / plasticity;
        if(masterGain<0){masterGain=0;}
    }
    if(masterGain>maxGain){masterGain = maxGain;}
    
    lastPeakCorrections = corrections;
    filterVariation = variation;
    

}

float *ReactiveFilter::getAnalMagnitudes(){
    return internals->analMagnitudes;
};


//
//  ReactiveFilterController.cpp
//  squidback
//
//  Created by Gnlc Elia on 28/11/2018.
//  Copyright © 2018 imect. All rights reserved.
//

#include "ReactiveFilterController.h"
#include "ReactiveFilter.h"

const int ReactiveFilterController::MaxTrackedValues = 1000;

void ReactiveFilterController::trackValue(std::string name,float value){
    if(std::find(trackingKeys.begin(), trackingKeys.end(), name) == trackingKeys.end()){
        trackingKeys.push_back(name);
    }
    trackings[name].push_back(value);
    
    if(trackings[name].size()>ReactiveFilterController::MaxTrackedValues){
        trackings[name].erase(trackings[name].begin());
    }

}

float ReactiveFilterController::average(std::string name, int n_values){
    if(n_values==0 || n_values > trackings[name].size()) n_values = (int) trackings[name].size();
    
    float sum = std::accumulate(trackings[name].rbegin(),trackings[name].rbegin()+n_values,0.0f);
    
    return sum/(float)n_values;
};

std::map<float,int> ReactiveFilterController::count(std::string name, int n_values){
    if(n_values==0 || n_values > trackings[name].size()) n_values = (int) trackings[name].size();
    
    std::map<float,int> counts;
    std::for_each(trackings[name].rbegin(),trackings[name].rbegin()+n_values, [&counts](float &n){
        counts[n]++;
    });
    
    return counts;
};


void ReactiveFilterController::printAll(){
    // t = 11ms, 10t = 0.11s, 100t = 1.1s, 1000t = 11s
    for(auto& k: trackingKeys){
        std::cout << k << ": " << average(k,10) << "/" << average(k,100) << "/" << average(k) << "\t";
    }
    std::cout << "peakThr: " << target->peakThreshold;
    std::cout << std::endl;
}


void ReactiveFilterController::adjustControls(){
    
    float avg = average("averageDb",100);
    float avgT = average("averageDb");
    
    float peak = average("peakDb",100);
    float peakT = average("peakDb");
    //float peakness = average("peakness",100);
    float peakThr = target->peakThreshold;
    float inVolDb = average("inVolDb",100);
    float outVolDb = average("outVolDb",100);
    float outVolDbAvg = average("outVolDb");
    float filterVariation = average("filterVariation",100);

    float limiterCorrection = average("limiterCorrection",10);
    //std::map<float,int> peaks = count("peakI");
    std::map<float,int> recentPeaks = count("peakI",500);
    std::map<float,int> mostRecentPeaks = count("peakI",100);

    
    int numPeaks = recentPeaks.size();
    trackValue("numPeaks", numPeaks);
    float numPeaksVariation = abs(1-numPeaks/average("numPeaks"));
    
    
    // raise gain to push out to -10
    float desiredDb = -6;
    float gainMaximize = ((outVolDb-desiredDb) + (outVolDbAvg-desiredDb)/2) * -0.001;
    // adjust gain to avoid saturation
    //float gainLimiter = (limiterCorrection * 4 - 1) ;
    //gainLimiter = pow(gainLimiter,5)* 0.1;
    //std::cout << gainMaximize << "\t" << gainLimiter << std::endl;
    
    float inGain = (inVolDb-outVolDb) * 0.01;
    if(inGain<0) inGain = 0;
    
    float peakExcess = peakThr-peak;
    float dPeak = peak/peakT;
    if(dPeak==0)dPeak+=0.01;

    
    target->masterGain += gainMaximize +inGain; // + gainLimiter;// + (peakExcess*0.001);
    //std::cout << "g: " << (gainMaximize +inGain + gainLimiter) << ": "<< gainMaximize  << " " << inGain << " " << inVolDb << " " << outVolDb << " " << std::endl;
    
    
    //__android_log_print(ANDROID_LOG_INFO,"peak","%f %f",peakExcess,dPeak);
    
    
    target->peakThreshold = (avgT*2+avg)/3;//avgT + (-avg/2);
    target->peakThreshold =  std::max(target->peakThreshold, -6.0f);//avgT * 0.25;// + (peakExcess/dPeak*0.01);
    //std::cout << "p: "<<peak << ": "<< (peak-avgT/2) << " a: " << avgT << std::endl;
    
    //std::cout << peakThr << std::endl;
    //__android_log_print(ANDROID_LOG_INFO,"peakThr","%f %f",peakness,target->peakThreshold);
    
    
    float totalCorrection = -target->getTotalCorrection(true);
    
    // plasticity: numPeaks
    //target->plasticity = 500*pow(2,abs(std::min(abs(5-numPeaks),abs(15-numPeaks))));
    //std::cout << numPeaks << ": " << target->plasticity << std::endl;

    // plasticity: totalCorrection
    //target->plasticity = (10000 * exp(-1*pow(totalCorrection-30,2)/(2*pow(10,2))))+100;
    //std::cout << totalCorrection << ": " << target->plasticity << std::endl;
    
    // plasticity: filterVariation
    //filterVariation = 6*pow(filterVariation,4) - 3*pow(filterVariation,6);
    //if(filterVariation<=0) filterVariation = 1;

    
    /*target->plasticity = 10000*
        (mapToCurve(abs(7-(numPeaks%15))/8.0,0.01,1,-4)+0.1);
    if(target->plasticity<=100) target->plasticity = 100;
    
    target->inertia = target->plasticity * log10(300/totalCorrection);
    if(target->inertia<=100) target->inertia = 100;
    std::cout << totalCorrection << ": " << target->inertia << ": "<< target->plasticity << std::endl;
    //std::cout << filterVariation << ": " << numPeaksVariation << ": "<< numPeaks << ": " << mapToCurve(abs(7-(numPeaks%15))/8.0,0.01,1,-4) << ": "<< target->plasticity << std::endl;*/
    std::cout << filterVariation << std::endl;
    filterVariation = mapToCurve(filterVariation/2, 0, 1, -2);
    
    totalCorrection /= target->getNumBands();
    
    target->plasticity = 10000 * (mapToCurve(totalCorrection/20.0, 0.001, 1, 3)) * pow(1.5,numPeaks) * filterVariation;
    if(target->plasticity<=100) target->plasticity = 100;
    
    target->inertia = 100.0 / mapToCurve(totalCorrection/20.0, 0.05, 1, 5) * filterVariation;
    if(target->inertia<=100) target->inertia = 100;
    std::cout << filterVariation <<" "<< totalCorrection << ": " << target->inertia << ": "<< target->plasticity  << std::endl;
    //std::cout << filterVariation << ": " << numPeaksVariation << ": "<< numPeaks << ": " << mapToCurve(abs(7-(numPeaks%15))/8.0,0.01,1,-4) << ": "<< target->plasticity << std::endl;
    
    
    auto most_persistent_peak = std::max_element(mostRecentPeaks.begin(), mostRecentPeaks.end(),
                                                 [](const std::pair<float, int>& p1, const std::pair<float, int>& p2) {
                                                     return p1.second < p2.second; });
    
    peakRegister[target->getBand(most_persistent_peak->first)] += 0.008;
    
    
    /*for(auto& p: peaks){
     target->incrementBand(p.first,p.second/(-500));
     }*/
    

    
}

float ReactiveFilterController::mapToCurve(float val,float min,float max,float curve){

    float grow = exp(curve);
    
    float a = abs(max-min) / (1.0 - grow);
    float b = min + a;
    
    return b - (a * pow(grow, val));
}


std::vector<float> ReactiveFilterController::getPersistentPeakCorrection(){
    std::vector<float> correction;
    int numBands = target->getNumBands();
    float *bands = target->getBands();
    for(int i=0;i<numBands;i++){
        correction.push_back(target->masterGain-peakRegister[bands[i]]);
    }
    
    return correction;
}

std::vector<float> ReactiveFilterController::getPersistentPeakCorrectionNoGain(){
    std::vector<float> correction;
    int numBands = target->getNumBands();
    float *bands = target->getBands();
    for(int i=0;i<numBands;i++){
        correction.push_back(-peakRegister[bands[i]]);
    }
    
    return correction;
}

float* ReactiveFilterController::getPersistentPeakCorrectionNoGainPointer(){
    int numBands = target->getNumBands();
    float *bands = target->getBands();
    if(targetNumBands != numBands){
        free(targetCorrection);
        targetCorrection = (float *) malloc(numBands * sizeof(float));
    }
    for(int i=0;i<numBands;i++){
        targetCorrection[i] = -peakRegister[bands[i]];
    }
    
    return targetCorrection;
}




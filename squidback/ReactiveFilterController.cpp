
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
    if(n_values==0 || n_values > trackings[name].size()) n_values = trackings[name].size();
    
    float sum = std::accumulate(trackings[name].rbegin(),trackings[name].rbegin()+n_values,0.0f);
    
    return sum/(float)n_values;
};

std::map<float,int> ReactiveFilterController::count(std::string name, int n_values){
    if(n_values==0 || n_values > trackings[name].size()) n_values = trackings[name].size();
    
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
    std::cout << std::endl;
}


void ReactiveFilterController::adjustControls(){
    
    float avg = average("averageDb",10);
    float peak = average("peakDb",100);
    float peakness = average("peakness",10);
    float peakThr = target->peakThreshold;
    float inVolDb = average("inVolDb",100);
    float outVolDb = average("outVolDb",100);
    float outVolDbAvg = average("outVolDb");

    float limiterCorrection = average("limiterCorrection",1);
    std::map<float,int> peaks = count("peakI");
    std::map<float,int> recentPeaks = count("peakI",10);

    
    


    // raise gain to push out to -10
    float gainMaximize = ((outVolDb + 15) + (outVolDbAvg+15)/2) * -0.001;
    // adjust gain to avoid saturation
    float gainLimiter = (limiterCorrection * 4 - 1) ;
    gainLimiter = pow(gainLimiter,5)* 0.1;
    //std::cout << gainMaximize << "\t" << gainLimiter << std::endl;
    
    float inGain = (inVolDb-outVolDb) * 0.001;
    if(inGain<0) inGain = 0;
    
    target->masterGain += gainMaximize +inGain;//+ gainLimiter;
    
    
    target->peakThreshold = avg * 0.75;
    //std::cout << peakThr << std::endl;

    //target->plasticity = (pow(2,1-(peakness/30))-1)*10;
    //target->plasticity = (pow(2,1-(peakness/30))-1)*10;
    float peakExcess = (peak-peakThr + 1) * -1;
    
    //target->plasticity = pow(10,peakExcess/20)*500;

    //std::cout << peakExcess << std::endl;
    //std::cout << peakExcess << "\t" << target->plasticity << std::endl;

    
    auto most_persistent_peak = std::max_element(recentPeaks.begin(), recentPeaks.end(),
                              [](const std::pair<float, int>& p1, const std::pair<float, int>& p2) {
                                  return p1.second < p2.second; });
    
    peakRegister[target->getBand(most_persistent_peak->first)] += 0.01;
    
    
    for(auto& p: peaks){
        target->incrementBand(p.first,p.second/(-1000));
    }
    
    
    //std::cout << x->first << ":" << x->second << std::endl;
    
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



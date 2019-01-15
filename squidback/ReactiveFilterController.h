//
//  ReactiveFilterController.hpp
//  squidback
//
//  Created by Gnlc Elia on 28/11/2018.
//  Copyright © 2018 imect. All rights reserved.
//

#ifndef ReactiveFilterController_h
#define ReactiveFilterController_h

#include <stdio.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <numeric>

class ReactiveFilter;



class ReactiveFilterController{
    
    static const int MaxTrackedValues;

    public:
    
    std::map<std::string, std::vector<float>> trackings;
    std::vector<std::string> trackingKeys;
    std::map<float, float> peakRegister;


    
    ReactiveFilter *target;
    
    void trackValue(std::string name,float value);
    
    float average(std::string name, int n_values=0);
    std::map<float,int> count(std::string name, int n_values=0);
    
    void adjustControls();
    
    void printAll();
    
    std::vector<float> getPersistentPeakCorrection();
    std::vector<float> getPersistentPeakCorrectionNoGain();
    float *getPersistentPeakCorrectionNoGainPointer();
    
    int targetNumBands=0;
    float *targetCorrection;
    
    float mapToCurve(float val,float min,float max,float curve);

    
};

#endif /* ReactiveFilterController_h */

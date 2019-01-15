//  The public Objective-C++ stuff we expose to Swift.

#import <Foundation/Foundation.h>

@interface Superpowered: NSObject

- (void)    initAudio;
- (void)    stopAudio;
- (bool)    isPlaying;
- (float)   getFade;


- (void)    getSpectrum:(float *)freqs;
- (void)    getSpectrumFrequencies:(float *)freqs;
- (int)     getNumVisualBands;

- (void)    getFilterDb:(float *)freqs;
- (void)    getCorrectionDb:(float *)freqs;

- (int)     getNumFilterBands;
- (void)    getFilterFrequencies:(float *)freqs;
- (float)    getPeakness;
- (float)    getPeak;
- (float)    getAverage;
- (float)    getLimiterCorrection;
- (void)    printTrackings;

- (float)    getMasterGain;

    
    
-(float)    setMaxGain:(float) perc;
-(void)     setPlasticity:(float) perc;
-(void)     setInertia:(float) perc;
-(void)     setPeakThr:(float) perc;
-(void)     setFilterBw:(int) perc;
-(void)     setLopass: (float) perc;
-(void)     setHipass: (float) perc;

-(void)     setMemsetGlitch: (bool) sw;

@end

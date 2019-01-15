//
//  Preset.swift
//  SuperpoweredFrequencies
//
//  Created by Gæstebruger on 21/10/2018.
//  Copyright © 2018 imect. All rights reserved.
//

import Foundation

struct Preset{
    var maxGain:Float=1.0
    var peakThr:Float=1.0
    var filterPrecision:Int=5
    var plasticity:Float=0.1
    var inertia:Float=0.0
    var lopass:Float=1.0
    var hipass:Float=0.0
    var memset=false
}

let precisionStrings = [
    "Octaves",
"ET 2 Tritones",
"ET 3 Major Thirds",
"ET 4 Minor Thirds",
"ET 6 Major Second",
"JI 7 3-limit",
"ET 12 Semitones",
"JI 19 5-limit",
"ET 24 Quarter-Tones",
"JI 31 7-limit",
"ET 43 11-limit",
"ET 48 Eighth-Tones"
]

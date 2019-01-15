//
//  GraphView.swift
//  squidback
//
//  Created by Gnlc Elia on 08/01/2019.
//  Copyright © 2019 imect. All rights reserved.
//

import UIKit

class GraphView: UIView {
    
    var spectrumXPoints:[Float]?
    var spectrumValues:[Float]?
    
    var filterXPoints:[Float]?
    var filterValues:[Float]?
    var filterPersistentValues:[Float]?
    var filterGain:Float?
    
    var peakY:Float?
    var avgY:Float?
    var inY:Float?
    var outY:Float?

    var spectrumPath:UIBezierPath?
    var filterPath:UIBezierPath?
    var filterPersistentPath:UIBezierPath?
    var filterGainPath:UIBezierPath?
    
    var peakPath:UIBezierPath?
    var avgPath:UIBezierPath?
    
    var spectrumColor:UIColor = UIColor.white.withAlphaComponent(0.85)
    var correctionColor:UIColor = UIColor.black
    var filterColor:UIColor = UIColor(red: 0.15, green: 0.15, blue: 0.15, alpha: 1)
    
    var peakColor:UIColor = UIColor.red
    var avgColor:UIColor = UIColor.green
    
    var color: UIColor?

    
    func drawSpectrum(path:UIBezierPath,xPoints:[Float]?,values:[Float]?,color:UIColor,invert:Bool,scale:Float=1){
        if(xPoints != nil && values != nil){
            if(xPoints!.count == values!.count){
                
                let y0 = invert ? 0 : self.frame.size.height
                
                path.move(to: CGPoint(x: 0.0, y: y0))
                for (x,y) in zip(xPoints!,values!){
                    path.addLine(to: CGPoint(x: CGFloat(x)*self.frame.size.width, y: CGFloat(1-y*scale)*self.frame.size.height))
                }
                path.addLine(to:CGPoint(x:self.frame.size.width,y:y0))
                path.close()
            }
        }
        color.setFill()
        path.fill()
        
    }
    
    func drawLine(path:UIBezierPath, y: Float?, color:UIColor){
        if(y != nil){
            let height =  CGFloat(1-y!)*self.frame.size.height
            path.move(to: CGPoint(x: 0.0, y:height))
            path.addLine(to:CGPoint(x:self.frame.size.width,y:height))
            color.setStroke()
            path.stroke()
        }
    }
    
    func reset(){
        if spectrumValues != nil{spectrumValues!.removeAll()}
        if filterValues != nil{filterValues!.removeAll()}
        if filterPersistentValues != nil{filterPersistentValues!.removeAll()}

        setNeedsDisplay()
    }

    
    // Only override draw() if you perform custom drawing.
    // An empty implementation adversely affects performance during animation.
    override func draw(_ rect: CGRect) {
        
        // Drawing code
        
        if let col = color {
            self.backgroundColor = col.withAlphaComponent(0.95)
            filterGainPath = UIBezierPath()
            drawSpectrum(path: filterPath!, xPoints: filterXPoints, values: filterValues, color: col, invert: false,scale:filterGain ?? 0)
        }
        
        filterPath = UIBezierPath()
        drawSpectrum(path: filterPath!, xPoints: filterXPoints, values: filterValues, color: filterColor, invert: true)
        
        filterPersistentPath = UIBezierPath()
        drawSpectrum(path: filterPersistentPath!, xPoints: filterXPoints, values: filterPersistentValues, color: correctionColor, invert: true)
        
        spectrumPath = UIBezierPath()
        drawSpectrum(path: spectrumPath!, xPoints: spectrumXPoints, values: spectrumValues, color: spectrumColor, invert: false)
        
        /*peakPath = UIBezierPath()
        drawLine(path:peakPath!,y:peakY,color:peakColor)
        
        avgPath = UIBezierPath()
        drawLine(path:avgPath!,y:avgY,color:avgColor)*/
        
    }
    

}

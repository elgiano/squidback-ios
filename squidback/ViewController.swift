import UIKit
//import Charts

// Recommended tutorial:
// "Swift and Objective-C in the Same Project"
// https://developer.apple.com/library/content/documentation/Swift/Conceptual/BuildingCocoaApps/MixandMatch.html

/*private class CubicLineSampleFillFormatter: IFillFormatter {
    func getFillLinePosition(dataSet: ILineChartDataSet, dataProvider: LineChartDataProvider) -> CGFloat {
        return -10
    }
}*/

class ViewController: UIViewController {
    
    /*@IBOutlet weak var lineChart: LineChartView!
    
    @IBOutlet weak var filterChart: LineChartView!*/
    
    @IBAction func renderCharts() {
        lineChartUpdate()
    }
    @IBOutlet weak var startBtn: UIButton!
    
    @IBOutlet weak var precisionText: UILabel!
    
    @IBOutlet weak var gainSldr: UISlider!
    @IBAction func gainSldrChanged(_ sender: UISlider) {
        let val = sender.value;
        settings.maxGain = val;
        if(started){
            realMaxGain = superpowered.setMaxGain(val)
            rescaleGraphs()
        }
    }
    @IBOutlet weak var peakThr: UISlider!
    @IBAction func peakThrChanged(_ sender: UISlider) {
        let val = sender.value;
        settings.peakThr = val;
        if(started){
            superpowered.setPeakThr(val)
        }
    }
    @IBOutlet weak var hicutSldr: UISlider!
    @IBAction func hicutChanged(_ sender: UISlider) {
        let val = sender.value;
        settings.lopass = val;
        if(started){
            superpowered.setLopass(val)
        }
    }
    @IBOutlet weak var plasticitySldr: UISlider!
    @IBAction func plasticityChanged(_ sender: UISlider) {
        let val = sender.value;
        settings.plasticity = val;
        if(started){
            superpowered.setPlasticity(val)
        }
    }
    @IBOutlet weak var inertiaSldr: UISlider!
    @IBAction func inertiaChanged(_ sender: UISlider) {
        let val = sender.value;
        settings.inertia = val;
        if(started){
            superpowered.setInertia(val)
        }
    }
    @IBOutlet weak var mamsetSwitch: UISwitch!
    @IBAction func memsetChanged(_ sender: UISwitch) {
        let val = sender.isOn;
        settings.memset = val;
        if(started){
            superpowered.setMemsetGlitch(val)
        }
    }
    
    @IBOutlet weak var precisionSldr: UISlider!
    
    @IBAction func precSliderMoved(_ sender: UISlider) {
        let val = lroundf(sender.value);
        settings.filterPrecision = val;
        sender.setValue(Float(val), animated: true)
        precisionText.text = precisionStrings[val]
        if(started){
            //superpowered.setFilterBw(val)
        }
    }
    
    @IBAction func precSliderChanged(_ sender: UISlider) {
        let val = Int(floor(sender.value));
        settings.filterPrecision = val;
        sender.setValue(Float(val), animated: true)
        precisionText.text = precisionStrings[val]
        if(started){
            superpowered.setFilterBw(Int32(val))
        }
    }
    
    func setAllControls(){
        gainSldr.setValue(settings.maxGain, animated: false)
        hicutSldr.setValue(settings.lopass, animated: false)
        plasticitySldr.setValue(settings.plasticity, animated: false)
        inertiaSldr.setValue(settings.inertia, animated: false)
        peakThr.setValue(settings.peakThr, animated: false)
        mamsetSwitch.setOn(settings.memset, animated: false)
        precisionSldr.setValue(Float(settings.filterPrecision), animated: false)
        sendAllControls()
    }
    
    func sendAllControls(){
        gainSldrChanged(gainSldr)
        hicutChanged(hicutSldr)
        plasticityChanged(plasticitySldr)
        inertiaChanged(inertiaSldr)
        peakThrChanged(peakThr)
        memsetChanged(mamsetSwitch)
        precSliderChanged(precisionSldr)
    }
 
    
    
    @IBOutlet weak var menuView: UIView!

    
    @IBOutlet weak var menuConstraint: NSLayoutConstraint!
    
    @IBAction func swipeRight(_ sender: UISwipeGestureRecognizer) {
        menuConstraint.constant = 16
        UIView.animate(withDuration: 0.3, animations: {
            self.view.layoutIfNeeded()
        })
    }
    @IBAction func swipeLeft(_ sender: UISwipeGestureRecognizer) {
        menuConstraint.constant = -1 * menuView.bounds.height - 100
        UIView.animate(withDuration: 0.3, animations: {
            self.view.layoutIfNeeded()
        })
    }
    
    @IBOutlet weak var graphView: GraphView!
    
    var superpowered:Superpowered!
    var displayLink:CADisplayLink!
    
    var numVisualBands:Int=0
    var spectrumFrequencies:UnsafeMutablePointer<Float>!
    var spectrumMags:UnsafeMutablePointer<Float>!
    var filterFrequencies:UnsafeMutablePointer<Float>!
    var filterMags:UnsafeMutablePointer<Float>!
    var correctionMags:UnsafeMutablePointer<Float>!

    var numFilterBands:Int=0
    /*var spectrumDataSet = LineChartDataSet(values:[],label:"spectrum data")
    var filterDataSet = LineChartDataSet(values:[],label:"filter data")
    var gainDataSet = LineChartDataSet(values:[],label:"gain data")
    var correctionDataSet = LineChartDataSet(values:[],label:"correction data")*/

    var settings = Preset() // load default settings
    var realMaxGain:Float = 30
    var started:Bool=false

     var screenEdgeRecognizerL: UIScreenEdgePanGestureRecognizer!
    var screenEdgeRecognizerR: UIScreenEdgePanGestureRecognizer!
    @objc func handlePanLeft(sender: UIScreenEdgePanGestureRecognizer) {
        let translation = sender.translation(in: graphView).x
        let val = Float((translation+self.view.layoutMargins.left)/self.view.bounds.width)
        //print("L:",val)
        settings.hipass = val;
        if(started){
            superpowered.setHipass(val)
        }
        
    }
    var screenEdgeRecognizer: UIScreenEdgePanGestureRecognizer!
    @objc func handlePanRight(sender: UIScreenEdgePanGestureRecognizer) {
        let translation = -sender.translation(in: graphView).x
        let val = 1-Float((translation+self.view.layoutMargins.right)/self.view.bounds.width)
        
        settings.lopass = val;
        //print("R:",val)
        if(started){
            superpowered.setLopass(val)
        }
        
    }

    override func viewDidLoad() {
        super.viewDidLoad()


        superpowered = Superpowered()

        // A display link calls us on every frame (60 fps).
        displayLink = CADisplayLink(target: self, selector: #selector(ViewController.onDisplayLink))
        if #available(iOS 10.0, *) {
            displayLink.preferredFramesPerSecond = 10
        } else {
            // Fallback on earlier versions
            displayLink.frameInterval = 6
        }
        displayLink.add(to: RunLoop.current, forMode: RunLoopMode.commonModes)
        
        
        setAllControls()
        //setupGraphs()
        
        // send away the menu
        menuConstraint.constant = -10000
        
        UIApplication.shared.isIdleTimerDisabled = true
        
        //Timer.scheduledTimer(timeInterval: 1.0, target: self, selector: #selector(printController), userInfo: nil, repeats: true)
        
        /*screenEdgeRecognizerL = UIScreenEdgePanGestureRecognizer(target: self,
                                                                action: #selector(handlePanLeft))
        screenEdgeRecognizerL.edges = .left
        self.view.addGestureRecognizer(screenEdgeRecognizerL)
        screenEdgeRecognizerR = UIScreenEdgePanGestureRecognizer(target: self,
                                                                action: #selector(handlePanRight))
        screenEdgeRecognizerR.edges = .right
        self.view.addGestureRecognizer(screenEdgeRecognizerR)*/


        
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        UIApplication.shared.isIdleTimerDisabled = false
    }
    
    /*func setupGraphs(){
        lineChart?.xAxis.enabled = false
        lineChart?.leftAxis.enabled = false
        lineChart?.rightAxis.enabled = false
        lineChart?.legend.enabled = false
        lineChart?.xAxis.drawGridLinesEnabled = false
        lineChart?.leftAxis.drawLabelsEnabled = false
        lineChart?.xAxis.drawLabelsEnabled = false


        lineChart?.isUserInteractionEnabled = false
        filterChart?.isUserInteractionEnabled = false

        filterChart?.xAxis.enabled = false
        filterChart?.leftAxis.enabled = false
        filterChart?.rightAxis.enabled = false
        filterChart?.legend.enabled = false
        filterChart?.xAxis.drawGridLinesEnabled = false
        filterChart?.leftAxis.drawLabelsEnabled = false
        lineChart?.xAxis.drawLabelsEnabled = false

        filterChart?.backgroundColor = .black
        
        spectrumDataSet.mode = .cubicBezier
        spectrumDataSet.drawCirclesEnabled = false
        spectrumDataSet.lineWidth = 0.4
        spectrumDataSet.setColor(UIColor(red: 255/255, green: 255/255, blue: 255/255, alpha: 1))
        spectrumDataSet.fillColor = .white
        spectrumDataSet.fillAlpha = 0.85
        spectrumDataSet.drawFilledEnabled = true
        spectrumDataSet.drawHorizontalHighlightIndicatorEnabled = false
        spectrumDataSet.fillFormatter = CubicLineSampleFillFormatter()
        
        //filterDataSet.mode = .cubicBezier
        filterDataSet.drawCirclesEnabled = false
        filterDataSet.lineWidth = 0
        filterDataSet.setColor(UIColor(red: 1, green: 0, blue: 0, alpha: 1))
        filterDataSet.fillColor = .black // this will be changed at runtime
        filterDataSet.fillAlpha = 0.85
        filterDataSet.drawFilledEnabled = true
        filterDataSet.drawHorizontalHighlightIndicatorEnabled = false
        filterDataSet.fillFormatter = CubicLineSampleFillFormatter()
        filterDataSet.drawValuesEnabled = false
        
        correctionDataSet.drawCirclesEnabled = false
        correctionDataSet.lineWidth = 0
        correctionDataSet.setColor(UIColor(red: 0.25, green: 0.25, blue: 0.25, alpha: 1))
        //this will be the color of the reactive filter
        correctionDataSet.fillColor = UIColor(red: 0.15, green: 0.15, blue: 0.15, alpha: 1)
        correctionDataSet.fillAlpha = 1
        correctionDataSet.drawFilledEnabled = true
        correctionDataSet.drawHorizontalHighlightIndicatorEnabled = false
        correctionDataSet.fillFormatter = CubicLineSampleFillFormatter()
        correctionDataSet.drawValuesEnabled = false
        
        gainDataSet.drawCirclesEnabled = false
        gainDataSet.lineWidth = 0
        gainDataSet.fillColor = .black // this will be changed at runtime
        gainDataSet.fillAlpha = 0.75
        gainDataSet.drawFilledEnabled = true
        gainDataSet.drawHorizontalHighlightIndicatorEnabled = false
        gainDataSet.fillFormatter = CubicLineSampleFillFormatter()
        gainDataSet.drawValuesEnabled = false
        

    }*/
    
    func scaleFilterDb(val:Float) -> Double{
        //let a = Double(1 / (1.0 - exp(5)));
        //let b = 0.001 + a;
        
        return Double(val)
        //return b - (a * Double(pow(exp(5), val))) - 0.001;
    }
    
    func lineChartUpdate(){
        
        // filter
        //var filterData = [ChartDataEntry]()
        
        // PEAK and AVERAGE lines DATA
        //graphView.peakY = Float(superpowered.getPeak()+30)/60.0
        //graphView.avgY = Float(superpowered.getAverage()+30)/60.0

        var nFilterBands = Int(superpowered.getNumFilterBands())
        while (nFilterBands != numFilterBands) {
            rescaleGraphs();
            nFilterBands = Int(superpowered.getNumFilterBands())
        }
        superpowered.getFilterDb(filterMags);
        
        //var correctionData = [ChartDataEntry]()
        superpowered.getCorrectionDb(correctionMags);
        
        //var gainData = [ChartDataEntry]()
        let masterGain = superpowered.getMasterGain();
        
        var filterY = [Float](); filterY.reserveCapacity(numFilterBands);
        var filterYPersistent = [Float](); filterYPersistent.reserveCapacity(numFilterBands);

        
        for i in 0..<Int(numFilterBands){
            /*var x = log2(Double(filterFrequencies[i]))
            var y = scaleFilterDb(val: (filterMags[i]+0+80)/(0*2+80))//Double(filterMags[i]+80)
            if(x.isNaN){x = 0}
            filterData.append(ChartDataEntry(x:x,y:y))
            
            y = y*Double(masterGain/realMaxGain) 
            if(y.isNaN){y = 0}
            gainData.append(ChartDataEntry(x:x,y:y))
            
            y = scaleFilterDb(val: (correctionMags[i]+0+80)/(0*2+80))//Double(filterMags[i]+80)
            if(y.isNaN){y = 0}
            correctionData.append(ChartDataEntry(x:x,y:y))*/
            var y = Float(scaleFilterDb(val: (filterMags[i]+0+80)/(0*2+80)))
            if(y.isNaN){y = 0}
            filterY.append(y)
            y = Float(scaleFilterDb(val: (correctionMags[i]+0+80)/(0*2+80)))
            if(y.isNaN){y = 0}
            filterYPersistent.append(y)
            
        }
        
        /*filterDataSet.values = filterData
        correctionDataSet.values = correctionData
        gainDataSet.values = gainData*/

        
        //filterChart?.data = LineChartData(dataSets: [correctionDataSet,gainDataSet,filterDataSet])
        
        graphView.filterGain = masterGain/realMaxGain;
        graphView.filterValues = filterY
        graphView.filterPersistentValues = filterYPersistent
        
        // spectrum
        //var spectrumData = [ChartDataEntry]()
        var peakIndex:Int=0;
        var testPeak:Float=0;

        var spectrumY = [Float](); spectrumY.reserveCapacity(numVisualBands);
        
        superpowered.getSpectrum(spectrumMags)
 
        for i in 0..<Int(numVisualBands){
            /*var x = log2(Double(spectrumFrequencies[i]))
            var y = 10 * log10(Double(spectrumMags[i])) + 30
            if(y.isNaN){
                y = 0
            }
            if(x.isNaN){
                x = 0
            }
            spectrumData.append(ChartDataEntry(x:x,y:y))*/
            
            if (spectrumMags[i] > testPeak) {
                testPeak = spectrumMags[i];
                peakIndex = i;
            }
            var y = Float(10 * log10(spectrumMags[i]) + 30)/60.0
            if(y.isNaN){y = 0}
            spectrumY.append(y)
        }
        graphView.spectrumValues = spectrumY
        graphView.setNeedsDisplay()

        //spectrumDataSet.values = spectrumData
        
        //let data = LineChartData(dataSets: [spectrumDataSet])
        //lineChart?.data = data

        
        // colorize
        let color = pitchToHsv(pitch: spectrumFrequencies[peakIndex], amp: spectrumMags[peakIndex])
        //filterDataSet.fillColor = color
        //gainDataSet.fillColor = color
        
        graphView.color = color
        let fade = CGFloat(superpowered.getFade())
        graphView.alpha = fade
        
        //This must stay at end of function
        //lineChart?.notifyDataSetChanged()
        //filterChart?.notifyDataSetChanged()
        
        //print(superpowered.getPeak(),"\t",superpowered.getAverage(),"\t",superpowered.getPeakness(),"\t",superpowered.getLimiterCorrection())
        
        
        
        
    }
    
    func rescaleGraphs() {
        numVisualBands = Int(superpowered.getNumVisualBands())
        numFilterBands = Int(superpowered.getNumFilterBands())
        
        spectrumFrequencies = UnsafeMutablePointer<Float>.allocate(capacity: (numVisualBands))
        spectrumMags = UnsafeMutablePointer<Float>.allocate(capacity: (numVisualBands))
        filterFrequencies = UnsafeMutablePointer<Float>.allocate(capacity: (numFilterBands))
        filterMags = UnsafeMutablePointer<Float>.allocate(capacity: (numFilterBands))
        correctionMags = UnsafeMutablePointer<Float>.allocate(capacity: (numFilterBands))

        
        for i in 0..<Int(numVisualBands){
            (spectrumFrequencies+i).initialize(to: 0.0)
            (spectrumMags+i).initialize(to: 0.0)
        }
        for i in 0..<Int(numFilterBands){
            (filterFrequencies+i).initialize(to: 0.0)
            (filterMags+i).initialize(to: 0.0)
            (correctionMags+i).initialize(to: 0.0)

        }
        
        superpowered.getSpectrumFrequencies(spectrumFrequencies)
        superpowered.getFilterFrequencies(filterFrequencies)

        /*lineChart?.leftAxis.axisMinimum = 0
        lineChart?.leftAxis.axisMaximum = Double(60)
        filterChart?.leftAxis.axisMinimum = 0
        filterChart?.leftAxis.axisMaximum = 1.0//Double(realMaxGain*2+80)
 
        

        lineChart?.xAxis.axisMinimum = log2(Double(spectrumFrequencies[0]))
        lineChart?.xAxis.axisMaximum = log2(Double(spectrumFrequencies[numVisualBands-1]))
        filterChart?.xAxis.axisMinimum = log2(Double(spectrumFrequencies[0]))
        filterChart?.xAxis.axisMaximum = log2(Double(spectrumFrequencies[numVisualBands-1]))
        */
        
        var spectrumX = [Float]();spectrumX.reserveCapacity(numVisualBands)
        for i in 0..<Int(numVisualBands){
            spectrumX.append( Float(log2(spectrumFrequencies[i])))
        }
        let min = spectrumX.min() ?? 0;let max = spectrumX.max() ?? 20000;
        graphView.spectrumXPoints = spectrumX.map { ($0 - min) / (max - min) }
        
        var filterX = [Float]();filterX.reserveCapacity(numFilterBands)
        for i in 0..<Int(numFilterBands){
            filterX.append( Float(log2(filterFrequencies[i])))
        }
        graphView.filterXPoints = filterX.map { ($0 - min) / (max - min) }
        
        
        
    }
    
    func resetGraphs(){
        //lineChart?.data = LineChartData(dataSet:LineChartDataSet(values:[],label:"empty"))
        //filterChart?.data = LineChartData(dataSet:LineChartDataSet(values:[],label:"empty"))
        startBtn.isEnabled = true
        startBtn.isHidden = false

        graphView.reset()
    }
    
    func pitchToHsv(pitch:Float,amp:Float) -> UIColor{
        // cpsmidi: 12*log2(fm/440 Hz) + 69
        let midi = (12 * log2(pitch/440)) + 69;
        let hue = midi.truncatingRemainder(dividingBy: 12)/12;
        var saturation = 1.25-(midi/120);
        var brightness = fabs(log(fabs(amp/100)/0.005) / log(1/0.005));
        
        if(saturation<0){saturation = 0}else if(saturation>1){saturation = 1}
        if(brightness<0){brightness = 0}else if(brightness>1){brightness = 1}
                
        return UIColor.init(
            hue:fabs(CGFloat(hue)).truncatingRemainder(dividingBy: 1),
            saturation:CGFloat(saturation),
            brightness:CGFloat(brightness),
            alpha:1)
    }

    @objc func onDisplayLink() {

        if(superpowered.isPlaying()){
            lineChartUpdate();
        }else{
            resetGraphs()
        }
        
    }
    @objc func printController() {
        
        if(started){
            superpowered.printTrackings();
        }
        
    }
    
    @IBAction func toggleAudio(_ sender: UIButton) {
        
        
        started = !started;
        if(started){
            superpowered.initAudio();
            rescaleGraphs();
            sendAllControls();
            UIApplication.shared.isIdleTimerDisabled = true

        }else{
            superpowered.stopAudio();
            //resetGraphs()
            //UIApplication.shared.isIdleTimerDisabled = false
            startBtn.isEnabled = false
            startBtn.isHidden = true


            
        }
        
    
        sender.isSelected = !sender.isSelected;

    }
    
}


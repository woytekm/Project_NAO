using Toybox.Application;
using Toybox.WatchUi;
using Toybox.BluetoothLowEnergy as Ble;
    
class NAOMonitor extends Application.AppBase {

    private var bleDelegate;
    private var profileManager;
    private var view;
    private var queue;

    function initialize() {
        AppBase.initialize();
    }
    
    function onStart(state) {
    }    

    function onStop(state) {
    }

    function getInitialView() {
        profileManager = new NAOPM();
        queue=new CommQueue();
        view=new NAOView(profileManager,queue); 	
		Ble.setDelegate(new NAOBLEDelegate(profileManager,view,queue));		   	
        profileManager.registerProfiles();    
        return [ view, new NAODelegate(profileManager,view,queue) ];
    }

}

using Toybox.WatchUi as Ui;
using Toybox.System as Sys;
using Toybox.BluetoothLowEnergy as Ble;

class NAODelegate extends Ui.BehaviorDelegate {
	var view,profileManager,queue;

    function initialize(pm,v,q) {
        BehaviorDelegate.initialize();
        profileManager=pm;
        queue=q;
        view=v;
    }
    
	function onSelect() {
 		return true;
    }
    
    function onBack() {
        return false;
    }
    
    function onMenu() {
		return true;	    
    }
    
    function onPreviousPage() {
            view.page--;
    		if(view.page<0) {view.page=view.MAXSCREENS-1;}
    		view.doRequests();
    		Ui.requestUpdate();
    	return true;
    }
    
    function onNextPage() {
            view.page++;
    		if(view.page==view.MAXSCREENS) {view.page=0;}
    		view.doRequests();
	    	Ui.requestUpdate();
    	return true;
    }    
}

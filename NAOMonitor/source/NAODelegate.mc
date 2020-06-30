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
           if(view.connected)
            {
            var myMenu=new Ui.Menu2({});
			if(view.page==view.BASEDATA) {
				myMenu.setTitle("NAO funcs") ; 
		    	myMenu.addItem(new Ui.MenuItem("toggle red",null,"toggleRed",null));
	      	    myMenu.addItem(new Ui.MenuItem("clear proxy bond",null,"clearBond",null));
	      	    myMenu.addItem(new Ui.MenuItem("reset proxy",null,"resetProxy",null));
	      	    myMenu.addItem(new Ui.MenuItem("set NAO name",null,"setName",null));
		   		}		            			
				Ui.pushView(myMenu, new NAOMenuDelegate(profileManager,view,queue), Ui.SLIDE_IMMEDIATE);  			
			}
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

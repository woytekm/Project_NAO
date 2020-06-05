using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;

//class PiMenuDelegate extends Ui.MenuInputDelegate {
class NAOMenuDelegate extends Ui.Menu2InputDelegate {
	var profileManager,view,queue;
	var char;
	
    function initialize(pm,v,q) {
        Menu2InputDelegate.initialize();
        profileManager=pm;
        view=v;
        queue=q;
    }

	//function onMenuItem(item) {
    function onSelect(item) {
    	var id=item.getId();
    	if(id.equals("eToggle")) {
			onBack();
    	} else if( id.equals("outC1") ) {
    	    //C1
    		var state = view.gpioOutC1==1 ? 0 : 1;
    		item.setEnabled(state==1 ? true : false);
			view.gpioOutC1=state;
    	} else if( id.equals("outC2") ) {
    		//C2
    		var state = view.gpioOutC2==1 ? 0 : 1;
    		item.setEnabled(state==1 ? true : false);
			view.gpioOutC2=state;
    	} else if (id.equals("demo")) {
    		view.demoMode=!view.demoMode;
			onBack();
    	} else if(id.equals("page2")) {
    		view.page=view.REARLIGHT;
  			view.page2IsGPIO=!view.page2IsGPIO;
			item.setLabel((view.page2IsGPIO) ? "GPIO" : "Enviro");    	
  			item.setSubLabel("Change to "+((view.page2IsGPIO) ? "Enviro" :"GPIO"));	
  			Application.Storage.setValue(view.page2Key, view.page2IsGPIO);
  			view.doRequests();	
    	}   	
    }
}

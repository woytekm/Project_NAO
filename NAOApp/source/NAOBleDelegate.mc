using Toybox.System as Sys;
using Toybox.BluetoothLowEnergy as Ble;
using Toybox.WatchUi as Ui;

//the BLE delegat.  How you see when things are happening with BLE

class NAOBLEDelegate extends Ble.BleDelegate {
	var profileManager=null;
	var view=null;
	var queue=null;
	
    function initialize(pm,topView,q) {
        BleDelegate.initialize();
        profileManager=pm;
        view=topView;
        queue=q;
    }
    
    // app connected to NAO Proxy
    function onConnectedStateChanged( device, state ) {
		view.deviceStatus=state;
		if(state==Ble.CONNECTION_STATE_CONNECTED) {
			view.stopCommTimer();
			view.connected=true;
			view.setService();
			view.commSetup=false;
			view.commDelay=2;
		}else {
			view.resetConnection(true);		
		}		
		Ui.requestUpdate();            
    }  
    
    function onScanResults( scanResults ) {    	
        var devName;
        Sys.println("Scan results returned.");
        for( var result = scanResults.next(); result != null; result = scanResults.next() ) {
        
            devName = result.getDeviceName();
            if(devName == null)
             { devName = "no name"; }
             
            Sys.println("  Found: "+devName);
            var devUUIDs = result.getServiceUuids();
            for(var uuid = devUUIDs.next(); uuid != null; uuid = devUUIDs.next())
             {
               print("    UUID: "+uuid.toString());                  
             }
        
             
//            if( contains( result.getServiceUuids(), profileManager.NAO_PROXY_SERVICE ) ) {
              if(devName.equals(profileManager.NAO_PROXY_NAME)) {
            	Sys.println("   Found NAO_PROXY_SERVICE. Now will try to connect.");
		    	Ble.setScanState(Ble.SCAN_STATE_OFF);
		    	view.scanning=false;
		    	view.startCommTimer();	    	
				view.device = Ble.pairDevice(result);
				view.curResult=result;			
				view.paired=true;
				Ui.requestUpdate();							               
            }
        }        
    }
    
    // in most cases, these just call back to the view to do things
    function onDescriptorWrite(desc, status) {
		view.handleDWrite(desc,status);    
    } 
    
    function onCharacteristicWrite(char, status) {
		view.handleCWrite(char,status);     
    }         
    
    function onCharacteristicRead(char, status, value) {
    		view.handleCRead(char, status, value);
    }

	function onDescriptorRead(desc, status, value) {
		queue.run(view);
	} 	    
    
    function onCharacteristicChanged(char, value) {
    		view.handleChanged(char,value); 
    } 
    
    //used to compare UUIDs while scanning  	               
    private function contains( iter, obj ) {
        for( var uuid = iter.next(); uuid != null; uuid = iter.next() ) {           
            if( uuid.equals( obj ) ) {
                return true;
            }
        }
        return false;
    }  
}

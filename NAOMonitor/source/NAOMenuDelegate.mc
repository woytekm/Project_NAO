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
    	
    	var NAOService = view.device.getService(profileManager.NAO_PROXY_SERVICE);
		var NAOWRChar = NAOService.getCharacteristic(profileManager.NAO_PROXY_WRITE);
		 
    	if(id.equals("toggleRed")) 
    	{
    	 Sys.println("Sending toggle red");
    	 var setNAOConf730300 = [0x09,0x74,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]b; // rear light status
    	 var setNAOConf730301 = [0x09,0x74,0x03,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]b; // rear light status
    	 var setNAOConf730302 = [0x09,0x74,0x03,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]b; // rear light status
    	 var setNAOConf730303 = [0x09,0x74,0x03,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]b; // rear light status
    	 
    	 if(view.redToggle == 0)
    	  {
  		   queue.add(view,[NAOWRChar,queue.C_WRITER,setNAOConf730301],profileManager.NAO_PROXY_WRITE);
  		   view.redToggle++;
  		  }
  		 else if(view.redToggle == 1)
    	  {
  		   queue.add(view,[NAOWRChar,queue.C_WRITER,setNAOConf730302],profileManager.NAO_PROXY_WRITE);
  		   view.redToggle++;
  		  }
  		 else if(view.redToggle == 2)
    	  {
  		   queue.add(view,[NAOWRChar,queue.C_WRITER,setNAOConf730303],profileManager.NAO_PROXY_WRITE);
  		   view.redToggle++;
  		  }
  		 else if(view.redToggle == 3)
    	  {
  		   queue.add(view,[NAOWRChar,queue.C_WRITER,setNAOConf730300],profileManager.NAO_PROXY_WRITE);
  		   view.redToggle = 0;
  		  }
  		  
  		 view.NAORefreshData();
  		  
    	} 
    	else if(id.equals("clearBond")) 
    	{    	
    	 Sys.println("Sending clear bond");
         var setProxyRemoveBond = [0x69,0x11]b;
         queue.add(view,[NAOWRChar,queue.C_WRITER,setProxyRemoveBond],profileManager.NAO_PROXY_WRITE);
        }
    }
}

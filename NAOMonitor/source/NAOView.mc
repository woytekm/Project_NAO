using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.System as Sys;
using Toybox.BluetoothLowEnergy as Ble;
using Toybox.Timer;

//
// This is a sample app to use in conjunction with a raspberry pi running the assocailed ble inertface.
// It's not meant to be a production app, but just a way to see what can be done with a pi.
//
// it allows getting "base data" from the pie, such as memory usange and uptime
// along with interacting with a Pimoroni Enviro PHAT or directly to GPIO pins
//

class NAOView extends Ui.View {

//enums for pages to display
enum {
	BASEDATA,
	REARLIGHT,
	MAXSCREENS
}

	var page2IsGPIO=true;
	
	var page=BASEDATA;
	var width,height;
	var centerW,centerH;
	var font_large=Gfx.FONT_LARGE;
	var font_medium=Gfx.FONT_MEDIUM;
	var font_small=Gfx.FONT_SMALL;
	var font_tiny=Gfx.FONT_TINY;
	var font_number_huge = Gfx.FONT_NUMBER_THAI_HOT;
	
	var scanning=false;
	
	var page2Key="page2";

	var curResult=null;
	var commSetup=false;
	var device=null;
	var paired=false;
	var deviceStatus=Ble.CONNECTION_STATE_DISCONNECTED;
	var connected=false;
	var profileManager,queue;
	var sfH_tiny=0;
	var sfH_small=0;
	var sfH_medium=0;
	var sfH_large=0;	
	var sfH_number_huge=0;
	var degree="°";				//utf-8, b0 hex, 176 dec
	var displayTimer=true;
	var requestTimer=15;		//how often to poll for new data (seconds)
	var commDelay=0;
	var notifDataTimeout = 0;
	var redFlip = 0;

	var ticks=0;
	
	var commTimer=10;
	var curCommTimer=0;
	var errorTimer=10;
	var curErrorTimer=0;
	var errorMsg="";
	var timer;
	
	var blank="--";
	var c1="";	//"p";
	var c2="";	//"r";

	
	var piService;
	var gpioOutC1=null,gpioOutC2=null,gpioInC1=null,gpioInC2=null;
	var NAOBattLeft=0;
	var NAOBattVoltage=0;
	var NAOLightIntensity=0;
	var NAORearLightStat=0;
	var deviceName="??";
	var gotNAOData=false;
	var NAOProfileName1 = "";
	var NAOProfileName2 = "";	
	
	//gpio demo mode settings
	var demoMode=false;
	//demo1 shows the state of demo1Switch in this app and how long it's been open and allows turning on demo1Output
	//for demo1ToggleTime seconds
	
	//this is for testing variations on the characteristics for GPOP.  
	//leave as false if you're not running this sort of test
	//for the pi end, 
	var gTestMode=false;	

    function initialize(pm,q) {
        View.initialize();
        profileManager=pm;
        queue=q;
	    
        resetConnection(true);
        
        var cfg=Application.Storage.getValue(page2Key);
        if(cfg!=null) {page2IsGPIO=cfg;}

		timer= new Timer.Timer();
		timer.start( method(:onTimer), 1000, true );	
	}

    // Load your resources here
    function onLayout(dc) {
		width=dc.getWidth();
		height=dc.getHeight();
		centerW=width/2;
		centerH=height/2;
		sfH_tiny = dc.getFontHeight(font_tiny);
		sfH_small=dc.getFontHeight(font_small);	
		sfH_medium=dc.getFontHeight(font_medium);	
		sfH_large=dc.getFontHeight(font_large);	
		sfH_number_huge=dc.getFontHeight(font_number_huge);
    }
    

    function onShow() {
    }

    // Update the view
    function onUpdate(dc) {	
		dc.setColor(Gfx.COLOR_BLACK,Gfx.COLOR_BLACK);
		dc.clear();
		dc.setColor(Gfx.COLOR_WHITE,Gfx.COLOR_TRANSPARENT);

		if(connected && deviceStatus==Ble.CONNECTION_STATE_DISCONNECTED) {
			setError("Disconnected");
			resetConnection(true);
		}	
		
		var y=60;
		if(!paired && curResult==null) {
			dc.drawText(centerW,centerH,font_medium,"Scanning",Gfx.TEXT_JUSTIFY_CENTER);
			return;
		} else if(!connected && paired) {
			dc.drawText(centerW,centerH,font_medium,"Connecting",Gfx.TEXT_JUSTIFY_CENTER);		
			return;
		}

		//if(device!=null && device.getName()!=null) {
		//	deviceName=device.getName();
		//}

		//dc.drawText(centerW,y,font_small,deviceName,Gfx.TEXT_JUSTIFY_CENTER);
		//y+=(sfH_small*1);

		var str;
		
		var time=Sys.getClockTime();
		ticks=time.sec;
		str=time.hour.format("%02d")+":"+time.min.format("%02d")+"  "+Sys.getSystemStats().battery.format("%d")+"%";
		dc.drawText(centerW,centerH-(sfH_number_huge*0.25)-sfH_tiny-6,font_small,str,Gfx.TEXT_JUSTIFY_CENTER);
	    y+=sfH_small;	
	    
		if(page==BASEDATA) {					
					
			if(gotNAOData) {
		         
			    dc.drawText(centerW,centerH-(sfH_number_huge*0.5),font_number_huge,NAOBattLeft+"%",Gfx.TEXT_JUSTIFY_CENTER);
			    drawPercentLeftDiagram(NAOBattLeft,dc);
			    drawLightIntensityDiagram(NAOLightIntensity,dc);					
			    //var str2 = NAOBattVoltage.format("%.2f");
			    //dc.drawText(centerW,y,font_small,"Batt volt:"+str2+"V",Gfx.TEXT_JUSTIFY_CENTER);
				//y+=sfH_small;					
						    
			    dc.setColor(Graphics.COLOR_BLUE,Graphics.COLOR_BLACK);
				dc.drawText(centerW,centerH+(sfH_number_huge*0.25),font_tiny,NAOProfileName1+NAOProfileName2,Gfx.TEXT_JUSTIFY_CENTER);
				//dc.drawText(centerW,centerH+(sfH_number_huge*0.25)+sfH_tiny+5,font_tiny,"Intensity:"+NAOLightIntensity+" "+NAORearLightStat.toString(),Gfx.TEXT_JUSTIFY_CENTER);
			    drawRearLightStatus(NAORearLightStat,dc);
		 }
      	}
      	
      	if(curErrorTimer!=0) {
      		dc.setColor(Gfx.COLOR_BLACK,Gfx.COLOR_PINK);
      		dc.drawText(centerW,centerH,font_medium,errorMsg,Gfx.TEXT_JUSTIFY_CENTER|Gfx.TEXT_JUSTIFY_VCENTER);
      	}	
    }


   function drawLightIntensityDiagram(LightIntensity,dc)
    {
     
          
     
     var frame_color = Graphics.COLOR_GREEN;
     var diag_color = 0;
     var diagramRadius = 0;
     var diagramStartDeg = 280;
     var diagramEndDeg = 260;
     var diagramWidth = 16;
     var diagramGap = diagramStartDeg - diagramEndDeg;
     var fullCircle = 360;
     var diagramDegrees = fullCircle - diagramGap;
     var end;
     
     // 53 - lowest
     // 1500 - highest
     // 50 == 0
     // 4 - base unit
       
     var lightIntensityPercent = (LightIntensity.toFloat()) / 15;
     
     if(lightIntensityPercent < 1) {lightIntensityPercent = 1;}
     
     var barUnit = diagramDegrees.toFloat() / 100;
     var barDegrees = ((lightIntensityPercent * barUnit)-1);  
        
     if(width == 240)
	  {
	    diagramRadius = 102;
	  }
	 else if(width == 280)
	  {
	    diagramRadius = 122;
	    diagramWidth = 16;
	  }
     
     diag_color = 0xff6a00;
     frame_color = 0xff6a00;
     if(lightIntensityPercent > 15) 
      {
       diag_color = 0xff9900;
       frame_color = 0xff9900;
      }
     if(lightIntensityPercent > 20) 
      {
       diag_color = 0xffb914;
       frame_color = 0xffb914;
      }
     if(lightIntensityPercent > 50) 
      {
       diag_color = 0xffe940;
       frame_color = 0xffe940;
      }
     if(lightIntensityPercent > 70) 
      {
       diag_color = 0xfbffb3;
       frame_color = 0xfbffb3;      
      }
     
     
     //dc.setColor(frame_color, Graphics.COLOR_BLACK);
	 //dc.drawArc(centerW,centerH,diagramRadius,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,diagramEndDeg);
     //dc.drawArc(centerW,centerH,diagramRadius - diagramWidth,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,diagramEndDeg);  
     
     //for(var i = 0; i < diagramWidth; i++)
     // {
     //   dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_CLOCKWISE,diagramStartDeg,diagramStartDeg-1);
     //   dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_CLOCKWISE,diagramEndDeg+1,diagramEndDeg);
     // }	  
   
      if(barDegrees < 1)
       {
         barDegrees = 1;
       }

      barDegrees = barDegrees.toNumber();
      
      dc.setColor(diag_color, Graphics.COLOR_BLACK);
     
      if((diagramEndDeg + barDegrees) > 360)
       {
        end = (diagramStartDeg + barDegrees) - 360;
       }
      else
       {
        end = diagramStartDeg + barDegrees;
       } 
       
      for(var i = 1; i < (diagramWidth); i++)
        {
         dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,end);
        }        
             
      
    }


    
   function drawPercentLeftDiagram(BattLeft,dc)
    {
     
     var frame_color = Graphics.COLOR_GREEN;
     var diag_color = 0;
     var diagramRadius = 0;
     var diagramStartDeg = 280;
     var diagramEndDeg = 260;
     var diagramWidth = 16;
     var diagramGap = diagramStartDeg - diagramEndDeg;
     var fullCircle = 360;
     var diagramDegrees = fullCircle - diagramGap;
     var barUnit = diagramDegrees.toFloat() / 100;
     var barDegrees = ((BattLeft * barUnit));
     var end;
        
     if(width == 240)
	  {
	    diagramRadius = 118;
	  }
	 else if(width == 280)
	  {
	    diagramRadius = 138;
	    diagramWidth = 16;
	  }
     
     diag_color = Graphics.COLOR_RED;
     frame_color = Graphics.COLOR_RED;
     if(BattLeft.toNumber() > 15) 
      {
       diag_color = Graphics.COLOR_ORANGE;
       frame_color = Graphics.COLOR_ORANGE;
      }
     if(BattLeft.toNumber() > 20) 
      {
       diag_color = Graphics.COLOR_YELLOW;
       frame_color = Graphics.COLOR_YELLOW;
      }
     if(BattLeft.toNumber() > 50) 
      {
       diag_color = Graphics.COLOR_GREEN;
       frame_color = Graphics.COLOR_GREEN;
      }
     if(BattLeft.toNumber() > 70) 
      {
       diag_color = Graphics.COLOR_DK_GREEN;
       frame_color = Graphics.COLOR_DK_GREEN;      
      }
     
     
     //dc.setColor(frame_color, Graphics.COLOR_BLACK);
	 //dc.drawArc(centerW,centerH,diagramRadius,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,diagramEndDeg);
     //dc.drawArc(centerW,centerH,diagramRadius - diagramWidth,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,diagramEndDeg);  
     
     //for(var i = 0; i < diagramWidth; i++)
     // {
     //   dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_CLOCKWISE,diagramStartDeg,diagramStartDeg-1);
     //   dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_CLOCKWISE,diagramEndDeg+1,diagramEndDeg);
     // }	  
   
      if(barDegrees < 1)
       {
         barDegrees = 1;
       }

      barDegrees = barDegrees.toNumber();
      
      dc.setColor(diag_color, Graphics.COLOR_BLACK);
     
      if((diagramStartDeg + barDegrees) > 360)
       {
        end = (diagramStartDeg + barDegrees) - 360;
       }
      else
       {
        end = diagramStartDeg + barDegrees;
       } 
       
      for(var i = 2; i < (diagramWidth-1); i++)
        {
         dc.drawArc(centerW,centerH,diagramRadius - i,Gfx.ARC_COUNTER_CLOCKWISE,diagramStartDeg,end);
        }        
             
      
    }
     
    
   function drawRearLightStatus(RearLightStat,dc)
    {
     if((RearLightStat == 2) || (RearLightStat == 0))
      {
        dc.setColor(Graphics.COLOR_BLACK, Graphics.COLOR_BLACK);
      }
     else if(RearLightStat == 3)
      {
        if(redFlip == 1)
         {
          dc.setColor(Graphics.COLOR_RED, Graphics.COLOR_BLACK);
          redFlip = 0;
         }
        else
         {
          dc.setColor(Graphics.COLOR_BLACK, Graphics.COLOR_BLACK);
          redFlip = 1;
         }
      }
     else if(RearLightStat == 1)   
      {
        dc.setColor(Graphics.COLOR_RED, Graphics.COLOR_BLACK);
      }
       
     dc.fillCircle(centerW,centerH+80,10);
       
    }  
    
    
    //general timer used for amany things
    function onTimer() {
        notifDataTimeout += 1;
        if(notifDataTimeout > 10)
          {  // clear stalled values
     	     NAOBattLeft=0;
	         NAOBattVoltage=0;
	         NAOLightIntensity=0;
	         NAORearLightStat=0;  
	         NAOProfileName1 = "";
	         NAOProfileName2 = "";     
          }
          
          
    	if(commDelay!=0) {
    		commDelay--;
    		if(commDelay==0) {
				if(connected && !commSetup) {doRequests();}
			}    		
    	}
    	// do reads/rights every so often on some things
    	if(ticks%requestTimer==0 && curResult!=null && connected) {
    		doRequests();			    	
    	}
    	
    	//display error if needed
    	if(curErrorTimer!=0) {
    		curErrorTimer--;
    	}
    	
    	//timeout for comm - if zero (exipred) start to display an error and clean up
    	if(curCommTimer!=0) {
    		curCommTimer--;
    		if(curCommTimer==0) {
				setError("Comm Error");
				resetConnection(true);
			}		
    	  }    		
		Ui.requestUpdate();
    }
   
   
   //enable/disable notify on a specific characteristic
    function setNotify(service,thisOne,on) {   
    	var char = (service!=null) ? service.getCharacteristic(thisOne) : null;
    	var toDo=(on) ? [0x01,0x00]b : [0x00,0x00]b;	
   		queue.add(self,[char,queue.D_WRITE,toDo],thisOne);		
    	return char;
    }
	
	//start a countdown after doing some comm
	function startCommTimer() {
		curCommTimer=commTimer;
	}
	
	//stop the countdown
	function stopCommTimer() {
		curCommTimer=0;
	}
	
	// set up tp display a specific error
	function setError(msg) {	
		errorMsg=msg;
		curErrorTimer=errorTimer; // how long to display the message
	}
	
	//convert seconds into an hh:mm:ss string
	function hms(secs) {
		var h=secs/3600;
		var m=(secs-h*3600)/60;
		var s=secs%60;
		return ""+h+":"+m.format("%02d")+":"+s.format("%02d");
	}	
	
	// do the timer based "reads" from the pi.  If the inital setup hasn't been done
	// do those calls too.
	function doRequests() {
		if(!connected) {return;}
//		setService();
		if(!commSetup) {initialComm();}
		startCommTimer();
	    var char;
	    //base data
		if(page==BASEDATA) {	    	        
		
		}  else if(page==REARLIGHT) {
			//everything is now using notify, or are writes so nothing needed													
		}
		//queue a screen update when the above are done
		queue.add(self,[null,queue.UPDATE,null],profileManager.NAO_PROXY_SERVICE);			  	
	}
	
	// set the service is needed
	function setService() {
		if(piService==null) {
			piService = device.getService(profileManager.NAO_PROXY_SERVICE );
			if(piService==null) {Sys.println("WARNING! piService is null!");}	
		}
	}

	
	//set up notifies for the characteristics used, and read the current start of
	//those characteristics
	function initialComm() {
		commSetup=true;    
	    var char;

 	    var NAOService = device.getService(profileManager.NAO_PROXY_SERVICE );
		char=setNotify(NAOService,profileManager.NAO_PROXY_READ,true);
		queue.add(self,[char,queue.C_READ,null],profileManager.NAO_PROXY_READ);
		
        NAORefreshData();

		if(!gTestMode) {

	  	}
	  							
	}
		
	
	function NAORefreshData() {
		
		var NAOService = device.getService(profileManager.NAO_PROXY_SERVICE );
		var NAOWRChar = NAOService.getCharacteristic(profileManager.NAO_PROXY_WRITE);
	
		var getNAOConf7320 = [0x09,0x73,0x20]b; // first part of profile name
		var getNAOConf7321 = [0x09,0x73,0x21]b; // second part of profile name
		var getNAOConf7303 = [0x09,0x73,0x03]b; // rear light status
		
		queue.add(self,[NAOWRChar,queue.C_WRITER,getNAOConf7320],profileManager.NAO_PROXY_WRITE);
		queue.add(self,[NAOWRChar,queue.C_WRITER,getNAOConf7321],profileManager.NAO_PROXY_WRITE);
		queue.add(self,[NAOWRChar,queue.C_WRITER,getNAOConf7303],profileManager.NAO_PROXY_WRITE);
	
	}
	 

	//
	//general handlers - some not used in demo mode
	//
    function handleDWrite(desc, status) { 
        queue.run(self);       
    } 	
	
 	function handleCWrite(char, status) {
		queue.run(self);	
	}  		
	
	//not used in demo mode	
	function handleCRead(char, status, value) {
		if(status==0) {
			var str="";
			var sz=value.size();
			for(var i=0;i<sz;i++) {str=str+value[i].toChar();}			
	        switch( char.getUuid() ) {

	     		case profileManager.NAO_PROXY_READ:
    	   			if(sz>0) {gotNAOData=true;}	       		
       				gpioInC2=value[3];
					break;										
														
				default:
					Sys.println("read char not handled="+char.getUuid()+" "+value+" s="+status);
					break;										       				                	                
			}					                		                		
		}
		queue.run(self); 	
	}
	
	// handle data that comes back in a "json" format ( {"xyz" : value}}
	function jsonToNum(str,sz,isFloat) {
		var i=str.find(":");
		var x=0;
		if(i!=null) {
			str=str.substring(i+1,sz);
			sz=str.length();
			sz--;
			str=str.substring(0,sz);
			i=str.find("\"");
			if(i!=null) {str=str.substring(i+1,sz-i-1);}		
			if(isFloat) {x=str.toFloat();}
			else {x=str.toNumber();}
		}	
		return x;
	}

	
	//not used in demo mode
	function handleChanged(char, value) {
		var sz=value.size();
		switch( char.getUuid() ) {

       		case profileManager.NAO_PROXY_READ:
       		    //Sys.println("Incominig notification from NAO_PROXY_READ");
       			if(sz>0) {gotNAOData=true;}	 
       			parseNAONotif(value);      		
				break;
																						
			default:			
				Sys.println("chg char not handled="+char.getUuid()+" v="+value);
				break;
		}      
		Ui.requestUpdate();     	
	}

	
	function parseNAONotif(notif_data)
	 {
       
	   var msg_len = notif_data.size();
	   if(msg_len != 20) {return false;}


       if(NAOProfileName1.length() < 2)
         {
           NAORefreshData();
         }
	   
	   var msg_type = notif_data.decodeNumber( Lang.NUMBER_FORMAT_UINT16, { :offset => 0  , :endianness => Lang.ENDIAN_BIG} );
	   
	   if(msg_type == 0x2003) 
	    {
	      var batt_left_units = notif_data.decodeNumber( Lang.NUMBER_FORMAT_UINT32, { :offset => 2  , :endianness => Lang.ENDIAN_LITTLE} );
	      var intensity = notif_data.decodeNumber( Lang.NUMBER_FORMAT_UINT16, { :offset => 16  , :endianness => Lang.ENDIAN_LITTLE} );
	      var voltage = notif_data.decodeNumber( Lang.NUMBER_FORMAT_UINT16, { :offset => 18  , :endianness => Lang.ENDIAN_LITTLE} );

   	      var percent_left = batt_left_units / 936503;
          var voltage_conv = voltage.toFloat() / 1000;
	   
	      NAOBattLeft = percent_left.toNumber();
	      NAOBattVoltage = voltage_conv;
	      NAOLightIntensity = intensity;
	      notifDataTimeout = 0;
	     }
	    else if(msg_type==0x7320) // first part of profile name
	     {
	      NAOProfileName1 = convertNAObytesToString(notif_data.slice(2,null));
	      notifDataTimeout = 0;
	     }
	    else if(msg_type==0x7321) // second part of profile name
	     {
	      NAOProfileName2 = convertNAObytesToString(notif_data.slice(2,null));
	      notifDataTimeout = 0;
	     }
	    else if(msg_type == 0x7303) // rear light status: 1 - off, 2 - blink, 3 - constant
	     {
	      NAORearLightStat = notif_data[3];
	      notifDataTimeout = 0;
         }	   
	 }

	 
	function convertNAObytesToString(byteArray)
	 {
	   
	   var sz=byteArray.size();
	   var str="";
	   
	   for(var i=0;i<sz;i=i+2) 
	     {
	       str=str+byteArray[i].toChar();   
	     }
	   return str;
	 }
	 
	  
	// set up for a new connection
	function resetConnection(doClear) {
	    if(scanning) {Ble.setScanState(Ble.SCAN_STATE_OFF);}
	   	scanning=false;
	   	connected=false;
    	paired=false;
    	gotNAOData=false;	
    		
	    if(device!=null) {Ble.unpairDevice(device);}	    
	    device=null;
	    deviceName="??";

	    curResult=null;
	    deviceStatus=Ble.CONNECTION_STATE_DISCONNECTED;    
	    curCommTimer=0;
	    
	    scanning=true;
        Ble.setScanState(Ble.SCAN_STATE_SCANNING);	    
	    
	    if(doClear) {	
			page=BASEDATA;
		}
		Ui.requestUpdate();
	}	    

    function onHide() {
    }    

}

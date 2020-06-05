using Toybox.BluetoothLowEnergy as Ble;
using Toybox.System as Sys;

// the bluetooth profile!  This is key to talk to the raspberry pi
//
// base UUID used here and this is something youll want to change for a real app:
//
//0f3df50f09c040a582089836c6040bXX
//
//where
//  XX=0x00 to 0x0f are services
//  XX=0x10 to 0x1f are base characteristics
//  XX=0x20 to 0x2f are enviro phat characteristics
//  XX=0x30 to 0x3f are gpio characteristics
//
// This is just what's used here, and you can use something different for your own app.
// These UUIDS do need to match what's used on the pic for the services/characteristics
//
class NAOPM {

    var NAO_PROXY_SERVICE = Ble.stringToUuid("00001523-1212-efde-6666-6666eabcd123");
    var NAO_PROXY_READ = Ble.stringToUuid("00001524-1212-efde-6666-6666eabcd123");
    var NAO_PROXY_WRITE = Ble.stringToUuid("00001525-1212-efde-6666-6666eabcd123");
    var NAO_PROXY_NAME = "NAO Proxy";

    private var NAOProfileDef = {
        :uuid => NAO_PROXY_SERVICE,
        :characteristics => [{
            :uuid => NAO_PROXY_READ,
            :descriptors => [Ble.cccdUuid()]
        }, {
            :uuid => NAO_PROXY_WRITE,
            :descriptors => [Ble.cccdUuid()]
        }]
    };   
    
    function registerProfiles() {
       	Ble.registerProfile(NAOProfileDef);     
    }
}

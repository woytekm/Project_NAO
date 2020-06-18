using Toybox.BluetoothLowEnergy as Ble;
using Toybox.System as Sys;


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

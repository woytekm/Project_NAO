<img src=https://github.com/woytekm/Project_NAO/blob/master/nao_app.jpg>

Petzl NAO+ is a top model of Petzl head lamp equipped with bluetooth link, allowing for lighting profile setup and battery monitoring. Petzl has it's own MyPetzl (not the best) Android App for NAO+, but it would be nice to
control and monitor NAO+ from some BLE enabled smart watch, like my Garmin Fenix 6. Unfortunately, NAO+ requires encrypted BLE connection, and Garmin BLE Delegate libary does not implement BLE encryption at all.
So, is everything lost? Not quite.

This project has four parts:
 - Reverse engineering NAO+ BLE interface (i won't include it here to avoid troubles, but if someone really wants to - interesting things can be inferred from BLE proxy code)
 - BLE proxy code which allows for encrypting/decrypting BLE communication between Fenix 6 and NAO+. This proxy is based on nRF52840 module. 
 - BLE proxy hardware - miniature device which you can put in your pocket or backpack when hiking/running/doing whaveter else with your head lamp on.
 - Garmin ConnectIQ app monitoring NAO+ via BLE proxy

Project is in progress: Proxy code is ready, ConnectIQ app is ready. Now on to the hardware design.



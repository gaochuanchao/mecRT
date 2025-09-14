## Message: Grant2Server

The command that the `scheduler` sends to the `server` module of edge server to initialize services for requests or terminate services for requests, which specifies:

- appId;       // unsigned int, the application id of the vehicle, 4 bytes
- resourceType;   // unsigned short, the resource type of the server  (e.g., "GPU"), 2 bytes
- service;     // unsigned short, the service name, 2 bytes
- processGnbId;   // unsigned short, the id of processing gNB, 2 bytes
- offloadGnbId;   // unsigned short, the id of offloading gNB, 2 bytes
- cmpUnits;      // int, allocated computing units, 4 bytes
- bands;      // int, allocated number of bands, 4 bytes
- exeTime;  // simtime_t, the time needed for app processing in seconds
- maxOffloadTime;  // simtime_t, the max offload time result in positive saved energy
- deadline;       // simtime_t, application deadline, 8 bytes
- outputSize;   // int, result size in bytes, 4 bytes
- inputSize;    // int, input size in bytes, 4 bytes
- start;      // bool, start=1 means this command is to initialize service
- stop;      // bool, stop =1 means this command is to terminate service
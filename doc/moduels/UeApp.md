## Module: UeApp

### Overview
`UeApp` is the application module of the user equipment (UE) in the `mecRT`. Each UE can have one or more applications. It is responsible for sending service requests to the scheduler, generating computational tasks, either process these tasks locally or offloading them to edge servers (ES) for processing based on the service grants from some ES. This module can track the conserved energy by offloading computation-intensive tasks to edge servers. 

### Source
- **Folder:** `src/mecrt/apps/ue/`
- **Quick Link:** [UeApp.h](../../src/mecrt/apps/ue/UeApp.h), [UeApp.cc](../../src/mecrt/apps/ue/UeApp.cc), [UeApp.ned](../../src/mecrt/apps/ue/UeApp.ned)

### Purpose
- Simulates a UE application that generates periodic tasks
- Sends service requests to a scheduler
- Decides whether to process tasks locally or offload them to ESs based on service grants
- Tracks statistics on processed/offloaded tasks and energy consumption

### Port Connections
The UeApp module connects to the MessageDispatcher module between the application layer and the transport layer.
- **socketOut:** UDP socket output to MAC layer
- **socketIn:** UDP socket input from MAC layer

### Signals & Statistics
- `localProcessCount`: Number of jobs processed locally
- `offloadCount`: Number of jobs offloaded
- `vehSavedEnergy`: Energy saved by offloading
- `vehEnergyConsumedIfLocal`: Energy consumed if all jobs processed locally

### cMessage
- Self-messages:
    - `selfSender_`: job instance generation for periodic tasks
    - `initRequest_`: initialization of service request and send it to the scheduler
- Incoming messages:
    - `VehGrant`: offloading grant from ES, specify whether a new job can be offloaded to the ES

### Main Methods
- `initialize(int stage)`: Sets up the module, parameters, and traffic
- `handleMessage(cMessage *msg)`: Handles events and incoming packets
- `initTraffic()`: Binds socket and schedules initial traffic
- `sendJobPacket()`: Sends a job packet to RSU or processes locally
- `sendServiceRequest()`: Sends a service request to the scheduler
- `computeExtraBytes(int dataSize)`: Calculates extra header bytes for UDP/IP (since we disable data fragmentation, we need to add the extra header bytes that supposed to be added when fragmentation happens)
- `finish()`: Finalization (empty, can be extended)

### NED Parameters (from UeApp.ned)
- `localPort`, `destPort`, `schedulerAddr`, `schedulerPort`
- `inputSize`, `resultSize`, `tos`, `dlScale`

### Example NED Usage
```ned
simple UeApp like IApp {
    parameters:
        int localPort = default(-1);
        string schedulerAddr;
        int schedulerPort;
        ...
    gates:
        output socketOut;
        input socketIn;
}
```

---
*This README summarizes the design and usage of the UeApp class in mecRT. For implementation details, see the source files.*

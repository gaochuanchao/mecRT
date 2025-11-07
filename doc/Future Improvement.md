## Issues Pending to solve

### 1. MecOspf Module
- [Solved] the hello and lsa packet has print errors in ipv4 module in the gui mode, but works fine in cmdenv mode, this issue will cause a core dump when we close the GUI window during ospf synchronization, consider to move it to the application layer in the future. So far, it does not influence the experiments in Cmdenv mode.


## 2. Global Scheduler Synchronization
- [ ] currently, the global scheduler synchronization is not well considered, when multiple global schedulers exist in the network, they may not be well synchronized. A possible solution is to let each global scheduler wait until its scheduling time is reached, if the scheduling time has been updated by other global schedulers. However, this may cause starvation if the scheduling time is always updated to a future time by other global schedulers. More advanced synchronization mechanisms may be needed in the future.
    - [x] temporarily use a counter to decide when to update the next scheduling time, i.e., when all global schedulers have finished their scheduling, then update the next scheduling time.
    - Modules involved: Scheduler.cc, NodeInfo.cc

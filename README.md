# mecRT: Mobile Edge Computing Simulator for Real-Time Applications  

**mecRT** is an open-source simulator for **Mobile Edge Computing (MEC)** scenarios (previously known as vecSim - vehicular edge computing simulator). It provides a comprehensive framework for modeling **task offloading** and **resource allocation** in heterogeneous 5G-enabled environments. By integrating realistic wireless communication (via **Simu5G**) with edge computing models, mecRT enables researchers to evaluate scheduling strategies for latency-sensitive applications under resource and deadline constraints.  

<center>
<img src="./doc/assets/architecture.png" alt="architecture" style="zoom:30%;" />
</center>

---

## 🔍 Why mecRT?  

Existing MEC simulators have limitations:  

- **iFogSim / EdgeCloudSim**: Good for resource modeling, but **lack 5G network support** and cannot capture real-time channel quality feedback.  
- **Simu5G / Fogbed**: Offer fine-grained 5G network modeling, but **do not support integrated offloading frameworks** that jointly optimize bandwidth and computational resources under deadline constraints.  

**mecRT bridges this gap** by combining realistic **5G-based communication** with a **deadline-aware scheduling framework**. This enables evaluation of adaptive offloading strategies in dynamic MEC environments.  

---

## ✨ Key Features  

- **Integrated Task Offloading & Resource Allocation**  
  - Joint optimization of bandwidth and computational resources.  
  - Online offloading control mechanisms with real-time channel feedback.  

- **5G-Based Communication**  
  - Built on **Simu5G** for realistic URLLC network modeling.  
  - UEs transmit **Sounding Reference Signals (SRS)** for channel estimation.  

- **Realistic Workloads & Traces**  
  - Support for **periodic MEC tasks** (e.g., object detection, sensor data analytics).  
  - Configurable application profiles: task type, period, deadline, data size.  
  - Integration with **real traces** or synthetic trajectories (via **SUMO**).  

- **Modular Architecture**  
  - **UE module**: application, mobility, and 5G NIC submodules.  
  - **Edge Server (ES) module**: wireless bandwidth allocation + server resource management.  
  - **Scheduler module**: customizable scheduling algorithms + system monitoring.  

- **Extensible Scheduling Framework**  
  - Plug in custom scheduling algorithms.  
  - Evaluate deadline-aware, adaptive, or approximation-based strategies.  

---

## 🏗️ System Architecture  

A typical mecRT environment consists of:  

- **User Equipments (UEs)** that offload tasks to nearby **Edge Servers (ESs)** via 5G links.  
- **Edge Servers** providing both wireless bandwidth and computational resources.  
- A centralized **Scheduler** that monitors system state and periodically decides offloading and allocation.  

mecRT extends Simu5G with:  
- Online control logic for bandwidth reallocation and suspension under poor channel conditions.  
- Deadline-aware resource allocation for real-time MEC applications.  

---

## 🚀 Installation  

[Installation Instruction](./doc/Installation_Guide.md)

If using WSL2 on windows 10/11, consider additional [GUI setting](./doc/WSL2_Setting.md).

---

## ⚡ Quick Start

**Coming Soon...**

1. Run an example simulation:
2. Modify the provided `.ini` configs to set:
   - Number of UEs and ESs
   - Task periods, deadlines, and data sizes
   - Scheduling policies

3. Visualize Result

---

## 📊 Example Use Cases

- Evaluate deadline-aware **task offloading** algorithms.
- Benchmark adaptive **resource allocation** under variable 5G channel quality.
- Study trade-offs between **latency, throughput, and computational load**.
- Integrate **real-world traces** and **application profiles** for realistic workloads.

---

## 📚 Citation

If you use mecRT in your research, please cite:



---

## 🙏 Acknowledgements

- Built on [**OMNeT++**](https://github.com/omnetpp/omnetpp), [**INET**](https://github.com/inet-framework/inet), and [**Simu5G**](https://github.com/Unipisa/Simu5G).


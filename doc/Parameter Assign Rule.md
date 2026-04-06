## Parameter Assign Rule in OMNeT++

When working with parameters in OMNeT++, especially in a hierarchical network structure, it’s crucial to understand how parameter names are resolved and how values are assigned across different modules. Here’s a concise guide to help you navigate this:

Example NED snippet:

    network DistMEC
    {
        parameters:
            double areaMinX;
            double areaMinY;
            double areaMaxX;
            double areaMaxY;
            double areaMinZ;
            double areaMaxZ;

    submodules:
        gNodeB[4]: GNodeB {
            mobility.constraintAreaMinX = parent.areaMinX;
            mobility.constraintAreaMinY = parent.areaMinY;
            mobility.constraintAreaMaxX = parent.areaMaxX;
            mobility.constraintAreaMaxY = parent.areaMaxY;
            mobility.constraintAreaMinZ = parent.areaMinZ;
            mobility.constraintAreaMaxZ = parent.areaMaxZ;
        }
    }


**Scope Rule (Where a name is searched)**
When you write a parameter expression, OMNeT++ resolves names in the current module context first.

In the example, this line was inside the gNodeB submodule block in DistMEC.ned:
- mobility.constraintAreaMinX = areaMinX

Here, areaMinX is searched in gNodeB first, not in DistMEC.  
So you must explicitly go up one level:
- mobility.constraintAreaMinX = parent.areaMinX

Useful qualifiers:
- plain name: current module
- parent.name: one level up
- ancestorIndex(n) etc.: for index logic
- full path in ini (like *.ue[*].mobility...): path-based assignment

**Override Rule (Who wins)**
Think of precedence as:
1. NED default value (lowest)
2. NED assignment from parent/compound
3. omnetpp.ini assignment (usually highest, unless parameter is final)

So a good pattern is:
- Put per-map defaults in each DistMEC.ned
- Override experiment-specific values in omnetpp.ini


**Practical checklist**
1. If value comes from same module: use plain name.
2. If value comes from containing network: use parent.name.
3. If passing through multiple levels: each level may need its own forwarding assignment.
4. If ini value seems ignored: check if parameter is marked final.
5. If “has no parameter named ...”: you are in wrong scope or typo.


**Parameter flow:**
1. DistMEC network parameters are defined in DistMEC.ned.
2. DistMEC passes values into each gNodeB and NRUe instance using parent-scope references in DistMEC.ned.
3. gNodeB/NRUe write into their mobility submodule parameters in the same file block.
4. omnetpp.ini can override DistMEC parameters at runtime via patterns like *.areaMaxX in omnetpp.ini.

Mental model (for your case):
- DistMEC.areaMaxX
- -> gnb[i].mobility.constraintAreaMaxX = parent.areaMaxX

Same for MinX, MinY, MaxY, MinZ, MaxZ.

Rule of thumb:
1. Same module value: use name directly.
2. Parent module value: use parent.name.
3. Ini override of network-level param: use wildcard path like *.areaMaxX.
4. If error says “did you mean parent.xxx?” then scope is wrong, not value type.


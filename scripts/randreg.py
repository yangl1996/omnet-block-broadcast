#!/usr/bin/python3

import networkx

template = """network {netName}
{{
    submodules:
        node[{numNodes}]: FullNode;
    connections:
{edges}}}
"""

edgeTemplate = """        node[{node1}].link++ <--> {{  delay = {edgeDelay}ms; }} <--> node[{node2}].link++;
"""

D=4
N=50

graph = networkx.generators.random_graphs.random_regular_graph(D, N)

edgeStr = ""
for a, b in graph.edges():
    edgeStr += edgeTemplate.format(node1=a, node2=b, edgeDelay=100)

print(template.format(numNodes=N, edges=edgeStr, netName="RandReg"))

#!/usr/bin/python3

import networkx
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("N", help="number of nodes", type=int)
parser.add_argument("D", help="degree", type=int)
args = parser.parse_args()

template = """network {netName}
{{
    submodules:
        node[{numNodes}]: FullNode;
    connections:
{edges}}}
"""

edgeTemplate = """        node[{node1}].peer++ <--> {{  delay = {edgeDelay}ms; }} <--> node[{node2}].peer++;
"""

D=args.D
N=args.N

graph = networkx.generators.random_graphs.random_regular_graph(D, N)

edgeStr = ""
for a, b in graph.edges():
    edgeStr += edgeTemplate.format(node1=a, node2=b, edgeDelay=0)

print(template.format(numNodes=N, edges=edgeStr, netName="RandReg"))

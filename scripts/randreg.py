#!/usr/bin/python3

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("N", help="number of nodes", type=int)
parser.add_argument("D", help="degree", type=int)
parser.add_argument("-d", "--delay", help="one-way per-edge delay in ms", type=float, default=0)
parser.add_argument("--name", help="name of the network", type=str, default="RandReg")
parser.add_argument("-m", "--module", help="module to instantiate", type=str, default="HBNode", choices=["HBNode", "LCNode"])
args = parser.parse_args()

# import the big library after parsing so ppl do not wait for the help message
import networkx

template = """// Generated with the following command:
// {command}

network {netName}
{{
    submodules:
        node[{numNodes}]: {moduleName};
    connections:
{edges}}}
"""

edgeTemplate = """        node[{node1}].peer++ <--> {{  delay = {edgeDelay}ms; }} <--> node[{node2}].peer++;
"""

graph = networkx.generators.random_graphs.random_regular_graph(args.D, args.N)

edgeStr = ""
for a, b in graph.edges():
    edgeStr += edgeTemplate.format(node1=a, node2=b, edgeDelay=args.delay)

import sys
print(template.format(numNodes=args.N, edges=edgeStr, netName=args.name, command=' '.join(sys.argv), moduleName=args.module))

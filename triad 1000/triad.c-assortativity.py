#!/usr/bin/python

import networkx as nx
G = nx.Graph()
G.add_weighted_edges_from(elist)
r = nx.degree_assortativity_coefficient(G)
print("assortativity coefficient = %3.3f"%r)
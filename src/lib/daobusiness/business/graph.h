/**
  * CODE FROM http://www.geeksforgeeks.org/topological-sorting/
  * http://www.geeksforgeeks.org/topological-sorting-indegree-based-solution/
  * Modified by David Pinelo (Asycom Global Solutions) 2016
  */
#ifndef GRAPH_H
#define GRAPH_H

#include <QtCore>
#include <alepherpglobal.h>

// A C++ program to print topological sorting of a DAG
#include<iostream>
#include <list>
#include <stack>
using namespace std;

// Class to represent a graph
class ALEPHERP_DLL_EXPORT Graph
{
    int V;    // No. of vertices'

    // Pointer to an array containing adjacency listsList
    list<int> *adj;

    // A function used by topologicalSort
    void topologicalSortUtil(int v, bool visited[], stack<int> &Stack);

public:
    Graph(int V);   // Constructor
    ~Graph();

     // function to add an edge to graph
    void addEdge(int v, int w);

    // prints a Topological Sort of the complete graph
    QList<int> topologicalSort();
};

#endif // GRAPH_H

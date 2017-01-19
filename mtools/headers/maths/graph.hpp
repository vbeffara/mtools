/** @file graph.hpp */
//
// Copyright 2015 Arvind Singh
//
// This file is part of the mtools library.
//
// mtools is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mtools  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include "../misc/misc.hpp" 
#include "../misc/stringfct.hpp" 
#include "../misc/error.hpp"
#include "vec.hpp"
#include "box.hpp"
#include "../random/classiclaws.hpp"
#include "permutation.hpp"
#include "combinatorialmap.hpp"

namespace mtools
	{

	/** Defines aliases representing a generic graph. **/
	typedef std::vector<std::vector<int> >	Graph1;
	typedef std::vector<std::deque<int> >	Graph2;
	typedef std::vector<std::list<int> >	Graph3;

	typedef Graph1 Graph; // default choice. 
	

	/**
	* Reorder the vertices of a graph according to a permutation.
	*
	* @tparam	GRAPH   	Type of the graph, typically std::vector< std::list<int> >.
	* 						- The outside container must be accessible via operator[].
	* 						- The inside container must accept be iterable and contain
	*                         elements convertible to size_t (corresponding to the indexes
	*                          the neighour vertices).
	* @param	graph	  	The graph to reorder.
	* @param	perm    	The permutation to apply: perm[i] = k means that the vertex with index k
	*                       must now become the vertex at index i in the new graph.
	* @param	invperm    	The inverse permutation of perm. (use the other permuteGraph() method if
	*						not previously computed).
	*
	* @return  the permuted graph.
	**/
	template<typename GRAPH> GRAPH permuteGraph(const GRAPH & graph, const Permutation  & perm, const Permutation & invperm)
		{
		const size_t l = graph.size();
		MTOOLS_INSURE(perm.size() == l);
		if (l == 0) return GRAPH();
		GRAPH res = permute<GRAPH>(graph, perm);	// permute the order of the vertices. 
		for (size_t i = 0; i < l; i++)
			{
			for (auto it = res[i].begin(); it != res[i].end(); it++)
				{
				(*it) = invperm[*it];
				}
			}
		return res;
		}


	/**
	* Reorder the vertices of a graph according to a permutation.
	* Same as above but also compute the inverse permutation.
	*/
	template<typename GRAPH> GRAPH permuteGraph(const GRAPH & graph, const Permutation & perm)
		{
		return(permuteGraph(graph, perm, invertPermutation(perm)));
		}


	/**
	* Convert a graph from type A to type B.
	*
	**/
	template<typename GRAPH_A, typename GRAPH_B> GRAPH_B convertGraph(const GRAPH_A & graph)
		{
		GRAPH_B res;
		const size_t l = graph.size();
		if (l == 0) return res;
		res.resize(l);
		for (size_t i = 0; i < l; i++)
			{
			auto & lv1 = graph[i];
			auto & lv2 = res[i];
			for (auto it = lv1.begin(); it != lv1.end(); ++it) { lv2.push_back(*it); }
			}
		return res;
		}



	/**
	* Explore the graph starting from a root vertex and following the oriented edges.
	* Accept lambda functions.
	*
	* See method computeDistances() below for an example of how to use it. 
	*
	* @param	gr	  	The graph.
	* @param	origin	The vertex to start exploration from.
	* @param	fun   	The function to call at each visited vertex. Of the form:
	* 					bool fun(int vert, int dist)
	* 					  - vert  : the vertice currently visited
	* 					  - dist  : the distance of vert from the stating position
	* 					  - return: true to explore its neighobur and false to stop.
	*
	* @return	the total number of vertices visited.
	**/
	template<typename GRAPH> int exploreGraph(const GRAPH & gr, int origin, std::function<bool(int, int)> fun)
		{
		const size_t l = gr.size();
		std::vector<char> vis(l, 0);
		std::vector<int> tempv1; tempv1.reserve(l);
		std::vector<int> tempv2; tempv2.reserve(l);
		std::vector<int> * pv1 = &tempv1;
		std::vector<int> * pv2 = &tempv2;
		vis[origin] = 1;
		pv1->push_back(origin);
		int sum = 1;
		int d = 0;
		while (pv1->size() != 0)
			{
			pv2->clear();
			const size_t l = pv1->size();
			for (int i = 0; i < l; i++)
				{
				const int k = pv1->operator[](i);
				if (fun(k, d))
					{
					for (auto it = gr[k].begin(); it != gr[k].end(); ++it)
						{
						const int n = (*it);
						if (vis[n] == 0) { vis[n] = 1;  pv2->push_back(n); sum++; }
						}
					}
				}
			d++;
			if (pv1 == &tempv1) 
				{ 
				pv1 = &tempv2; 
				pv2 = &tempv1; 
				} 
			else 
				{ 
				pv1 = &tempv1; 
				pv2 = &tempv2; 
				}
			}
		return sum;
		}



	/**
	 * Compute the distance from a given vertex in the graph.
	 *
	 * @param	gr				   	The graph.
	 * @param	rootVertex		   	index of the vertex to compute the distance to.
	 * @param [in,out]	maxdistance	used to indicate the max distance found.
	 * @param [in,out]	connected  	used to indicate if the graph is connected.
	 *
	 * @return	a vector containing the distance for each vertex of the graph (distance is -1 if not
	 * 			in the same connected component). Flag connected is set to true is gr is connected and
	 * 			false otherwise. maxdistance is filled with the max distance found. 
	 **/
	template<typename GRAPH> std::vector<int> computeGraphDistances(const GRAPH & gr, int rootVertex, int & maxdistance, bool & connected)
		{
		const size_t l = gr.size();
		std::vector<int> dist(l, -1);
		int md = 0;
		int nbvis = exploreGraph(gr, rootVertex, [&](int vert, int d) -> bool { dist[vert] = d; if (d > md) { md = d; } return true; });
		maxdistance = md;
		connected = (nbvis == l);
		return dist;
		}


	/**
	/* As above but without indication on the connectness and max distance.
	**/
	template<typename GRAPH> std::vector<int> computeGraphDistances(const GRAPH & gr, int rootVertex)
		{
		bool b;
		int d;
		return computeGraphDistances(gr, rootVertex,d,b);
		}


	/* forward declaration */
	struct GraphInfo;
	template<typename GRAPH>  GraphInfo graphInfo(const GRAPH & gr);


	/** Structure holding informations about a graph. */
	struct GraphInfo
		{
		// (A) Info for any graph
		bool isValid;					// true is the graph is valid
		bool isEmpty;					// true if the graph is empty

		// (B) Info for valid and un-empty graphs
		bool undirected;				// true if the graph is unoriented
		bool hasLoops;					// true if the graph has loops
		bool hasDoubleEdges;			// true if the graph has double edges
		bool hasIsolatedVertex;			// true if there are isolated vertices  
		bool hasIsolatedVertexOut;		// true if there are vertices with out-degre 0. 
		bool hasIsolatedVertexIn;		// true if there are vertices with in-degre 0. 
		
		int  nbVertices;				// number of vertices
		int  nbOrientedEdges;			// number of oriented edges
		int  maxVertexInDegree;			// maximum in-degree of any vertex
		int  minVertexInDegree;			// minimum in-degree of any vertex
		int  maxVertexOutDegree;		// maximum out-degree of any vertex
		int  minVertexOutDegree;		// minimum out-degree of any vertex

		// (C) Only for unoriented graphs;
		bool connected;					// true if the graph is connected
		int diameter_min;				// lower bound on the diameter (-1 if not connected) 
		int diameter_max;				// upper bound on the diameter (-1 if not connected) 

		// (D) Only for simple graph [unoriented, without loops, nor double edges nor isolated vertices]. 
		int nbFaces;					// number of combinatorial faces of the graph (1 for a tree)
		int genus;						// genus of the embedding (0 = planar)
		int minFaceDegree;				// minimum degree of a face
		int maxFaceDegree;				// maximum degree of a face
		int vertexRegular_average;		// = d is all vertices have degree d except at most one. -1 otherwise
		int vertexRegular_exceptional;	// = n is all vertices have the same degre excpet one with degree n. -1 otherwise
		int faceRegular_average;		// = d is all faces have degree d except at most one. -1 otherwise
		int faceRegular_exceptional;	// = n is all faces have the same degre excpet one with degree n. -1 otherwise


		std::string toString() const
			{
			std::string s;
			if (isEmpty)  { s += std::string("EMPTY GRAPH\n"); return s; }
			if (!isValid) { s += std::string("!!! INVALID GRAPH !!!!\n"); return s; }
			if (!undirected)
				{ // oriented graphs
				s += std::string("ORIENTED GRAPH\n");
				if (hasLoops) { s += std::string("    -> WITH LOOPS\n"); } else { s += std::string("    -> no loop.\n"); }
				if (hasDoubleEdges) { s += std::string("    -> WITH DOUBLE EDGES\n"); } else { s += std::string("    -> no double edge.\n"); }
				s += std::string(" - Vertices         : ") + mtools::toString(nbVertices) + "\n";
				s += std::string(" - Oriented edges   : ") + mtools::toString(nbOrientedEdges) + "\n";
				s += std::string(" - out degree range : [") + mtools::toString(minVertexOutDegree) + "," + mtools::toString(maxVertexOutDegree) + "]\n";
				s += std::string(" - in  degree range : [") + mtools::toString(minVertexInDegree) + "," + mtools::toString(maxVertexInDegree) + "]\n";
				s += std::string(" - Isolated vertice out   : ") + mtools::toString(hasIsolatedVertexOut) + "\n";
				s += std::string(" - Isolated vertices in   : ") + mtools::toString(hasIsolatedVertexIn) + "\n";
				s += std::string(" - Isolated vertices both : ") + mtools::toString(hasIsolatedVertex) + "\n";
				return s;
				}
			// undirected graph
			if ((hasIsolatedVertex) || (hasLoops) || (hasDoubleEdges)) 
				{ // not simple 
				s += std::string("UNDIRECTED GRAPH\n");
				if (hasLoops) { s += std::string("    -> WITH LOOPS\n"); }	else { s += std::string("    -> no loop.\n"); }
				if (hasDoubleEdges) { s += std::string("    -> WITH DOUBLE EDGES\n"); } else { s += std::string("    -> no double edge.\n"); }
				if (hasIsolatedVertex) { s += std::string("    -> WITH ISOLATED VERTEX\n"); } else { s += std::string("    -> no isolated vertex.\n"); }
				s += std::string("Edges        : ") + mtools::toString(nbOrientedEdges / 2) + "\n";
				s += std::string("Vertices     : ") + mtools::toString(nbVertices) + "\n";
				s += std::string("  |-> degree : [") + mtools::toString(minVertexInDegree) + "," + mtools::toString(maxVertexInDegree) + "]\n";
				if (connected) { s += std::string("CONNECTED. Estimated diameter [") + mtools::toString(diameter_min) + "," + mtools::toString(diameter_max) + "]\n"; }
				else { s += std::string("NOT CONNECTED !\n"); }
				return s;
				}
			// simple graph
			s += std::string("SIMPLE UNDIRECTED GRAPH (no loop/no double edge/no isolated vertex)\n");
			s += std::string("   Edges        : ") + mtools::toString(nbOrientedEdges/2) + "\n";
			s += std::string("   Faces        : ") + mtools::toString(nbFaces); if (nbFaces == 1) { s += std::string(" (TREE)"); }  s += "\n";
			s += std::string("     |-> degree : [") + mtools::toString(minFaceDegree) + "," + mtools::toString(maxFaceDegree) + "]\n";
			s += std::string("   Vertices     : ") + mtools::toString(nbVertices) + "\n";
			s += std::string("     |-> degree : [") + mtools::toString(minVertexInDegree) + "," + mtools::toString(maxVertexInDegree) + "]\n";
			if (!connected)	{ s += std::string("NOT CONNECTED !\n"); return s; }
			s += std::string("CONNECTED. Diameter range [") + mtools::toString(diameter_min) + "," + mtools::toString(diameter_max) + "]\n";
			s += std::string("Genus : ") + mtools::toString(genus);  if (genus == 0) s += std::string(" -> PLANAR GRAPH\n");
			if (vertexRegular_average > 0)
				{
				if (vertexRegular_average == vertexRegular_exceptional) { s += std::string("REGULAR GRAPH: every site has degree ") + mtools::toString(vertexRegular_average) + "\n"; }
				else { s += std::string("ALMOST REGULAR GRAPH: every site has degree ") + mtools::toString(vertexRegular_average) + " except one with degree " + mtools::toString(vertexRegular_exceptional) + "\n"; }
				}
			if (faceRegular_average > 0)
				{
				if (faceRegular_average == faceRegular_exceptional) { s += std::string("ANGULATION: every face has degree ") + mtools::toString(faceRegular_average) + "\n"; }
				else { s += std::string("ANGULATION WITH BOUNDARY: every face has degree ") + mtools::toString(faceRegular_average) + " except one with degree " + mtools::toString(faceRegular_exceptional) + "\n"; }
				}
			return s;
			}

		};



	// anonymous namsepace for private function
	namespace
		{ 

		/* fills part (A) and (B) of the GraphInfo structure. */
		template<typename GRAPH>  GraphInfo graphInfo_partAB(const GRAPH & gr)
			{
			GraphInfo res;

			res.isValid = isGraphValid(gr);
			res.isEmpty = isGraphEmpty(gr);
			if ((!res.isValid) || (res.isEmpty)) return res;

			res.hasLoops = false;				// no loop found yet
			res.nbVertices = (int)gr.size();	// number of vertices
			res.nbOrientedEdges = 0;			// number of edges found yet
			std::map<std::pair<int, int>, std::pair<int, int> >  mapedge;	// map to store edges count to check for symmetry
			std::vector<int> invec; invec.resize(res.nbVertices);			// in degre vector
			std::vector<int> outvec; outvec.resize(res.nbVertices);			// out degree vector
			for (int i = 0; i < res.nbVertices; i++)
				{ // iterate over the vertices
				res.nbOrientedEdges += (int)gr[i].size();					// add the edge found
				for (auto it = gr[i].begin(); it != gr[i].end(); ++it)
					{
					const int j = *it;
					if (i == j) { res.hasLoops = true; }	// loop found
					else
						{
						invec[j]++; outvec[i]++;
						if (i < j) { mapedge[{i, j}].first++; } else { mapedge[{j, i}].second++; }
						}
					}
				}
			res.hasIsolatedVertexOut = false;
			res.hasIsolatedVertexIn  = false;
			res.hasIsolatedVertex    = false;
			for (size_t i = 0; i < res.nbVertices; i++)
				{
				if (invec[i] == 0) { res.hasIsolatedVertexIn = true; }
				if (outvec[i] == 0) { res.hasIsolatedVertexOut = true;  if (invec[i] == 0) { res.hasIsolatedVertex = true; } }
				}

			res.hasDoubleEdges = false;
			res.undirected = true;
			for (auto it = mapedge.begin(); it != mapedge.end(); ++it)
				{
				auto & value = it->second;
				if (value.first != value.second) { res.undirected = false; }
				if ((value.first > 1) || (value.second > 1)) { res.hasDoubleEdges = true; }
				}
			std::sort(invec.begin(), invec.end());
			std::sort(outvec.begin(), outvec.end());
			if (invec.size() > 0) { res.minVertexInDegree = invec.front(); res.maxVertexInDegree = invec.back(); }
			if (outvec.size() > 0) { res.minVertexOutDegree = outvec.front(); res.maxVertexOutDegree = outvec.back(); }
			if ((!res.undirected) || (invec.size() < 2)) return res;
			res.vertexRegular_average = -1;
			res.vertexRegular_exceptional = -1;
			if (invec[1] == invec.back()) { res.vertexRegular_exceptional = invec.front(); res.vertexRegular_average = invec.back(); }
			if (invec[invec.size() - 2] == invec.front()) { res.vertexRegular_exceptional = invec.back(); res.vertexRegular_average = invec.front(); }
			return res;
			}

		}



	/**  Query if a graph is valid **/
	template<typename GRAPH>  bool isGraphValid(const GRAPH & gr) 
		{
		const int nbv = (int)gr.size();
		for (int i = 0; i < nbv; i++) 
			{
			for (auto it = gr[i].begin(); it != gr[i].end(); ++it)
				{
				const int j = *it; if ((j < 0) || (j >= nbv)) { return false; }
				}
			}
		return true;
		}


	/** Queries if a graph is empty. **/
	template<typename GRAPH>  bool isGraphEmpty(const GRAPH & gr) { return(gr.size() == 0); }


	/** Queries if a graph is undirected. **/
	template<typename GRAPH>  bool isGraphUndirected(const GRAPH & gr) { return graphInfo_partAB(gr).undirected;	}


	/**
	* Query if a graph is connected from vertex x (ie any position may be reached starting from
	* rootvertex following the oriented edges).
	**/
	template<typename GRAPH> bool isGraphConnected(const GRAPH & gr, int rootVertex = 0)
		{
		bool b;
		int d;
		computeGraphDistances(gr, rootVertex, d, b);
		return b;
		}


	/**
	 * Query if a graph is simple:
	 *    -> undirected, without loops, without double edges, without isolated vertices.  
	 *
	 * Those are the  graphs that can be converted into a CombinatorialMap object.
	 **/
	template<typename GRAPH>  inline bool isGraphSimple(const GRAPH & gr)
		{
		GraphInfo res = graphInfo_partAB(gr);
		return ((res.isValid)&&(res.undirected) && (!res.hasIsolatedVertex) && (!res.hasLoops) && (!res.hasDoubleEdges));
		}


	/**
	 * Gather information about a graph and put the result in
	 * a GraphInfo structure.
	 **/
	template<typename GRAPH>  GraphInfo graphInfo(const GRAPH & gr)
		{
		// part (A) and (B)
		GraphInfo res;
		res = graphInfo_partAB(gr);
		if (!res.undirected) return res; // done if the graph is oriented.
		// part (C)
		computeGraphDistances(gr, 0, res.diameter_min, res.connected);
		res.diameter_max = 2 * res.diameter_min; // conservative estimate... 
		if ((res.hasIsolatedVertex) || (res.hasLoops) || (res.hasDoubleEdges)) return res;
		// part (D)
		CombinatorialMap cm(gr);
		res.genus = cm.genus();
		GraphInfo res2 = graphInfo_partAB(cm.getDual().toGraph());
		res.nbFaces = res2.nbVertices;
		res.minFaceDegree = res2.minVertexOutDegree;
		res.maxFaceDegree = res2.maxVertexOutDegree;
		res.faceRegular_average = res2.vertexRegular_average;
		res.faceRegular_exceptional = res2.vertexRegular_exceptional;
		return res;
		}














	/**
	 * Troncate a graph, keeping only the sub-graph consisting of all the verticess a distance at
	 * most radius from centerVertex (and the edges joining these vertices).
	 *
	 * @param	gr				The graph.
	 * @param	centerVertex	The center vertex.
	 * @param	radius			The radius of the (closed) ball to keep
	 * @param	dist			(optional) The distance vector of all points to centerVertex. 
	 *
	 * @return	- first member:  the troncated graph.
	 * 			- second member: the number N of boundary vertices (is adjacent to a vertex removed). they are
	 * 			                 indexed between [0,N-1] in the new graph. 
	 * 			- third member:  the permutation that describe how the vertices are mapped to the new graph. 
	 * 			                 perm[i] is the new indice of the vertex that was initially at i.
	 * 			                 If (perm[i] >= newgraph.size()), this means the vertex i was removed.
	 **/
	template<typename GRAPH> std::tuple<GRAPH, int, Permutation> keepBall(const GRAPH & gr, int centerVertex, int radius, const std::vector<int> & dist);


	/**
	 * same as above but does not need to pass the distance vector.  
	 **/
	template<typename GRAPH> std::tuple<GRAPH, int, Permutation> keepBall(const GRAPH & gr, int centerVertex, int radius)
		{
		return keepBall(gr, centerVertex, radius, computeDistances(gr, centerVertex));
		}


	/**
	* Troncate a graph, removing all the verticess a distance less or equal to radius 
	* from centerVertex (and the edges joining these vertices).
	*
	* @param	gr				The graph.
	* @param	centerVertex	The center vertex.
	* @param	radius			The radius of the (closed) ball around vertexCenter to to removed
	* @param	dist			(optional) The distance vector of all points to centerVertex.
	*
	* @return	- first member:  the troncated graph.
	* 			- second member: the number N of boundary vertices (is adjacent to a vertex removed). they are
	* 			                 indexed between [0,N-1] in the new graph.
	* 			- third member:  the permutation that describe how the vertices are mapped to the new graph.
	* 			                 perm[i] is the new indice of the vertex that was initially at i.
	* 			                 If (perm[i] >= newgraph.size()), this means the vertex i was removed.
	**/
	template<typename GRAPH> std::tuple<GRAPH, int, Permutation> removeBall(const GRAPH & gr, int centerVertex, int radius, const std::vector<int> & dist);



	/**
	* same as above but does not need to pass the distance vector.
	**/
	template<typename GRAPH> std::tuple<GRAPH, int, Permutation> removeBall(const GRAPH & gr, int centerVertex, int radius)
		{
		return removeBall(gr, centerVertex, radius, computeDistances(gr, centerVertex));
		}





	/**
	additionnal info on the graph

	- planar ? number of faces ?
	- is a tree ? 
	- is degree regular
	- max degree of vertices, min degree
	- diameter

	**/

	}



/* end of file */

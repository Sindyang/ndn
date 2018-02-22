/*
 * getShortestPath.h
 *
 *  Created on: Feb 21, 2018
 *      Author: wangsy
 */
#ifndef GETSHORTESTPATH_H_
#define GETSHORTESTPATH_H_

#include <map>
#include <string>
#include <vector>
#include <queue>
#include <set>
#include <cmath>
#include <functional>

namespace ns3
{
namespace ndn
{
namespace nrndn
{
	
#define INF 200000000
#define nV 16

struct Edge {
	int to;          // 边终止节点
	int cost;        // 花费

	Edge(int to1, int cost1) {
		to = to1;
		cost = cost1;
	}
};

struct Ans {
	vector<int> path;
	int cost;
	int start;

	void getCost() {
		cost = G1[start][path[0]];
		for (unsigned int i = 0; i < path.size() - 1; i++) {
			cost += G1[path[i]][path[i + 1]];
		}
	}
};

class getShortestPath{
public:
	getShortestPath();
	~getShortestPath();
	void addEdge(int from, int to, double cost,vector<Edge> G[nV]);
	void dijkstra(int s, vector<Edge> G[nV]);
	void dfs(int s, int t, Ans &A, vector<Ans> &paths, int start);
	void solve(string from, string to);
	
	int G1[nV][nV];               // 图的邻接矩阵形式

private:
	Ptr<vanetmobility::sumomobility::SumoMobility> m_sumodata;
	map<string,int> m; 			//顶点与下标的对应关系
	vector<Edge> G[nV];          // 图的邻接表形式
	int dist[nV];                // 从源点出发的最短距离
	typedef pair<int, int> P;   // first是最短距离，second是顶点编号
	bool vis[nV];
	vector<Edge> G4[nV];
};


} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* GETSHORTESTPATH_H_ */
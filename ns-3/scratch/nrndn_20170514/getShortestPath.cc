/*
 * getShortestPath.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: wangsy
 */

#include<map>
#include<string>
#include "getShortestPath.h"
#include "ns3/network-module.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

using namespace std;

getShortestPath::getShortestPath()
{
	// 图的初始化
	for (uint32_t i = 0; i < nV; i++) 
	{
		for (unsigned int j = i; j < nV; j++) 
		{
			G1[i][j] = G1[j][i] = INF;
		}
	}
	
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	
	//对顶点编号
	uint32_t num = 0;
	for (map<string,Edge>::iterator edge=edges.begin();edge!=edges.end();edge++)
	{
		string from = (*edge).second.from;
		string to = (*edge).second.to;
		double cost = (*edge).second.lane.length;
		map<string,int>::iterator it = m.find(from); 
		if(it == m.end())
		{
			m[from] = num++;
		}
		
		it = m.find(to);
		if(it == m.end())
		{
			m[to] = num++;
		}
		
		addEdge(m[from],m[to],cost,G);
		G1[m[from]][m[to]] = cost;
	}
}

getShortestPath::~getShortestPath()
{
	
}

getShortestPath::addEdge(int from, int to, double cost,vector<Edge> G[nV])
{
	Edge e(to, cost);
	G[from].push_back(e);
}

getShortestPath::dijkstra(int s, vector<Edge> G[nV])
{
	//对dist进行初始化
	fill(dist, dist + nV + 1, INF);
	priority_queue<P, vector<P>, greater<P> > q;
	dist[s] = 0;
	q.push(P(0, s));
	while (!q.empty()) {
		P p = q.top();   //从尚未使用的顶点中找到一个距离最小的顶点
		q.pop();
		int v = p.second;
		if (dist[v] < p.first)
			continue;

		//G[v]代表点v与其他顶点之间的距离
		for (unsigned int i = 0; i < G[v].size(); i++) {
			Edge &e = G[v][i];
			int dis = dist[v] + e.cost;
			//更新距离列表
			if (dist[e.to] > dis) {
				dist[e.to] = dist[v] + e.cost;
				q.push(P(dist[e.to], e.to));
				G4[v].push_back(e);
			} else if (dist[e.to] == dis) {
				G4[v].push_back(e);
			}
		}
	}
}

getShortestPath::dfs(int s, int t, Ans &A, vector<Ans> &paths, int start) 
{
	if (s == t) {
		A.start = start;
		A.getCost();
		paths.push_back(A);
	}

	for (unsigned int i = 0; i < G4[s].size(); i++) {
		int u = G4[s][i].to;
		if (!vis[u]) {
			vis[u] = true;
			A.path.push_back(u);
			dfs(u, t, A, paths, start);
			A.path.pop_back();
			vis[u] = false;
		}
	}
}

getShortestPath::solve(string from, string to)
{
	//将想要得到的路线设为无穷大
	addEdge(m[from],m[to],INF,G);
	G1[m[from]][m[to]] = INF;
	
	dijkstra(m[from], G);
	
	vector<Ans> paths;
	Ans ans;
	
	memset(vis, false, sizeof(vis));
	dfs(m[from], m[to], ans, paths, m[from]);

	cout << from<< " 到 " << to<<" 所有最短路径：" << endl;
	for (uint32_t i = 0; i < paths.size(); i++) {
		for (uint32_t j = 0; j < paths[i].path.size(); j++) {
			cout << paths[i].path[j] << " ";
		}
		cout << "---cost：" << paths[i].cost << endl;
	}
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

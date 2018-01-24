/*
 * main.cpp
 *
 *  Created on: 2018年1月24日
 *      Author: wangsy
 */
#include "iostream"
#include "string"
#include "fstream"
#include "vector"
#include "queue"
#include "sstream"
#include "set"
#include "string.h"
#include "math.h"
#include <functional>

using namespace std;

#define INF 200000000
#define N 16

struct Edge {
	int to;          // 边终止节点
	int cost;        // 花费

	Edge(int to1, int cost1) {
		to = to1;
		cost = cost1;
	}
};

unsigned int nV;                      // 顶点数
unsigned int nE;                      // 边数
vector<Edge> G[N];           // 图的邻接表形式
int G1[N][N];                // 图的邻接矩阵形式
int dist[N];                 // 从源点出发的最短距离
typedef pair<int, int> P;    // first是最短距离，second是顶点编号
bool vis[N];
vector<Edge> G4[N];

void build();                                                  // 建图
void dijkstra(int s, vector<Edge> G[N]);                       // 求最短路径
void solve();                                                  // 主体

void addEdge(int from, int to, int cost, vector<Edge> G[N]) {
	Edge e(to, cost);
	G[from].push_back(e);

	//Edge e1(from, cost);
	//G[to].push_back(e1);
}

void build() {
	unsigned int i;
	ifstream fin;
	fin.open("data1.txt");
	cout << "顶点数：";
	fin >> nV;
	cout << nV << endl;
	cout << "边数：";
	fin >> nE;
	cout << nE << endl;

	// 输入图
	for (i = 0; i < nV; i++) {
		for (unsigned int j = i; j < nV; j++) {
			G1[i][j] = G1[j][i] = INF;
		}
	}
	cout << endl << "原图{边的起点，终点，花费}：" << endl;
	int from, to, cost;
	for (i = 0; i < nE; i++) {
		fin >> from >> to >> cost;
		cout << from << " " << to << " " << cost << endl;
		addEdge(from, to, cost, G);
		//G1[from][to] = G1[to][from] = cost;
		G1[from][to] = cost;
	}
	fin.close();
}

void dijkstra(int s, vector<Edge> G[N]) {
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

void dfs(int s, int t, Ans &A, vector<Ans> &paths, int start) {
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

void solve() {
	build();
	dijkstra(5, G);

	unsigned int i, j;
	cout << endl << "最短路径图{边的起点，终点，花费}：" << endl;
	for (i = 0; i < nV; i++) {
		for (unsigned int j = 0; j < G4[i].size(); j++) {
			cout << i << " " << G4[i][j].to << " " << G4[i][j].cost << endl;
		}
	}

	vector<Ans> paths;
	Ans ans;
	//ans.path.push_back(5);
	memset(vis, false, sizeof(vis));
	dfs(5, 9, ans, paths, 5);

	cout << endl << "5到9的所有最短路径：" << endl;
	for (i = 0; i < paths.size(); i++) {
		//	cout << "5 ";
		for (j = 0; j < paths[i].path.size(); j++) {
			cout << paths[i].path[j] << " ";
		}
		cout << "---cost：" << paths[i].cost << endl;
	}
}

int main() {
	solve();
	return 0;
}


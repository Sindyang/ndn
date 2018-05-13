#include "iostream"
#include "string"
#include "fstream"
#include "vector"
#include "queue"
#include "sstream"
#include "set"
#include "string.h"
#include "math.h"
#include "map"
#include <functional>

using namespace std;

#define INF 200000000
#define N 341 //RSU�ĸ���

struct Edge {
	int to;          // ����ֹ�ڵ�
	int cost;        // ����

	Edge(int to1, int cost1) {
		to = to1;
		cost = cost1;
	}
};

unsigned int nV;             // ������
unsigned int nE;             // ����
vector<Edge> G[N];           // ͼ���ڽӱ���ʽ
int G1[N][N];                // ͼ���ڽӾ�����ʽ
int dist[N];                 // ��Դ���������̾���
typedef pair<int, int> P;    // first����̾��룬second�Ƕ�����
bool vis[N];
vector<Edge> G4[N];
string junctions[N];
map<pair<int, int>, string> m; //������·�εĶ�Ӧ
string route;
ofstream out;

void build(int start, int end);                                            // ��ͼ
void dijkstra(int s, vector<Edge> G[N]);                       // �����·��
void solve(int start, int end);                                            // ����

void addEdge(int from, int to, int cost, vector<Edge> G[N]) {
	Edge e(to, cost);
	G[from].push_back(e);
}

void build(int start, int end) {
	unsigned int i;
	ifstream fin;
	fin.open("data.txt");
	//cout << "��������";
	fin >> nV;
	//cout << nV << endl;
	//cout << "������";
	fin >> nE;
	//cout << nE << endl;

	// ����ͼ
	for (i = 0; i < nV; i++) {
		for (unsigned int j = i; j < nV; j++) {
			G1[i][j] = G1[j][i] = INF;
		}
	}
	//cout << endl << "ԭͼ{�ߵ���㣬�յ㣬����}��" << endl;
	int from, to, cost;
	for (i = 0; i < nE; i++) {
		fin >> from >> to >> cost >> route;
		if (from == start && to == end)
			continue;

		pair<int, int> p;
		p = make_pair(from, to);
		m[p] = route;

		//cout << from << " " << to << " " << cost << endl;
		addEdge(from, to, cost, G);
		//G1[from][to] = G1[to][from] = cost;
		G1[from][to] = cost;
	}
	fin.close();
}

void dijkstra(int s, vector<Edge> G[N]) {
	//��dist���г�ʼ��
	fill(dist, dist + nV + 1, INF);
	priority_queue<P, vector<P>, greater<P> > q;
	dist[s] = 0;
	q.push(P(0, s));
	while (!q.empty()) {
		P p = q.top();   //����δʹ�õĶ������ҵ�һ��������С�Ķ���
		q.pop();
		int v = p.second;
		if (dist[v] < p.first)
			continue;

		//G[v]�����v����������֮��ľ���
		for (unsigned int i = 0; i < G[v].size(); i++) {
			Edge &e = G[v][i];
			int dis = dist[v] + e.cost;
			//���¾����б�
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

void solve(int start, int end) {

	build(start, end);
	dijkstra(start, G);

	unsigned int i, j;
	/*cout << endl << "���·��ͼ{�ߵ���㣬�յ㣬����}��" << endl;
	 for (i = 0; i < nV; i++) {
	 for (unsigned int j = 0; j < G4[i].size(); j++) {
	 cout << i << " " << G4[i][j].to << " " << G4[i][j].cost << endl;
	 }
	 }*/

	vector<Ans> paths;
	Ans ans;
	memset(vis, false, sizeof(vis));

	dfs(start, end, ans, paths, start);

	vector<int> result;
	for (i = 0; i < paths.size(); i++) {
		cout << endl << junctions[start] << " �� " << junctions[end]
				<< " �������·����" << endl;

		out << junctions[start] << " " << junctions[end] << " ";

		result.clear();
		result.push_back(start);
		cout << junctions[start] << " ";

		for (j = 0; j < paths[i].path.size(); j++) {
			cout << junctions[paths[i].path[j]] << " ";
			result.push_back(paths[i].path[j]);
		}

		cout << "---cost��" << paths[i].cost << endl;

		map<pair<int, int>, string>::iterator it;
		for (unsigned int i = 0; i < result.size() - 1; i++) {
			pair<int, int> p;
			p = make_pair(result[i], result[i + 1]);
			it = m.find(p);
			if (it != m.end()) {
				cout << it->second << " ";
				out << it->second << " ";
			}
		}
		out << endl;
		cout << endl;
	}
}

int main() {
	out.open("result.txt");
	ifstream fin;

	fin.open("junctions.txt");

	fin >> nV;

	string junc;
	int num;
	for (unsigned int i = 0; i < nV; i++) {
		fin >> junc >> num;
		junctions[num] = junc;
	}

	fin.close();

	fin.open("data.txt");
	//cout << "��������";
	fin >> nV;
	//cout << nV << endl;
	//cout << "������";
	fin >> nE;
	//cout << nE << endl;

	//cout << endl << "ԭͼ{�ߵ���㣬�յ㣬����}��" << endl;
	int start, end, cost;
	for (unsigned int i = 0; i < nE; i++) {
		fin >> start >> end >> cost >> route;

		solve(start, end);

		//��ʼ��
		for (unsigned int i = 0; i < nV; i++) {
			G[i].clear();
			G4[i].clear();
			memset(dist, 0, sizeof(dist));
		}
	}
	fin.close();
	out.close();
	return 0;
}

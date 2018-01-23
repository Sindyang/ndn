#include<cstdio>
#include<queue>
#include<vector>
#include<map>
#include<string>
#include<iostream>
#include<cmath>
#include<cstdlib>
#include<cstring>
#include<algorithm>
using namespace std;
#define INF 0x7fffffff
#define LL __int64

const int maxn = 105;
int mpt[maxn][maxn];
int vis[maxn]; //找到最短路径的点
int m, n;
int dist[maxn]; //从起点到其他点的最短距离

int dijkstra() {
	int start, end;
	start = 1;
	end = n;
	for (int i = 1; i <= n; i++)
		dist[i] = mpt[start][i];
	vis[1] = 1;
	while (true) {
		//第一步
		//找出与起点集合相连的最短边
		int minx = INF;
		int v;
		for (int i = 1; i <= n; i++) {
			if (dist[i] < minx && vis[i] == 0) {
				minx = dist[i];
				v = i;
			}
		}
		if (minx >= INF)
			break;        //如果所有点都在起点集合内

		vis[v] = 1;
		//松弛、更新DIST数组
		for (int i = 1; i <= n; i++) {
			if (vis[i]
					== 0&&dist[i]>dist[v]+mpt[v][i]&&dist[v]<INF&&mpt[v][i]<INF) {
				dist[i] = dist[v] + mpt[v][i];
			}
		}
	}
	return dist[end];
}

void init() {
	memset(vis, 0, sizeof(vis));
	for (int i = 1; i <= n; i++) {
		for (int j = 1; j <= n; j++) {
			if (i == j)
				mpt[i][j] = 0;
			else
				mpt[i][j] = INF;
		}
	}
}

int main() {

	while (scanf("%d%d", &n, &m) != EOF) {
		if (n == 0 && m == 0)
			break;
		init();
		//建图
		for (int i = 1; i <= m; i++) {
			int a, b, c;
			scanf("%d%d%d", &a, &b, &c);
			if (c < mpt[a][b]) {
				mpt[a][b] = c;
				mpt[b][a] = c;
			}
		}
		printf("%d\n", dijkstra());
	}

	return 0;
}


#include<cstdio>
#include<queue>
#include<vector>
#include<map>
#include<string>
#include<iostream>
#include<cmath>
#include<cstdlib>
#include<cstring>
#include<algorithm>
using namespace std;
#define INF 0x7fffffff
#define LL __int64

const int maxn = 105;
int mpt[maxn][maxn];
int vis[maxn]; //找到最短路径的点
int m, n;
int dist[maxn]; //从起点到其他点的最短距离

int dijkstra() {
	int start, end;
	start = 1;
	end = n;
	for (int i = 1; i <= n; i++)
		dist[i] = mpt[start][i];
	vis[1] = 1;
	while (true) {
		//第一步
		//找出与起点集合相连的最短边
		int minx = INF;
		int v;
		for (int i = 1; i <= n; i++) {
			if (dist[i] < minx && vis[i] == 0) {
				minx = dist[i];
				v = i;
			}
		}
		if (minx >= INF)
			break;        //如果所有点都在起点集合内

		vis[v] = 1;
		//松弛、更新DIST数组
		for (int i = 1; i <= n; i++) {
			if (vis[i]
					== 0&&dist[i]>dist[v]+mpt[v][i]&&dist[v]<INF&&mpt[v][i]<INF) {
				dist[i] = dist[v] + mpt[v][i];
			}
		}
	}
	return dist[end];
}

void init(int numofRSU) {
	memset(vis, 0, sizeof(vis));
	for (int i = 0; i < numofRSU; i++) {
		for (int j = 0; j < numofRSU; j++) {
			if (i == j)
				mpt[i][j] = 0;
			else
				mpt[i][j] = INT_MAX;
		}
	}
}

int main() {
	int start; //起点
	int end; //终点
	int numofRSU;
	int numofroute;
	string junction[] = {"0/0","0/1",};
	string from;
	string to;
	double length;
	for(int i = 0;i < numofroute;i++)
	{

	}
	return 0;
}


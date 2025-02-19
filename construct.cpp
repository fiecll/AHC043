#include <bits/stdc++.h>
#include <atcoder/all>
using namespace std;
using namespace atcoder;
#define rep(i, n) for (int i = 0; i < (int)(n); i++)
#define all(v) v.begin(), v.end()
using ll = long long;
using Graph = vector<vector<int>>;
using P = pair<int,int>;
template<typename T,typename U>inline bool chmax(T&a,U b)
{if(a<b){a=b;return 1;}return 0;}
template<typename T,typename U>inline bool chmin(T&a,U b)
{if(a>b){a=b;return 1;}return 0;}

ofstream logFile("log.txt");

static int dx[4]={-1,0,1,0};// 上下左右
static int dy[4]={0,1,0,-1};

// 8方向 上　下　右　左　右上　右下　左下　左上
static int dx8[8] = {-1, 1, 0, 0, -1, 1, 1, -1}; // 8方向 上　下　右　左　右上　右下　左下　左上
static int dy8[8] = {0, 0, 1, -1, 1, 1, -1, -1};

// グリッド状態: 0=更地, 1=線路, 2=駅
static const int EMPTY   = 0;
static const int RAIL    = 1;
static const int STATION = 2;

// コスト
static const int COST_STATION = 5000;
static const int COST_RAIL    = 100;

// 人の所在地・職場
struct Person {
    int sx, sy; // 家
    int tx, ty; // 職場
    int ekix, ekiy; // 最寄り駅
    int aimx, aimy; //　職場の最寄り駅
    bool cango = false; // 職場まで行けるか
    int fare (){
        return abs(sx - tx) + abs(sy - ty);
    }
};

int getRailType(int x1,int y1,int x2,int y2){
    if(x1==0&&x2 == 0) return 1;
    if(y1==0&&y2 == 0) return 2;
    if(((x1==0 && y1 == 1) && (x2 == 1 && y2 == 0)) || ((x1 == -1 && y1 == 0) && (x2 == 0 && y2 == -1))) return 3;
    if(((x1==0 && y1 == 1) && (x2 == -1 && y2 == 0)) || ((x1 == 1 && y1 == 0) && (x2 == 0 && y2 == -1))) return 4;
    if(((x1==0 && y1 == -1) && (x2 == -1 && y2 == 0)) || ((x1 == 1 && y1 == 0) && (x2 == 0 && y2 == 1))) return 5;
    if(((x1==0 && y1 == -1) && (x2 == 1 && y2 == 0)) || ((x1 == -1 && y1 == 0) && (x2 == 0 && y2 == 1))) return 6;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N, M, K, T;
    cin >> N >> M >> K >> T;

    vector<Person> people(M);
    for(int i=0; i<M; i++){
        cin >> people[i].sx >> people[i].sy >> people[i].tx >> people[i].ty;
    }

    int saraly = 0;

    // グリッド (0=更地,1=線路,2=駅)
    vector<vector<int>> grid(N, vector<int>(N, EMPTY));
    // 資金
    int funds = K;
    // 出力アクション (最大 T=800)
    vector<string> actions;
    actions.reserve(T);

    // (マンハッタン距離, index) でソート
    vector<pair<int,int>> order;
    order.reserve(M);
    for(int i=0; i<M; i++){
        int dist = abs(people[i].sx - people[i].tx)
                 + abs(people[i].sy - people[i].ty);
        order.push_back({dist, i});
    }
    sort(order.begin(), order.end());
    reverse(order.begin(), order.end());

    // 駅を建設(家or職場). コスト5000
    auto buildStation = [&](int r,int c, Person &person, int finished)->bool {
        assert(finished == 0 || finished == 1);
        if((int)actions.size()>=T) return false; // 行動数上限
        if(funds < COST_STATION){
            // 資金不足 => 待機
            actions.push_back("-1");
            return false;
        }
        if(grid[r][c] != EMPTY){
            // 上書き禁止 => 待機
            actions.push_back("-1");
            return false;
        }
        // 建設
        grid[r][c] = 2; // STATION
        funds -= COST_STATION;
        if(finished == 0) person.ekix = r, person.ekiy = c;
        if(finished == 1) person.aimx = r, person.aimy = c;
        actions.push_back("0 "+to_string(r)+" "+to_string(c));
        return true;
    };

    // 線路を建設(1～6). コスト100
    auto buildRail = [&](int r,int c,int p, Person &person)->bool {
        if((int)actions.size()>=T) return false;
        if(funds < COST_RAIL){
            actions.push_back("-1");
            logFile << "資金不足" << endl;
            return false;
        }
        if(grid[r][c] != EMPTY){
            // 上書き禁止
            actions.push_back("-1");
            logFile << "上書き禁止" << endl;
            return false;
        }
        grid[r][c] = 1; // RAIL
        funds -= COST_RAIL;
        // 出力: "p r c"
        
        actions.push_back(to_string(p)+" "+to_string(r)+" "+to_string(c));
        return true;
    };

    // 4方向BFSで更地のみ通過する最短経路
    auto findPath = [&](int sx,int sy,int tx,int ty){
        vector<vector<bool>> visited(N, vector<bool>(N,false));
        vector<vector<pair<int,int>>> parent(N, vector<pair<int,int>>(N,{-1,-1}));
        queue<pair<int,int>>q;
        visited[sx][sy] = true;
        q.push({sx,sy});
        bool found=false;
        

        while(!q.empty()){
            auto [cx,cy] = q.front(); q.pop();
            if(cx==tx && cy==ty){
                found=true; 
                break;
            }
            for(int i=0;i<4;i++){
                int nx=cx+dx[i], ny=cy+dy[i];
                if(nx<0||nx>=N||ny<0||ny>=N) continue;
                // 駅や線路は通行不可 => 更地のみ
                if(!visited[nx][ny] && grid[nx][ny]==EMPTY){
                    visited[nx][ny]=true;
                    parent[nx][ny] = {cx, cy};
                    q.push({nx,ny});
                }
            }
        }
        if(!found) return vector<pair<int,int>>{};
        // 経路復元
        vector<pair<int,int>> path;
        {
            int cx=tx, cy=ty;
            while(!(cx==sx && cy==sy)){
                path.push_back({cx,cy});
                auto [px,py] = parent[cx][cy];
                cx=px; cy=py;
            }
            path.push_back({sx,sy});
            reverse(path.begin(), path.end());
        }
        return path;
    };

    // 近い人から順に
    for(auto &od : order){
        if((int)actions.size()>=T) break;
        bool canceled=false;

        int i=od.second;
        int sx=people[i].sx, sy=people[i].sy;
        int tx=people[i].tx, ty=people[i].ty;
        int predictedcost = COST_STATION * 2 + COST_RAIL * (abs(sx - tx) + abs(sy - ty));
        if(funds < predictedcost) continue;

        // 家を駅化
        bool finish1 = false;
        while(!finish1){
            // 失敗 => この人終了
            if(!buildStation(sx, sy, people[i], 0)){
            }
            else {
                finish1 = true;

                }
                if(actions.size() >= T){
                    finish1 = true;
                }
                funds += saraly;
        }
        if((int)actions.size()>=T || canceled) continue;

        // BFSで家(駅)→職場(更地)のパス探索(駅/線路は障害扱い)
        auto path = findPath(sx, sy, tx, ty);
        if(path.empty()){
            // 経路なし => 次の人へ
            continue;
        }
        if((int)actions.size()>=T) continue;

        // 中間セルに線路敷設
        for(int pi=1; pi<(int)path.size()-1; pi++){
            if((int)actions.size()>=T){ canceled=true; break; }
            auto prev=path[pi-1];
            auto cur =path[pi];
            auto next=path[pi+1];

            int movex1 = cur.first - prev.first;
            int movey1 = cur.second - prev.second;
            int movex2 = next.first - cur.first;
            
            
            int movey2 = next.second - cur.second;

            int rtype = getRailType(movex1, movey1, movex2, movey2);
            
            if(!buildRail(cur.first, cur.second, rtype, people[i])){
                pi--;
            }
            funds += saraly;
            logFile << funds << endl;
        }
        if(canceled) continue;
        if((int)actions.size()>=T) continue;

        // 職場が更地なら駅化
        if(grid[tx][ty] == EMPTY){
            bool finish = false;
            while(!finish){
                if(!buildStation(tx, ty, people[i], 1)){
                    
                }
                else {
                finish = true;
                saraly += people[i].fare();
                logFile << "saraly" << people[i].fare() << endl;
                logFile << people[i].sx << " " << people[i].sy << " " << people[i].tx << " " << people[i].ty << endl;
                }
                funds += saraly;
                if(actions.size() >= T){
                    finish = true;
                }
            }
        }
        if(canceled) logFile << "職場駅化失敗" << lower_bound(order.begin(), order.end(), od) - order.begin() << endl;
    }

    

    // 残りは待機(-1)
    while((int)actions.size() < T){
        actions.push_back("-1");
        logFile << "回数オーバー" << endl;
    }

    // 出力
    for(int i=0; i<T; i++){
        cout << actions[i] << "\n";
    }
    logFile.close();
    return 0;
}

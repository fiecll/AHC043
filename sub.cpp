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


// ----- 定数と方向ベクトル -----
static int dx[4] = {-1, 0, 1, 0};   // 上下左右
static int dy[4] = {0, 1, 0, -1};

static int dx8[8] = {-1, 1, 0, 0, -1, 1, 1, -1};
static int dy8[8] = {0, 0, 1, -1, 1, 1, -1, -1};

static const int EMPTY   = 0;   // 更地
static const int RAIL    = 1;   // 線路
static const int STATION = 2;   // 駅

static const int COST_STATION = 5000;
static const int COST_RAIL    = 100;
static const int endtime      = 700;
static const int cellsize     = 3;
//int gridcount = (50/cellsize) * (50/cellsize);


ofstream logFile("log.txt");

// ----- 人の情報 -----
struct Person {
    int id;
    int sx, sy;    // 家
    int tx, ty;    // 職場
    int ekix, ekiy;  // 家側最寄り駅
    int aimx, aimy;  // 職場側最寄り駅
    int ekicellid = -1;
    int aimcellid = -1;
    int fare() const {
        return abs(sx - tx) + abs(sy - ty);
    }
    bool cango() const {
        if (ekicellid == -1 || aimcellid == -1) return false;
        else if (ekicellid == aimcellid) return true;
        else return false;
    }
    
    Person() { ekix = -1; ekiy = -1; aimx = -1; aimy = -1; }
};

int N, M, K, T;
vector<Person> people;
int gridVersion = 0; // グリッド状態のバージョン管理用



// ----- グリッドセルの情報 ----- 
struct GridCell {
    int x, y;
    vector<int> homeStationUserIds;
    vector<int> workStationUserIds;
    bool ekiconstructed = false;
};

struct SimulationState {
    vector<Person> people;
    vector<GridCell> gridCells;
    int funds;
    vector<vector<int>> grid; // グリッド状態
    dsu gridcell ;
    int turn = 0;
    int ekicount = 0;
    int gridVersion = 0;
    vector<int> gridCellIndex;    // 駅を建設したグリッドセルのインデックス
    vector<bool> gridCellIndexBool; // 駅を建設したグリッドセルのインデックス
    map<int,set<int>> homeindex;        // 家側接続のユーザID
    map<int,set<int>> workplaceindex;   // 職場側接続のユーザID
};

SimulationState initialState;




// ----- 2セル間の運賃・コスト計算 -----
//これは、ネットワーク最初の二点接続を行うときのみ
int calculateFareIncrease(const GridCell &cellA, const GridCell &cellB, vector<Person> &people) {
    int totalFare = 0;
    unordered_set<int> workB(cellB.workStationUserIds.begin(), cellB.workStationUserIds.end());
    for (int userId : cellA.homeStationUserIds) {
        if (workB.find(userId) != workB.end())
            totalFare += people[userId].fare();
    }
    unordered_set<int> workA(cellA.workStationUserIds.begin(), cellA.workStationUserIds.end());
    for (int userId : cellB.homeStationUserIds) {
        if (workA.find(userId) != workA.end())
            totalFare += people[userId].fare();
    }
    return totalFare;
}


int calcSecondFareIncreaseCost(int fromid,int toid,map<int,set<int>>&homeindex, map<int,set<int>>& workindex, vector<Person> &people) {
    int totalFare = 0;
    for (auto id : homeindex[fromid]) {
        if (workindex[toid].find(id) != workindex[toid].end() && !people[id].cango())
            totalFare += people[id].fare();
    }
    for (auto id : homeindex[toid]) {
        if (workindex[fromid].find(id) != workindex[fromid].end() && !people[id].cango())
            totalFare += people[id].fare();
    }
    return totalFare;
}

// ----- 線路の種類判定 ----- 
int getRailType(int x1, int y1, int x2, int y2) {
    if(x1 == 0 && x2 == 0) return 1;
    if(y1 == 0 && y2 == 0) return 2;
    if (((x1 == 0 && y1 == 1) && (x2 == 1 && y2 == 0)) ||
        ((x1 == -1 && y1 == 0) && (x2 == 0 && y2 == -1))) return 3;
    if (((x1 == 0 && y1 == 1) && (x2 == -1 && y2 == 0)) ||
        ((x1 == 1 && y1 == 0) && (x2 == 0 && y2 == -1))) return 4;
    if (((x1 == 0 && y1 == -1) && (x2 == -1 && y2 == 0)) ||
        ((x1 == 1 && y1 == 0) && (x2 == 0 && y2 == 1))) return 5;
    if (((x1 == 0 && y1 == -1) && (x2 == 1 && y2 == 0)) ||
        ((x1 == -1 && y1 == 0) && (x2 == 0 && y2 == 1))) return 6;
    return 0;
}

inline int manhattan(int r1, int c1, int r2, int c2) {
    return abs(r1 - r2) + abs(c1 - c2);
}

// ----- 代表セルの抽出 -----
// 各ブロック内で、マンハッタン距離2以内の家と職場数が最大のセルを代表とする
vector<pair<int,int>> getRepresentatives(int N, const vector<Person> &people, vector<GridCell> &gridCells) {
    int blockSize = cellsize;
    int numBlocks = N / blockSize;
    vector<pair<int,int>> representatives;

    for (int bi = 0; bi < numBlocks; bi++) {
        for (int bj = 0; bj < numBlocks; bj++) {
            int startR = bi * blockSize;
            int endR   = startR + blockSize - 1;
            int startC = bj * blockSize;
            int endC   = startC + blockSize - 1;
            
            int bestCount = -1;
            pair<int,int> bestCell = {-1, -1};
            vector<int> bestHomeIds, bestWorkIds;
            
            for (int r = startR; r <= endR; r++) {
                for (int c = startC; c <= endC; c++) {
                    int cnt = 0;
                    vector<int> homeIds, workIds;
                    for (const auto &p : people) {
                        if (manhattan(r, c, p.sx, p.sy) <= 2) {
                            cnt++;
                            homeIds.push_back(p.id);
                        }
                        if (manhattan(r, c, p.tx, p.ty) <= 2) {
                            cnt++;
                            workIds.push_back(p.id);
                        }
                    }
                    if (cnt > bestCount) {
                        bestCount = cnt;
                        bestCell = {r, c};
                        bestHomeIds = homeIds;
                        bestWorkIds = workIds;
                    }
                }
            }
            if (bestCell.first != -1) {
                representatives.push_back(bestCell);
                gridCells.push_back({bestCell.first, bestCell.second, bestHomeIds, bestWorkIds});
            }
        }
    }
    return representatives;
}

vector<pair<int, int>> getRepresentatives2(int N, const vector<Person>& people, vector<GridCell>& gridCells) {
    int blockSize = cellsize;
    int numBlocks = N / blockSize;
    vector<pair<int, int>> representatives;

    for (int bi = 0; bi < numBlocks; bi++) {
        for (int bj = 0; bj < numBlocks; bj++) {
            int startR = bi * blockSize;
            int endR = startR + blockSize - 1;
            int startC = bj * blockSize;
            int endC = startC + blockSize - 1;

            int bestCount = -1, secondBestCount = -1;
            pair<int, int> bestCell = {-1, -1}, secondBestCell = {-1, -1};
            vector<int> bestHomeIds, bestWorkIds, secondBestHomeIds, secondBestWorkIds;

            // 各ブロック内の全セルを調べる
            for (int r = startR; r <= endR; r++) {
                for (int c = startC; c <= endC; c++) {
                    int cnt = 0;
                    vector<int> homeIds, workIds;

                    for (const auto& p : people) {
                        if (manhattan(r, c, p.sx, p.sy) <= 2) {
                            cnt++;
                            homeIds.push_back(p.id);
                        }
                        if (manhattan(r, c, p.tx, p.ty) <= 2) {
                            cnt++;
                            workIds.push_back(p.id);
                        }
                    }

                    if (cnt > bestCount) {
                        // 以前の bestCell を secondBestCell に更新
                        secondBestCount = bestCount;
                        secondBestCell = bestCell;
                        secondBestHomeIds = bestHomeIds;
                        secondBestWorkIds = bestWorkIds;

                        // 新しい bestCell を設定
                        bestCount = cnt;
                        bestCell = {r, c};
                        bestHomeIds = homeIds;
                        bestWorkIds = workIds;
                    } 
                    // bestCell とは異なる場所で、2番目に良いものを選ぶ
                    else if (cnt > secondBestCount) {
                        secondBestCount = cnt;
                        secondBestCell = {r, c};
                        secondBestHomeIds = homeIds;
                        secondBestWorkIds = workIds;
                    }
                }
            }

            // 代表点1（必須）
            if (bestCell.first != -1) {
                representatives.push_back(bestCell);
                gridCells.push_back({bestCell.first, bestCell.second, bestHomeIds, bestWorkIds});
            }

            // 代表点2（オプション: bestCell と異なる場合のみ追加）
            if (secondBestCell.first != -1 && secondBestCell != bestCell) {
                representatives.push_back(secondBestCell);
                gridCells.push_back({secondBestCell.first, secondBestCell.second, secondBestHomeIds, secondBestWorkIds});
            }
        }
    }
    return representatives;
}


vector<int> cangolist(vector<Person> &people) {
    vector<int> cangolist;
    for(auto &p: people) {
        if(p.cango()) cangolist.push_back(p.id);
    }
    return cangolist;
}

int calcsarary(vector<int> cangolist, vector<Person> &people) {
    int sarary = 0;
    for(auto id : cangolist) {
        sarary += people[id].fare();
    }
    return sarary;
}

void logPersonInfo(const vector<Person>& people) {
    for (const auto &p : people) {
        logFile << "ID: " << p.id 
                << ", Home: (" << p.sx << ", " << p.sy << ")"
                << ", Work: (" << p.tx << ", " << p.ty << ")"
                << ", Ekix/Ekiy: (" << p.ekix << ", " << p.ekiy << ")"
                << ", Aimx/Aimy: (" << p.aimx << ", " << p.aimy << ")"
                << ", Ekicellid: " << p.ekicellid 
                << ", Aimcellid: " << p.aimcellid 
                << ", cango: " << (p.cango() ? "true" : "false")
                << ", Fare: " << p.fare()
                << "\n";
    }
}

void input(){
    cin >> N >> M >> K >> T;
    people.resize(M);
    for (int i = 0; i < M; i++){
        cin >> people[i].sx >> people[i].sy 
            >> people[i].tx >> people[i].ty;
        people[i].id = i;
    }
}
 
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    input();

    auto start_time = chrono::steady_clock::now();
    const int TIME_LIMIT_MS = 2700; // とりあえず妥協
 
    vector<GridCell> gridCells;
    vector<pair<int,int>> representatives = getRepresentatives(N, people, gridCells);
    //vector<pair<int,int>> representatives = getRepresentatives2(N, people, gridCells);

    int saraly = 0;
    int turn = 0;
    int ekicount = 0;
    int cellcount = N / cellsize;
    int gridcount = int(gridCells.size());
    int findpathcount = 0;
    vector<int> gridCellIndex;    // 駅を建設したグリッドセルのインデックス
    vector<bool>gridCellIndexBool(gridcount, false);
    map<int,set<int>> homeindex;        // 家側接続のユーザID
    map<int,set<int>> workplaceindex;   // 職場側接続のユーザID
    vector<int> stationorder;
    dsu gridcell(gridcount);

    // for (int i = 0; i < gridcount; i++) {
    //     homeindex[i] = set<int>(gridCells[i].homeStationUserIds.begin(), gridCells[i].homeStationUserIds.end());
    //     workplaceindex[i] = set<int>(gridCells[i].workStationUserIds.begin(), gridCells[i].workStationUserIds.end());
    // }

    for (int i = 0; i < gridCells.size(); i++) {
        for (int id : gridCells[i].homeStationUserIds) {
            homeindex[i].insert(id);
        }
        for (int id : gridCells[i].workStationUserIds) {
            workplaceindex[i].insert(id);
        }
    }

    for(int i=0;i<gridcount;i++){
        for(auto id:gridCells[i].homeStationUserIds){
            people[id].ekicellid = i;
        }
        for(auto id:gridCells[i].workStationUserIds){
            people[id].aimcellid = i;
        }
    }
 
    // グリッド状態（0:更地, 1:線路, 2:駅）
    vector<vector<int>> grid(N, vector<int>(N, EMPTY));
 
    // 候補セル（家の近く）の情報作成
    vector<vector<vector<int>>> candidate(N, vector<vector<int>>(N));
    for (int i = 0; i < M; i++){
        for (int di = -2; di <= 2; di++){
            for (int dj = -2; dj <= 2; dj++){
                int r = people[i].sx + di;
                int c = people[i].sy + dj;
                if(r < 0 || r >= N || c < 0 || c >= N) continue;
                if(abs(di) + abs(dj) <= 2)
                    candidate[r][c].push_back(i);
            }
        }
    }
    vector<pair<int, pair<int,int>>> order2;
    for (int i = 0; i < N; i++){
        for (int j = 0; j < N; j++){
            order2.push_back({ int(candidate[i][j].size()), {i, j} });
        }
    }
    sort(order2.begin(), order2.end(), greater<>());
 
    // 資金とアクションの初期化
    int funds = K;
    vector<string> actions;
    vector<string> bestaction;
    actions.reserve(T);
    bool finishmakestation = false;

    //初期状態保存
    auto saveInitialState = [&]() -> void {
        initialState.people = people;
        initialState.gridCells = gridCells;
        initialState.funds = funds;
        initialState.grid = grid;
        initialState.gridcell = gridcell;
        initialState.turn = turn;
        initialState.ekicount = ekicount;
        initialState.gridVersion = gridVersion;
        initialState.gridCellIndex = gridCellIndex;
        initialState.gridCellIndexBool = gridCellIndexBool;
        initialState.homeindex = homeindex;
        initialState.workplaceindex = workplaceindex;
    };

    auto resetSimulationState = [&]() -> void {
        people = initialState.people;
        gridCells = initialState.gridCells;
        funds = initialState.funds;
        grid = initialState.grid;
        // DSU は各セルが独立している状態に再構築
        gridcell = initialState.gridcell;
        turn = initialState.turn;
        ekicount = initialState.ekicount;
        gridVersion = initialState.gridVersion;
        gridCellIndex = initialState.gridCellIndex;
        gridCellIndexBool = vector<bool>(gridcount, false);
        homeindex = initialState.homeindex;
        workplaceindex = initialState.workplaceindex;
    };

    auto logUnionFindState = [&]()-> void{
        logFile << "DSU state:" << "\n";
        int n = gridcount;
        for (int i = 0; i < n; i++){
            logFile << "Cell " << i << ": leader = " << gridcell.leader(i) << endl;
        }
        logFile << "HomeIndex:" << "\n";
        for (auto &p : homeindex) {
        logFile << "Leader " << p.first << ": ";
        for (int id : p.second) logFile << id << " ";
        logFile << "\n";
        }
        logFile << "WorkIndex:" << "\n";
        for (auto &p : workplaceindex) {
        logFile << "Leader " << p.first << ": ";
        for (int id : p.second) logFile << id << " ";
        logFile << "\n";
        }
    };

    auto mergeGroups = [&](int a, int b) -> void {
        // DSU の merge を行い、リーダー L を取得する
        int L = gridcell.merge(a, b);
        // a, b の各グループのユーザー集合を L に統合する
        homeindex[L].insert(all(homeindex[a]));
        homeindex[L].insert(all(homeindex[b]));
        workplaceindex[L].insert(all(workplaceindex[a]));
        workplaceindex[L].insert(all(workplaceindex[b]));
        if(a != L) {
            homeindex.erase(a);
            workplaceindex.erase(a);
        }
        if(b != L) {
            homeindex.erase(b);
            workplaceindex.erase(b);
        }
    };

    // DSU の最新のリーダーを各 Person に反映する
    auto updatePersons = [&]() -> void {
        for (auto &p : people) {
            if (p.ekicellid != -1) {
                p.ekicellid = gridcell.leader(p.ekicellid);
            }
            if (p.aimcellid != -1) {
                p.aimcellid = gridcell.leader(p.aimcellid);
            }
        }
    };

    // ----- 駅・線路建設用ラムダ関数 -----
    auto buildStation = [&](int r, int c, vector<int> personshome, vector<int> personwork, int cellid, int fromid) -> bool {
        if ((int)actions.size() >= T) return false;
        logFile << "turn :" << turn << ' ' << saraly << " funds: " << funds << endl;
        if (funds < COST_STATION) {
            actions.push_back("-1");
            turn++;
            return false;
        }
        if (grid[r][c] != EMPTY) {
            actions.push_back("-1");
            turn++;
            return false;
        }
        grid[r][c] = STATION;
        funds -= COST_STATION;

        if(fromid != -1) {
            gridcell.merge(cellid, fromid);
            mergeGroups(cellid, fromid);
            updatePersons();
        }

        gridVersion++;
        for (auto &p : personshome) {
            people[p].ekix = r;
            people[p].ekiy = c;
            people[p].ekicellid = gridcell.leader(cellid);
        }
        for (auto &p : personwork) {
            people[p].aimx = r;
            people[p].aimy = c;
            people[p].aimcellid = gridcell.leader(cellid);
        }
        //logUnionFindState();
        vector<int> cangoList = cangolist(people);

        actions.push_back("0 " + to_string(r) + " " + to_string(c));
        turn++;
        stationorder.push_back(cellid);
        ekicount++;

        return true;
    };
 
    auto buildRail = [&](int r, int c, int p, Person &person) -> bool {
        if ((int)actions.size() >= T) return false;
        logFile << "turn :" << turn << ' ' << saraly << " funds: " << funds << endl;
        if (funds < COST_RAIL) {
            actions.push_back("-1");
            turn++;
            return false;
        }
        if (grid[r][c] != EMPTY) {
            actions.push_back("-1");
            turn++;
            return false;
        }
        grid[r][c] = RAIL;
        funds -= COST_RAIL;
        gridVersion++;
        actions.push_back(to_string(p) + " " + to_string(r) + " " + to_string(c));
        turn++;
        return true;
    };
    
    
    auto findPath = [&](int sx, int sy, int tx, int ty) -> vector<pair<int,int>> {
        vector<vector<bool>> visited(N, vector<bool>(N, false));
        vector<vector<pair<int,int>>> parent(N, vector<pair<int,int>>(N, {-1, -1}));
        queue<pair<int,int>> q;
        visited[sx][sy] = true;
        q.push({sx, sy});
        bool found = false;
 
        while (!q.empty()){
            auto [cx, cy] = q.front();
            q.pop();
            if (cx == tx && cy == ty) {
                found = true;
                break;
            }
            for (int i = 0; i < 4; i++){
                int nx = cx + dx[i], ny = cy + dy[i];
                if (nx < 0 || nx >= N || ny < 0 || ny >= N) continue;
                if (!visited[nx][ny] && grid[nx][ny] == EMPTY) {
                    visited[nx][ny] = true;
                    parent[nx][ny] = {cx, cy};
                    q.push({nx, ny});
                }
            }
        }
        if (!found) return vector<pair<int,int>>{};
 
        vector<pair<int,int>> path;
        int cx = tx, cy = ty;
        while (!(cx == sx && cy == sy)) {
            path.push_back({cx, cy});
            auto [px, py] = parent[cx][cy];
            cx = px; cy = py;
        }
        path.push_back({sx, sy});
        reverse(path.begin(), path.end());
        findpathcount++;
        logFile << "findpathcount" << findpathcount << endl;
        return path;
    };

    struct CellPairKey {
        int i, j;
        bool operator==(const CellPairKey &other) const {
            return i == other.i && j == other.j;
        }
    };
    
    struct CellPairHash {
        size_t operator()(const CellPairKey &key) const {
            return ((size_t)key.i * 1000003ULL) ^ ((size_t)key.j);
        }
    };
    
    struct CachedPath {
        int version;
        vector<P> path;
    };
    
    unordered_map<CellPairKey, CachedPath, CellPairHash> pathCache;
    
    auto findPathcell = [&] (int startcellid, int endcellid) -> vector<pair<int,int>> {
        CellPairKey key = {startcellid, endcellid};
        if (pathCache.find(key) != pathCache.end()) {
            // キャッシュのバージョンが現在の `gridVersion` と一致していればキャッシュを使用
            if (pathCache[key].version == gridVersion) {
                return pathCache[key].path;
            }
        }    
        // 一致しない場合、経路を再計算
        int sx = gridCells[startcellid].x;
        int sy = gridCells[startcellid].y;
        int tx = gridCells[endcellid].x;
        int ty = gridCells[endcellid].y;
        vector<pair<int,int>> path = findPath(sx, sy, tx, ty);
 
        // キャッシュを更新（新しいバージョンをセット）
        pathCache[key] = {gridVersion, path};
        return path;
    };
    
    //駅建設コスト
    auto calcConstCost = [&](int cellaid, int cellbid) -> int {
        vector<pair<int,int>> path = findPathcell(cellaid, cellbid);
        return COST_STATION * 2 + COST_RAIL * (int)path.size();
    };

    //ネットワーク最初の二つの駅を建設
    auto buildinitpair = [&]() -> void{
        pair<int,int> bestconnect = {-1, -1};
    int bestsarary = 0;
    //logPersonInfo(people);
    for (int i = 0; i < gridcount; i++){
        if(gridCellIndexBool[i]) continue;
        for (int j = i+1; j < gridcount; j++){
            if(gridCellIndexBool[j]) continue;
            if (abs(gridCells[i].x - gridCells[j].x) < cellsize && abs(gridCells[i].y - gridCells[j].y) < cellsize) {
                continue;
            }
            int calcFare = calculateFareIncrease(gridCells[i], gridCells[j], people);
            //logFile << "id: " << i << ' ' << j << ' ' << calcFare << endl;
            if (calcFare > bestsarary && (gridCellIndex.size() != 0 || funds >= calcConstCost(i,j))){
                if(calcConstCost(i,j) > calcFare * (T-turn)) continue;
                bestsarary = calcFare;
                bestconnect = {i, j};
            }
        }
    }
    if(bestconnect == make_pair(-1, -1)) {
        finishmakestation = true;
        return;
    }
    logFile <<"id:" << bestconnect.first << ' ' << bestconnect.second <<"bestsarary" << bestsarary << endl;
    gridCellIndex.push_back(bestconnect.first);
    gridCellIndex.push_back(bestconnect.second);
    gridCellIndexBool[bestconnect.first] = true;
    gridCellIndexBool[bestconnect.second] = true;
 
    int sx0 = gridCells[bestconnect.first].x;
    int tx0 = gridCells[bestconnect.second].x;
    int sy0 = gridCells[bestconnect.first].y;
    int ty0 = gridCells[bestconnect.second].y;
 
    bool finish3 = false;
    while (!finish3) {
        if (turn >= endtime) break;
        if (!buildStation(sx0, sy0, gridCells[bestconnect.first].homeStationUserIds,
                          gridCells[bestconnect.first].workStationUserIds, bestconnect.first, -1)) {
        } else {
            finish3 = true;
            gridCells[bestconnect.first].ekiconstructed = true;
        }
        if ((int)actions.size() >= T) {
            finish3 = true;
        }
        funds += saraly;
    }
 
    auto path = findPathcell(bestconnect.first, bestconnect.second);
    if (!path.empty()) {
        for (int pi = 1; pi < (int)path.size()-1; pi++){
            if ((int)actions.size() >= T) break;
            auto prev = path[pi-1];
            auto cur  = path[pi];
            auto next = path[pi+1];
 
            int movex1 = cur.first - prev.first;
            int movey1 = cur.second - prev.second;
            int movex2 = next.first - cur.first;
            int movey2 = next.second - cur.second;
            int rtype = getRailType(movex1, movey1, movex2, movey2);
 
            if (!buildRail(cur.first, cur.second, rtype, people[0])) {
                pi--;
            }
            funds += saraly;
        }
    }
 
    bool finish4 = false;
    while (!finish4) {
        if (!buildStation(tx0, ty0, gridCells[bestconnect.second].homeStationUserIds,
                          gridCells[bestconnect.second].workStationUserIds, bestconnect.second, bestconnect.first)) {
        } else {
            finish4 = true;
            gridCells[bestconnect.second].ekiconstructed = true;
            int cellid = gridcell.leader(bestconnect.first);
            for (auto id : gridCells[bestconnect.second].workStationUserIds)
                workplaceindex[cellid].insert(id);
            for (auto id : gridCells[bestconnect.first].workStationUserIds)
                workplaceindex[cellid].insert(id);
            for (auto id : gridCells[bestconnect.second].homeStationUserIds)
                homeindex[cellid].insert(id);
            for (auto id : gridCells[bestconnect.first].homeStationUserIds)
                homeindex[cellid].insert(id);
            saraly = calcsarary(cangolist(people), people);
            gridcell.merge(bestconnect.first, bestconnect.second);
        }
        funds += saraly;
        if ((int)actions.size() >= T)
            finish4 = true;
    }
    };


    auto buildnetwork = [&]() ->void{
        bool finish5 = false;
    while (!finish5) {
        //logPersonInfo(people);
        auto current_time = chrono::steady_clock::now();
        int elapsed_ms = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
        if (elapsed_ms > TIME_LIMIT_MS) {
            logFile << "Timeout in finish5 loop at turn: " << turn << "\n";
            break;
        }
        int bestsarary2 = 0;
        int bestconnect2 = -1;
        
        for (auto id : gridCellIndex) {
            for (int i = 0; i < gridcount; i++){
                if (i == id) continue;
                if (gridCellIndexBool[i]) continue;
                if (abs(gridCells[id].x - gridCells[i].x) < cellsize && abs(gridCells[id].y - gridCells[i].y) < cellsize) {
                    continue;
                }
                //if(grid)
                int fromid = gridcell.leader(id);
                int toid = gridcell.leader(i);
                int calcFare = calcSecondFareIncreaseCost(fromid,toid,homeindex,workplaceindex,people);
                logFile << "id: " << id << ' ' <<i << ' ' <<  calcFare << endl;
                // 残りターンに応じて寛容度を調整
            // 残りターンが少ないほど大きくして、必要資金をさらに下げる
            double toleranceFactor = 1.5 - (double)(turn) / endtime;
            if (calcFare > bestsarary2 
                && funds + saraly * (endtime - turn) >= calcConstCost(id,i) - COST_STATION * toleranceFactor
                && grid[gridCells[i].x][gridCells[i].y] == EMPTY) {
                bestsarary2 = calcFare;
                bestconnect2 = i;
            }
            }
        }
        logFile << "nextstation" << bestconnect2 << endl;
        logFile << "nextpoint " << gridCells[bestconnect2].x << ' ' << gridCells[bestconnect2].y << endl;
        if(bestconnect2 == -1) break;
        if (bestconnect2 != -1) {
            int startindex;
            int cost = 1000000000;
            for (auto gridindex : gridCellIndex) {
                int tempCost = calcConstCost(gridindex, bestconnect2)-COST_STATION;
                if (tempCost < cost){
                    if (findPathcell(gridindex, bestconnect2).empty()) continue;{
                        cost = tempCost;
                        startindex = gridindex;
                    }
                }
            }
            int sx2 = gridCells[startindex].x;
            int tx2 = gridCells[bestconnect2].x;
            int sy2 = gridCells[startindex].y;
            int ty2 = gridCells[bestconnect2].y;
 
            auto path2 = findPathcell(startindex, bestconnect2);
            if(path2.empty()) {
                logFile << "path2 is empty, turn :" << turn << endl;
                break;
            }
            if (!path2.empty()) {
                for (int pi = 1; pi < (int)path2.size()-1; pi++){
                    if ((int)actions.size() >= T) break;
                    auto prev = path2[pi-1];
                    auto cur  = path2[pi];
                    auto next = path2[pi+1];
 
                    int movex1 = cur.first - prev.first;
                    int movey1 = cur.second - prev.second;
                    int movex2 = next.first - cur.first;
                    int movey2 = next.second - cur.second;
                    int rtype = getRailType(movex1, movey1, movex2, movey2);
 
                    if (!buildRail(cur.first, cur.second, rtype, people[0])) {
                        pi--;
                    }
                    funds += saraly;
                }
            }
 
            bool finish6 = false;
            while (!finish6) {
                if (!buildStation(tx2, ty2, gridCells[bestconnect2].homeStationUserIds,
                                  gridCells[bestconnect2].workStationUserIds, bestconnect2, startindex)) {
                    // 失敗時ループ継続
                } else {
                    finish6 = true;
                    gridCells[bestconnect2].ekiconstructed = true;
                    for (auto id : gridCells[bestconnect2].workStationUserIds)
                        workplaceindex[gridcell.leader(startindex)].insert(id);
                    for (auto id : gridCells[bestconnect2].homeStationUserIds)
                        homeindex[gridcell.leader(startindex)].insert(id);
                    gridCellIndex.push_back(bestconnect2);
                    gridCellIndexBool[bestconnect2] = true;
                    saraly = calcsarary(cangolist(people), people);
                }
                funds += saraly;
                if ((int)actions.size() >= T)
                    finish6 = true;
            }
        } 
        if((int)actions.size() >= T) {
            finish5 = true;
        }
    }
    };

    auto buildbeamnetwork = [&]() -> void {
        const int BEAM_WIDTH = 5; // ビーム幅の設定（例えば5）
        bool finish5 = false;
        
        while (!finish5) {
            auto current_time = chrono::steady_clock::now();
            int elapsed_ms = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
            if (elapsed_ms > TIME_LIMIT_MS) {
                logFile << "Timeout in finish5 loop at turn: " << turn << "\n";
                break;
            }
    
            // すべての候補駅を収集
            vector<pair<int, int>> candidateStations; // (運賃増加, 駅ID)
            for (auto id : gridCellIndex) {
                for (int i = 0; i < gridcount; i++) {
                    if (i == id || gridCellIndexBool[i]) continue;
    
                    int fromid = gridcell.leader(id);
                    int toid = gridcell.leader(i);
                    int calcFare = calcSecondFareIncreaseCost(fromid, toid, homeindex, workplaceindex, people);
                    
                    // コスト条件を考慮
                    double toleranceFactor = 1.5 - (double)(turn) / endtime;
                    if (funds + saraly * (endtime - turn) >= calcConstCost(id, i) - COST_STATION * toleranceFactor) {
                        candidateStations.push_back({calcFare, i});
                    }
                }
            }
    
            // 運賃増加が大きい上位 `BEAM_WIDTH` 件を選択
            sort(candidateStations.rbegin(), candidateStations.rend()); // 降順ソート
            candidateStations.resize(min((int)candidateStations.size(), BEAM_WIDTH));
    
            // ビームサーチの試行
            int bestChoice = -1;
            int bestScore = -1;
            vector<string> bestActions;
            saveInitialState();
            for (auto [fareIncrease, stationID] : candidateStations) {
                resetSimulationState();
                
                int startindex;
                int minCost = 1e9;
                for (auto gridindex : gridCellIndex) {
                    int tempCost = calcConstCost(gridindex, stationID) - COST_STATION;
                    if (tempCost < minCost && !findPathcell(gridindex, stationID).empty()) {
                        minCost = tempCost;
                        startindex = gridindex;
                    }
                }
    
                // 経路探索
                auto path = findPathcell(startindex, stationID);
                if (path.empty()) continue;
    
                // シミュレーション実行
                vector<string> tempActions = actions;
                int tempFunds = funds;

                // 評価
                int newScore = funds;
                if (newScore > bestScore) {
                    bestScore = newScore;
                    bestChoice = stationID;
                    bestActions = actions;
                }
    
                actions = tempActions; // シミュレーション前の状態に戻す
                funds = tempFunds;
            }
    
            // 最適なものを選んで実行
            if (bestChoice == -1) break;
            actions = bestActions;
            gridCellIndex.push_back(bestChoice);
            gridCellIndexBool[bestChoice] = true;
    
            // 建設完了処理
            gridCells[bestChoice].ekiconstructed = true;
            for (auto id : gridCells[bestChoice].workStationUserIds)
                workplaceindex[gridcell.leader(bestChoice)].insert(id);
            for (auto id : gridCells[bestChoice].homeStationUserIds)
                homeindex[gridcell.leader(bestChoice)].insert(id);
            saraly = calcsarary(cangolist(people), people);
    
            if ((int)actions.size() >= T) finish5 = true;
        }
    };
    
    

    saveInitialState();
    int bestfund = 0;
    

    // 回数を無理やりしているので、時間が余裕があれば修正する
    //for(int i=1;i>0;i++){
         actions.clear();
         resetSimulationState();
         //logPersonInfo(people);
     while(gridCellIndex.size() < 8) {
        buildinitpair();
        buildnetwork();

        if(finishmakestation) break;
        // auto current_time = chrono::steady_clock::now();
        // int elapsed_ms = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
        // if (elapsed_ms > TIME_LIMIT_MS) {
        //     logFile << "Timeout in finish5 loop at turn: " << turn << "\n";
        //     break;
        // }
    }
    // ----- 後続アクション：ターン上限まで待機 ("-1") -----
    while ((int)actions.size() < T) {
        actions.push_back("-1");
    }
    if(funds > bestfund){
        bestfund = funds;
        bestaction = actions;
        initialState = initialState;
    }
    //}
    //}
    // 出力
    for (int i = 0; i < T; i++){
        cout << bestaction[i] << "\n";
    }

    return 0;
}

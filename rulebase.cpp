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
static const int cellsize     = 5;


ofstream logFile("log.txt");

// ----- 人の情報 -----
struct Person {
    int id;
    int sx, sy;    // 家
    int tx, ty;    // 職場
    int ekix, ekiy;  // 家側最寄り駅
    int aimx, aimy;  // 職場側最寄り駅
    bool cango = false; // 職場まで行けるか

    int fare() {
        return abs(sx - tx) + abs(sy - ty);
    }
    Person() { ekix = -1; ekiy = -1; aimx = -1; aimy = -1; }
};

// ----- グリッドセルの情報 ----- 
struct GridCell {
    int x, y;
    vector<int> homeStationUserIds;
    vector<int> workStationUserIds;
    bool ekiconstructed = false;
};

// ----- 2セル間の運賃・コスト計算 -----
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


int calcSecondFareIncreaseCost(const GridCell &cellA,set<int> homeid, set<int> workid, vector<Person> &people) {
    int totalFare = 0;
    unordered_set<int> workB(workid.begin(), workid.end());
    for (int userId : cellA.homeStationUserIds) {
        if (workB.find(userId) != workB.end() && !people[userId].cango)
            totalFare += people[userId].fare();
    }
    unordered_set<int> workA(cellA.workStationUserIds.begin(), cellA.workStationUserIds.end());
    for (int userId : homeid) {
        if (workA.find(userId) != workA.end() && !people[userId].cango)
            totalFare += people[userId].fare();
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
    int blockSize = 5;
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

vector<int> cangolist(set<int> homeindex, set<int> workplaceindex, vector<Person> &people) {
    vector<int> cangolist;
    for(auto id : homeindex) {
        if(find(workplaceindex.begin(), workplaceindex.end(), id) != workplaceindex.end()){
        cangolist.push_back(id);
        people[id].cango = true;
        }
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
 
// ----- メイン -----
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
 
    int N, M, K, T;
    cin >> N >> M >> K >> T;

    auto start_time = chrono::steady_clock::now();
    const int TIME_LIMIT_MS = 2700; // とりあえず妥協
 
    vector<Person> people(M);
    for (int i = 0; i < M; i++){
        cin >> people[i].sx >> people[i].sy 
            >> people[i].tx >> people[i].ty;
        people[i].id = i;
    }
 
    // 代表セルを抽出（ここでは代表セル群とともにGridCellの情報を生成）
    vector<GridCell> gridCells;
    vector<pair<int,int>> representatives = getRepresentatives(N, people, gridCells);

    int saraly = 0;
    int turn = 0;
    int ekicount = 0;
    int cellcount = N / cellsize;
    int gridcount = int(gridCells.size());
    int findpathcount = 0;
    vector<int> gridCellIndex;    // 駅を建設したグリッドセルのインデックス
    vector<bool>gridCellIndexBool(gridcount, false);
    set<int> homeindex;        // 家側接続のユーザID
    set<int> workplaceindex;   // 職場側接続のユーザID
    dsu gridcell(gridcount);
 
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
    actions.reserve(T);
 
    // (マンハッタン距離, index) で乗客を降順にソート
    vector<pair<int,int>> order;
    for (int i = 0; i < M; i++){
        int dist = abs(people[i].sx - people[i].tx) + abs(people[i].sy - people[i].ty);
        order.push_back({dist, i});
    }
    sort(order.begin(), order.end());
    reverse(order.begin(), order.end());
 
    // ----- 駅・線路建設用ラムダ関数 -----
    auto buildStation = [&](int r, int c, vector<int> personshome, vector<int> personwork) -> bool {
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
        for (auto &p : personshome) {
            people[p].ekix = r;
            people[p].ekiy = c;
            //if(people[p].aimx!=-1 && people[p].aimy!=-1) people[p].cango = true;
        }
        for (auto &p : personwork) {
            people[p].aimx = r;
            people[p].aimy = c;
            //if(people[p].ekix!=-1 && people[p].ekiy!=-1) people[p].cango = true;
        }
        vector<int> cangoList = cangolist(homeindex, workplaceindex, people);
        for(auto c:cangoList){
            people[c].cango = true;
        }
        actions.push_back("0 " + to_string(r) + " " + to_string(c));
        turn++;
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
//       // gridcount は候補セルの総数とする
// vector<vector<vector<P>>> pathiddict(gridcount, vector<vector<P>>(gridcount));
// vector<vector<bool>> pathdict(gridcount, vector<bool>(gridcount, false));

// auto findPathcell = [&] (int startcellid, int endcellid) -> vector<pair<int,int>> {
//     if(pathdict[startcellid][endcellid]) return pathiddict[startcellid][endcellid];
//     int sx = gridCells[startcellid].x;
//     int sy = gridCells[startcellid].y;
//     int tx = gridCells[endcellid].x;
//     int ty = gridCells[endcellid].y;
//     vector<pair<int,int>> path = findPath(sx, sy, tx, ty);
//     pathdict[startcellid][endcellid] = true;
//     pathiddict[startcellid][endcellid] = path;
//     return path;
// };

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
    
    unordered_map<CellPairKey, vector<P>, CellPairHash> pathCache;
    
    auto findPathcell = [&] (int startcellid, int endcellid) -> vector<pair<int,int>> {
        CellPairKey key = {startcellid, endcellid};
        if(pathCache.find(key) != pathCache.end()){
            return pathCache[key];
        }
        int sx = gridCells[startcellid].x;
        int sy = gridCells[startcellid].y;
        int tx = gridCells[endcellid].x;
        int ty = gridCells[endcellid].y;
        vector<pair<int,int>> path = findPath(sx, sy, tx, ty);
        pathCache[key] = path;
        return path;
    };
    


    //駅建設コスト
    auto calcConstCost = [&](const GridCell &cellA, const GridCell &cellB) -> int {
        vector<pair<int,int>> path = findPath(cellA.x, cellA.y, cellB.x, cellB.y);
        return COST_STATION * 2 + COST_RAIL * (int)path.size();
    };

    // int roopcount = 0;
    // while(roopcount < 3){
    //     pathCache.clear();
    // ----- メインの接続処理 -----
    // 最初に、候補セル order2 の中から、ある程度資金が足りる状態で駅建設可能なセルを探す
    pair<int,int> bestconnect = {-1, -1};
    int bestsarary = 0;
    for (int i = 0; i < gridcount; i++){
        if(gridCellIndexBool[i]) continue;
        for (int j = i+1; j < gridcount; j++){
            if(gridCellIndexBool[j]) continue;
            int calcFare = calculateFareIncrease(gridCells[i], gridCells[j], people);
            if (calcFare > bestsarary && funds >= calcConstCost(gridCells[i], gridCells[j])){
                bestsarary = calcFare;
                bestconnect = {i, j};
            }
        }
    }
    logFile << "bestsarary" << bestsarary << endl;
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
                          gridCells[bestconnect.first].workStationUserIds)) {
            // 失敗時はループ継続
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
                          gridCells[bestconnect.second].workStationUserIds)) {
            // 失敗時ループ継続
        } else {
            finish4 = true;
            gridCells[bestconnect.second].ekiconstructed = true;
            for (auto id : gridCells[bestconnect.second].workStationUserIds)
                workplaceindex.insert(id);
            for (auto id : gridCells[bestconnect.first].workStationUserIds)
                workplaceindex.insert(id);
            for (auto id : gridCells[bestconnect.second].homeStationUserIds)
                homeindex.insert(id);
            for (auto id : gridCells[bestconnect.first].homeStationUserIds)
                homeindex.insert(id);
            saraly = calcsarary(cangolist(homeindex, workplaceindex,people), people);
            gridcell.merge(bestconnect.first, bestconnect.second);
        }
        funds += saraly;
        if ((int)actions.size() >= T)
            finish4 = true;
    }
 
    // ----- 2回目以降の接続 -----
    bool finish5 = false;
    while (!finish5) {
        //logFile << "cangolist" << endl;
        auto current_time = chrono::steady_clock::now();
        int elapsed_ms = chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count();
        if (elapsed_ms > TIME_LIMIT_MS) {
            logFile << "Timeout in finish5 loop at turn: " << turn << "\n";
            break;
        }
        for(auto id : cangolist(homeindex, workplaceindex, people)) {
            //logFile << id << ' '<<  endl;
        }
        //logFile << "people" << endl;
        // for(auto &p: people) {
        //     if(p.cango) logFile << p.id << ' ' << endl;
        // }
        int bestsarary2 = 0;
        int bestconnect2 = -1;
        
        for (auto id : gridCellIndex) {
            for (int i = 0; i < gridcount; i++){
                if (i == id) continue;
                if (gridCellIndexBool[i]) continue;
                //if(grid)
                int calcFare = calcSecondFareIncreaseCost(gridCells[i], homeindex, workplaceindex, people);
                // if (calcFare > bestsarary2 && funds + saraly * (endtime - turn) >= calcConstCost(gridCells[id], gridCells[i])- COST_STATION
                //     && grid[gridCells[i].x][gridCells[i].y] == EMPTY) {
                //     bestsarary2 = calcFare;
                //     bestconnect2 = i;
                // }
                // 残りターンに応じて寛容度を調整
            // 残りターンが少ないほど大きくして、必要資金をさらに下げる
            double toleranceFactor = 1.5 - (double)(turn) / endtime;
            if (calcFare > bestsarary2 
                && funds + saraly * (endtime - turn) >= calcConstCost(gridCells[id], gridCells[i]) - COST_STATION * toleranceFactor
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
                int tempCost = calcConstCost(gridCells[gridindex], gridCells[bestconnect2])-COST_STATION;
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
                                  gridCells[bestconnect2].workStationUserIds)) {
                    // 失敗時ループ継続
                } else {
                    finish6 = true;
                    gridCells[bestconnect2].ekiconstructed = true;
                    for (auto id : gridCells[bestconnect2].workStationUserIds)
                        workplaceindex.insert(id);
                    for (auto id : gridCells[bestconnect2].homeStationUserIds)
                        homeindex.insert(id);
                    gridCellIndex.push_back(bestconnect2);
                    gridCellIndexBool[bestconnect2] = true;
                    saraly = calcsarary(cangolist(homeindex, workplaceindex, people), people);
                }
                funds += saraly;
                if ((int)actions.size() >= T)
                    finish6 = true;
            }
        } 
        if((int)actions.size() >= T) {
            finish5 = true;
        }
        // ここでループを抜ける条件（例：一定条件を満たす）
        //finish5 = true;  // この例では1回のみ実施（内容は変えずに残す）
    }
    // if(gridCellIndex.size()>= cellcount/2)break;
    // if((int)actions.size() >= T) break;
    // roopcount++;
    // }

    // ----- 後続アクション：ターン上限まで待機 ("-1") -----
    while ((int)actions.size() < T) {
        actions.push_back("-1");
    }
 
    // 出力
    for (int i = 0; i < T; i++){
        cout << actions[i] << "\n";
    }

    return 0;
}

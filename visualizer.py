import os
import matplotlib.pyplot as plt
import matplotlib.animation as animation

# 定数・カラーマップ設定
CELL_EMPTY = 0   # 更地
CELL_RAIL = 1    # 線路
CELL_STATION = 2 # 駅

CELL_COLORS = {
    CELL_EMPTY: (1.0, 1.0, 1.0),  # 白
    CELL_RAIL:  (0.6, 0.6, 0.6),  # グレー
    CELL_STATION: (1.0, 0.0, 0.0) # 赤
}

def read_input_file(path):
    """
    入力ファイルを読み込み、(N, M, K, T) と people情報 を返す
    people: [(sx, sy, tx, ty), ...]
    """
    with open(path, 'r') as f:
        data = f.read().strip().split()
    idx = 0
    N = int(data[idx]); idx+=1
    M = int(data[idx]); idx+=1
    K = int(data[idx]); idx+=1
    T = int(data[idx]); idx+=1
    
    people = []
    for _ in range(M):
        sx = int(data[idx]); idx+=1
        sy = int(data[idx]); idx+=1
        tx = int(data[idx]); idx+=1
        ty = int(data[idx]); idx+=1
        people.append((sx, sy, tx, ty))

    return N, M, K, T, people

def read_output_file(path, T):
    """
    出力ファイル(T行分)を読み込み、行動リスト(文字列の配列)を返す。
    行数が足りない場合や多い場合の処理は割愛。
    """
    actions = []
    with open(path, 'r') as f:
        lines = f.read().strip().split('\n')
    # 先頭 T 行を利用 (Tより多い行があっても切り捨て)
    # 不足時は-1(待機)で埋めるなど方法はあるが、ここでは単純に想定通りとする
    for i in range(T):
        if i < len(lines):
            actions.append(lines[i].strip())
        else:
            actions.append("-1")  # なければ待機扱い
    return actions

def create_animation(N, T, actions, save_path):
    """
    グリッドを初期化し、各ターンの行動を反映しながらアニメーションを作成・保存。
    """
    grid = [[CELL_EMPTY]*N for _ in range(N)]
    
    fig, ax = plt.subplots()
    ims = []

    for turn in range(T):
        line_str = actions[turn]
        elems = line_str.split()
        
        if elems[0] == "-1":
            # 待機
            pass
        else:
            p = int(elems[0])   # 0なら駅、1-6なら線路の種類
            i = int(elems[1])
            j = int(elems[2])
            if p == 0:
                # 駅を建設
                grid[i][j] = CELL_STATION
            else:
                # 線路 (種類によらず同一表示)
                grid[i][j] = CELL_RAIL
        
        # グリッドをカラー形式に変換
        color_grid = []
        for r in range(N):
            row_color = []
            for c in range(N):
                row_color.append(CELL_COLORS[grid[r][c]])
            color_grid.append(row_color)
        
        im = plt.imshow(color_grid, animated=True)
        title = plt.text(0.5, 1.01, f"Turn {turn+1}", ha="center", va="bottom",
                         transform=ax.transAxes, fontsize="large")
        ims.append([im, title])
    
    ani = animation.ArtistAnimation(fig, ims, interval=500, blit=True)

    # 軸ラベルや目盛りなど不要にする
    plt.axis('off')
    plt.title("Nihonbashi Simulator Visualization")

    # GIFを保存 (pillow必須)
    ani.save(save_path, writer="pillow")
    plt.close(fig)  # メモリ解放

def main():
    # 可視化結果を格納するフォルダを作成（なければ作る）
    os.makedirs("./tools/vis", exist_ok=True)
    
    # ./tools/in/ から .txt ファイルを全て取得
    in_files = sorted([f for f in os.listdir("./tools/in") if f.endswith(".txt")])
    
    for in_file in in_files:
        # 対応する出力ファイルが ./tools/out にあるか確認
        base_name = os.path.splitext(in_file)[0]  # "0000" など
        out_file = base_name + ".txt"
        in_path = os.path.join("./tools/in", in_file)
        out_path = os.path.join("./tools/out", out_file)
        if not os.path.exists(out_path):
            print(f"Output file {out_file} not found, skip.")
            continue
        
        # 入力読み込み
        N, M, K, T, people = read_input_file(in_path)
        # 出力読み込み
        actions = read_output_file(out_path, T)
        
        # GIF保存先パス
        save_gif = os.path.join("./tools/vis", f"visualize_{base_name}.gif")
        
        print(f"Visualizing {base_name} ...")
        create_animation(N, T, actions, save_gif)
        print(f"Saved GIF to {save_gif}")

if __name__ == "__main__":
    main()

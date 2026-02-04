// =============================================
// File: GameScene.cpp （置き換え）
// =============================================
#include "GameScene.h"
#include "kamataEngine.h"
#include <imgui.h>
using namespace KamataEngine;
#include "StageEditor.h"
#include "StageEditorView.h"
#include "./stage/include/math/MathUtil.h"

GameScene::~GameScene() {
	// モデル等はエンジン側のライフサイクルに追従（必要なら delete）
}

void GameScene::Initialize() {
	// --- モデル ---
	cubeModel_ = Model::CreateFromOBJ("cube", true); // 無ければ Model::Create() に差し替え

	// --- カメラ（真上固定風） ---
	camera_.Initialize();
	camera_.farZ = 2000.0f; // 広め
	camera_.nearZ = 0.1f;

	// ※ あなたの Camera に LookAt/Orthographic セッタがあれば、ここで「真上固定」にしてください。
	// 未対応の場合でも、モデル配置を原点付近＆Zも使っていれば見えるはず。必要なら後で微調整。
	// KamataEngine の Camera が position/rotation を直接持つタイプ想定
	camera_.translation_ = {0.0f, CAM_H, 0.0f};
	// X軸に -90° 回転（上から地面(XZ)を覗く）
	camera_.rotation_ = {-3.14159265f * 0.5f, 0.0f, 0.0f};
	camera_.TransferMatrix(); // ← エンジンの更新メソッド名に合わせて（Update/Reset等ならそちらを）

	// --- マップ（壁を少しだけ配置） ---
	walls_.clear();
	wallWts_.clear();
	wallWts_.reserve(16); // 適当な上限でOK
	auto addWall = [&](float cx, float cz, float sx, float sz) {
		BoxXZ w{};
		w.minx = cx - sx * 0.5f;
		w.maxx = cx + sx * 0.5f;
		w.minz = cz - sz * 0.5f;
		w.maxz = cz + sz * 0.5f;
		walls_.push_back(w); // 座標だけを格納（コピーOK）

		 auto wt = std::make_unique<KamataEngine::WorldTransform>();
		wt->Initialize();
		BuildWallWT(w, *wt);               // 初期行列を設定
		wallWts_.push_back(std::move(wt)); // 永続保持
	};
	addWall(0.0f, 0.0f, 2.0f, 1.0f);
	addWall(0.0f, 3.0f, 2.0f, 1.0f);
	addWall(-3.5f, 0.0f, 1.0f, 4.0f);


	// --- プレイヤー／敵 初期化 ---
	player_.wt.Initialize();
	player_.x = -2.5f;
	player_.z = -2.0f;
	player_.dir4 = 0; // +X 向き
	SyncWT_Tank(player_);

	enemy_.wt.Initialize();
	enemy_.x = +2.5f;
	enemy_.z = +2.0f;
	enemy_.dir4 = 2; // -X 向き
	SyncWT_Tank(enemy_);

	bullets_.clear();

	phase_ = Phase::Reserve;
	selSlot_ = 0;
	execStep_ = execMini_ = 0;

	// GameScene.cpp の Initialize 内
	// タイトル用スプライト
	titleSprite_ = Sprite::Create(TextureManager::Load("title.png"), {0.0f, 0.0f}); // 画面中央（例）

	// クリア用スプライト
	clearSprite_ = Sprite::Create(TextureManager::Load("clear.png"), {0.0f, 0.0f});




	// id→モデル対応を準備（あなたの資産に合わせて）
	tileModels_.clear();
	tileModels_.resize(3);

	// ★ 2. タイルIDとモデルの対応を設定
	// 例: 0=床, 1=壁, 2=木など
	tileModels_[0] = Model::CreateFromOBJ("cube", true);
	tileModels_[1] = Model::CreateFromOBJ("particle", true);
	tileModels_[2] = Model::CreateFromOBJ("cube", true);

	// ★ エディタ側のパレット上限と選択IDを“使用可能ID”へクランプ
	const int maxUsableId = static_cast<int>(tileModels_.size()) - 1;
	editor_.paletteMax = (std::max)(1, maxUsableId); // 1..maxUsableId まで表示
	if (editor_.selectedId > maxUsableId)
		editor_.selectedId = (maxUsableId >= 1 ? 1 : 0);

	// マップサイズとセルの大きさを決める（例: 32x24, 1.0fユニット=1タイル）
	editor_.Initialize(32, 24, 1.0f);

	// 必要なら CSV から初期レイアウトを読み込む（任意）
	editor_.LoadCSV("stage/stage01.csv");

	// LoadCSV の直後か、必要なタイミングで一度だけ実行
	{
		// CSV読み込み後にidをサニタイズ（tileModels_が既にあるのでOK）
		editor_.ForEach(
		    [&](int x, int y, int id) {
			    if (id <= 0)
				    return;
			    if (id >= (int)tileModels_.size() || !tileModels_[id]) {
				    editor_.Set(x, y, 0);
			    }
		    },
		    true);
	}

	tileWT_.Initialize(); // 一度だけ。以降は毎タイルで値を更新→TransferMatrix()
	                      // モデルを用意した後でOK。モデル数に合わせて確保
	                      // デバッグカメラの初期位置（少し斜め上から）
	debugCamera_.Initialize();
	debugCamera_.farZ = 2000.0f;
	debugCamera_.nearZ = 0.1f;
	debugCamera_.translation_ = {5.0f, 8.0f, -5.0f};
	debugCamera_.rotation_ = {-0.6f, 0.7f, 0.0f};
	debugCamera_.TransferMatrix();
}

void GameScene::Update() {

	 // ★ カメラを毎フレーム転送（真上固定）
	camera_.translation_ = {0.0f, CAM_H, 0.0f};
	camera_.rotation_ = {-3.14159265f * 0.5f, 0.0f, 0.0f}; // -90°俯瞰
	camera_.TransferMatrix();


	// 予約 or 実行 or リザルト
	switch (phase_) {
	case Phase::Reserve:
		HandleReserveInput();
		break;
	case Phase::Execute:
		if (TickExecuteMiniFrame()) {
			phase_ = Phase::Result;
		}
		break;
	case Phase::Result:
		// Enter でリスタート（同じマップで再戦）
		if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
			Initialize();
		}
		break;
	}

	// --- ImGui（簡易デバッグUI） ---
	if (ImGui::Begin("Order Tank Debug")) {
		ImGui::Text("Phase: %s", phase_ == Phase::Reserve ? "Reserve" : phase_ == Phase::Execute ? "Execute" : "Result");
		ImGui::Separator();
		ImGui::Text("Player Queue:");
		for (size_t i = 0; i < player_.queue.size(); ++i) {
			ImGui::SameLine();
			ImGui::Text("[%d]=%d", (int)i, (int)player_.queue[i]);
		}
		ImGui::Text("\nEnemy Queue:");
		for (size_t i = 0; i < enemy_.queue.size(); ++i) {
			ImGui::SameLine();
			ImGui::Text("[%d]=%d", (int)i, (int)enemy_.queue[i]);
		}
	}
	ImGui::End();

	//    ※ ここで左ドラッグでペイント、右ドラッグで消しゴム、Save/Load が操作できる
	editor_.UpdateEditorUI("Stage Editor");

	// ImGui のエディタを回した後に安全側でクランプ
	{
		const int maxUsableId = static_cast<int>(tileModels_.size()) - 1;
		if (editor_.paletteMax > maxUsableId)
			editor_.paletteMax = (std::max)(1, maxUsableId);
		if (editor_.selectedId > maxUsableId)
			editor_.selectedId = (maxUsableId >= 1 ? 1 : 0);
	}

	// カメラ切替（Tabキーでトグル）
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		useDebugCamera_ = !useDebugCamera_;
	}

	// デバッグカメラ操作
	if (useDebugCamera_) {
		const float moveSpeed = 0.2f;
		const float rotSpeed = 0.02f;

		// 回転：矢印キー
		if (Input::GetInstance()->PushKey(DIK_UP))
			debugCamera_.rotation_.x -= rotSpeed;
		if (Input::GetInstance()->PushKey(DIK_DOWN))
			debugCamera_.rotation_.x += rotSpeed;
		if (Input::GetInstance()->PushKey(DIK_LEFT))
			debugCamera_.rotation_.y -= rotSpeed;
		if (Input::GetInstance()->PushKey(DIK_RIGHT))
			debugCamera_.rotation_.y += rotSpeed;

		// 移動：WASD・上下：QE
		if (Input::GetInstance()->PushKey(DIK_W))
			debugCamera_.translation_.z += moveSpeed * cos(debugCamera_.rotation_.y);
		if (Input::GetInstance()->PushKey(DIK_S))
			debugCamera_.translation_.z -= moveSpeed * cos(debugCamera_.rotation_.y);
		if (Input::GetInstance()->PushKey(DIK_D))
			debugCamera_.translation_.x += moveSpeed * sin(debugCamera_.rotation_.y);
		if (Input::GetInstance()->PushKey(DIK_A))
			debugCamera_.translation_.x -= moveSpeed * sin(debugCamera_.rotation_.y);
		if (Input::GetInstance()->PushKey(DIK_Q))
			debugCamera_.translation_.y -= moveSpeed;
		if (Input::GetInstance()->PushKey(DIK_E))
			debugCamera_.translation_.y += moveSpeed;

		debugCamera_.TransferMatrix();
	}
}

void GameScene::Draw() {
	// ★ カメラを毎フレーム転送（真上固定）
	camera_.translation_ = {0.0f, CAM_H, 0.0f};
	camera_.rotation_ = {-3.14159265f * 0.5f, 0.0f, 0.0f}; // -90°俯瞰
	                                                       // ★ 使うカメラを選択
	KamataEngine::Camera& activeCam = (useDebugCamera_) ? debugCamera_ : camera_;

	// Model::PreDraw(dxCommon->GetCommandList());
	activeCam.TransferMatrix();

	auto* dxCommon = DirectXCommon::GetInstance();

	Sprite::PreDraw(dxCommon->GetCommandList());
	Sprite::PostDraw();

	Model::PreDraw(dxCommon->GetCommandList());
	camera_.TransferMatrix();
	//// 壁（永続WTで描画）
	//for (size_t i = 0; i < walls_.size(); ++i) {
	//	// もし壁を動かすなら BuildWallWT で更新してから描画
	//	DrawCube(*wallWts_[i], camera_);
	//}

	//if (player_.alive)
	//	DrawCube(player_.wt, camera_);
	//if (enemy_.alive)
	//	DrawCube(enemy_.wt, camera_);

	//// 弾（永続WTで描画）
	//for (auto& b : bullets_) {
	//	if (!b.alive || !b.wt)
	//		continue;
	//	DrawCube(*b.wt, camera_);
	//}

	//cubeModel_->Draw(player_.wt, camera_);
	
	
	

	//const float s = editor_.cellSize;
	//// 中央寄せしたいならオフセット、左上基準で良ければ 0.0f に
	//const float ox = (editor_.width * 0.5f - 0.5f) * s; // 中央寄せ
	//const float oz = (editor_.height * 0.5f - 0.5f) * s;

	struct TileItem {
		int id, x, y;
	};
	static std::vector<TileItem> tiles;
	tiles.clear();

	// 1) 描くセルを収集（editor/CSVは正しいので全て入るはず）
	editor_.ForEach(
	    [&](int x, int y, int id) {
		    if (id <= 0 || id >= (int)tileModels_.size())
			    return;
		    if (!tileModels_[id])
			    return;
		    tiles.push_back({id, x, y});
	    },
	    true);

	// 2) タイルごとに別の WT（=別CB）を用意
	static std::vector<std::unique_ptr<KamataEngine::WorldTransform>> wts;
	if (wts.size() < tiles.size()) {
		size_t old = wts.size();
		wts.resize(tiles.size());
		for (size_t i = old; i < wts.size(); ++i) {
			wts[i] = std::make_unique<KamataEngine::WorldTransform>();
			wts[i]->Initialize();
		}
	}

	// 3) 行列を作ってGPUへ転送（左上原点／少しだけYを持ち上げてZ-fight回避）
	const float s = editor_.cellSize;
	auto GridYtoWorldY = [&](int gy) { return (editor_.height - 1 - gy) * s; };

	for (size_t i = 0; i < tiles.size(); ++i) {
		auto& wt = *wts[i];
		wt.scale_ = {s*0.5f, s*0.5f, s};
		wt.rotation_ = {0, 0, 0};
		wt.translation_ = {tiles[i].x * s, GridYtoWorldY(tiles[i].y), 0.01f};
		wt.matWorld_ = ge3::math::MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();

		auto* mdl = tileModels_[tiles[i].id];
		if (mdl)
			mdl->Draw(wt, camera_);
	}

	Model::PostDraw();
}

// ================= 予約入力 ====================
void GameScene::HandleReserveInput() {
	// スロット移動
	if (Input::GetInstance()->TriggerKey(DIK_LEFT))
		selSlot_ = (std::max)(0, selSlot_ - 1);
	if (Input::GetInstance()->TriggerKey(DIK_RIGHT))
		selSlot_ = (std::min)(2, selSlot_ + 1);

	auto pushAct = [&](Act a) {
		if ((int)player_.queue.size() < 3) {
			// カーソル位置に挿入：分かりやすさ優先（末尾でも可）
			if (selSlot_ > (int)player_.queue.size())
				selSlot_ = (int)player_.queue.size();
			player_.queue.insert(player_.queue.begin() + selSlot_, a);
			selSlot_ = (std::min)(2, selSlot_ + 1);
		}
	};

	// アクション入力（W/A/D/J）
	if (Input::GetInstance()->TriggerKey(DIK_W))
		pushAct(Act::MOVE_FWD);
	if (Input::GetInstance()->TriggerKey(DIK_A))
		pushAct(Act::ROT_L);
	if (Input::GetInstance()->TriggerKey(DIK_D))
		pushAct(Act::ROT_R);
	if (Input::GetInstance()->TriggerKey(DIK_J))
		pushAct(Act::SHOOT);

	// 削除（BackSpace）
	if (Input::GetInstance()->TriggerKey(DIK_BACK)) {
		if (!player_.queue.empty()) {
			if (selSlot_ >= (int)player_.queue.size())
				selSlot_ = (int)player_.queue.size() - 1;
			if (selSlot_ >= 0)
				player_.queue.erase(player_.queue.begin() + selSlot_);
			selSlot_ = (std::max)(0, selSlot_ - 1);
		}
	}

	// 実行（Enter）
	if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		if (!player_.queue.empty()) {
			BuildEnemyOrder();
			phase_ = Phase::Execute;
			execStep_ = 0;
			execMini_ = 0;
		}
	}
}

// ============== 敵の予約（超シンプル） =================
void GameScene::BuildEnemyOrder() {
	enemy_.queue.clear();

	// 同列/同行なら射線チェック（壁に遮られてないなら SHOOT）
	auto clearLine = [&](bool sameX) -> bool {
		if (sameX) {
			float x = player_.x;
			if (std::abs(x - enemy_.x) > 0.01f)
				return false;
			float a = (std::min)(player_.z, enemy_.z), b = (std::max)(player_.z, enemy_.z);
			for (auto& w : walls_)
				if (w.minx <= x && x <= w.maxx && !(b <= w.minz || w.maxz <= a))
					return false;
			return true;
		} else {
			float z = player_.z;
			if (std::abs(z - enemy_.z) > 0.01f)
				return false;
			float a = (std::min)(player_.x, enemy_.x), b = (std::max)(player_.x, enemy_.x);
			for (auto& w : walls_)
				if (w.minz <= z && z <= w.maxz && !(b <= w.minx || w.maxx <= a))
					return false;
			return true;
		}
	};

	if (clearLine(true) || clearLine(false)) {
		enemy_.queue.push_back(Act::SHOOT);
	}

	// 残りはプレイヤーに近づく（大きい軸を優先）
	float dx = player_.x - enemy_.x;
	float dz = player_.z - enemy_.z;
	if (std::abs(dx) > std::abs(dz)) {
		// X 方向を向く
		int want = (dx >= 0) ? 0 : 2;
		int delta = (want - enemy_.dir4 + 4) % 4;
		if (delta == 1)
			enemy_.queue.push_back(Act::ROT_R);
		else if (delta == 3)
			enemy_.queue.push_back(Act::ROT_L);
		enemy_.queue.push_back(Act::MOVE_FWD);
	} else {
		// Z 方向を向く
		int want = (dz >= 0) ? 1 : 3;
		int delta = (want - enemy_.dir4 + 4) % 4;
		if (delta == 1)
			enemy_.queue.push_back(Act::ROT_R);
		else if (delta == 3)
			enemy_.queue.push_back(Act::ROT_L);
		enemy_.queue.push_back(Act::MOVE_FWD);
	}

	// 3手未満なら MOVE で埋める
	while ((int)enemy_.queue.size() < 3)
		enemy_.queue.push_back(Act::MOVE_FWD);
}

// ============== 実行フェーズ進行 ========================
bool GameScene::TickExecuteMiniFrame() {
	// 既に誰か死んでいたら決着
	if (!player_.alive || !enemy_.alive)
		return true;

	// 現在ステップのアクションを取り出す（あれば実行）
	auto stepAct = [&](TankXZ& t) -> Act {
		if ((int)t.queue.size() > execStep_)
			return t.queue[execStep_];
		return Act::MOVE_FWD; // 無ければ何もしない扱いでもOK
	};

	// ミニフレーム進行：ROT/SHOOT はミニフレーム0で即時実行、MOVE は分割
	if (execMini_ == 0) {
		// 回転
		if (stepAct(player_) == Act::ROT_L)
			player_.dir4 = (player_.dir4 + 3) & 3;
		if (stepAct(player_) == Act::ROT_R)
			player_.dir4 = (player_.dir4 + 1) & 3;
		if (stepAct(enemy_) == Act::ROT_L)
			enemy_.dir4 = (enemy_.dir4 + 3) & 3;
		if (stepAct(enemy_) == Act::ROT_R)
			enemy_.dir4 = (enemy_.dir4 + 1) & 3;

		// SHOOT
		auto shoot = [&](TankXZ& t) {
			float dx, dz;
			DirVec(t.dir4, dx, dz);
			BulletXZ b{};
			b.x = t.x;
			b.z = t.z;
			b.vx = dx * BULLET_SPEED / STEP_N;
			b.vz = dz * BULLET_SPEED / STEP_N;
			b.alive = true;

			b.wt = std::make_unique<KamataEngine::WorldTransform>(); // 永続WT
			b.wt->Initialize();
			b.wt->scale_ = {BULLET_R * 2, BULLET_R * 2, BULLET_R * 2}; // 見た目サイズ
			b.wt->translation_ = {b.x, Y_LIFT, b.z};
			b.wt->TransferMatrix();

			bullets_.push_back(std::move(b));
		};

		if (stepAct(player_) == Act::SHOOT)
			shoot(player_);
		if (stepAct(enemy_) == Act::SHOOT)
			shoot(enemy_);

		// 描画WTを同期
		SyncWT_Tank(player_);
		SyncWT_Tank(enemy_);
	}

	// MOVE のときだけ分割移動
	if (stepAct(player_) == Act::MOVE_FWD)
		MoveOneMini(player_);
	if (stepAct(enemy_) == Act::MOVE_FWD)
		MoveOneMini(enemy_);

	// 弾の更新（全弾）
	if (UpdateBulletsMini())
		return true; // 決着

	// 次ミニフレームへ
	execMini_++;
	if (execMini_ >= STEP_N) {
		execMini_ = 0;
		execStep_++;
		// 次ステップへ（3手終わったら次ターン）
		if (execStep_ >= 3) {
			execStep_ = 0;
			// 終了チェック
			if (!player_.alive || !enemy_.alive)
				return true;
			// 次ターンへ
			player_.queue.clear();
			enemy_.queue.clear();
			selSlot_ = 0;
			bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](const BulletXZ& b) { return !b.alive; }), bullets_.end());
			phase_ = Phase::Reserve; // 予約に戻る
		}
	}
	return (!player_.alive || !enemy_.alive);
}

void GameScene::MoveOneMini(TankXZ& t) {
	float dx, dz;
	DirVec(t.dir4, dx, dz);
	float nx = t.x + dx * (CELL / STEP_N);
	float nz = t.z + dz * (CELL / STEP_N);

	// 壁衝突（タンク半径ぶん膨張）
	if (!HitBoxExpanded(nx, nz, TANK_HALF_W, TANK_HALF_D)) {
		t.x = nx;
		t.z = nz;
		SyncWT_Tank(t);
	}
}

bool GameScene::UpdateBulletsMini() {
	for (auto& b : bullets_) {
		if (!b.alive)
			continue;
		float px = b.x, pz = b.z;
		b.x += b.vx;
		b.z += b.vz;
		ReflectBullet(b, px, pz);
		if (b.alive && b.wt) { // 存在する間はWTを更新
			b.wt->translation_ = {b.x, Y_LIFT, b.z};
			b.wt->TransferMatrix();
		}
		if (!player_.alive || !enemy_.alive)
			return true;
	}
	return (!player_.alive || !enemy_.alive);
}


void GameScene::ReflectBullet(BulletXZ& b, float prevx, float prevz) {
	if (!b.alive)
		return;

	// 壁AABB（弾半径ぶん膨張）と最近点距離
	auto hitWall = [&](const BoxXZ& w) -> bool {
		float nx = Clamp(b.x, w.minx - BULLET_R, w.maxx + BULLET_R);
		float nz = Clamp(b.z, w.minz - BULLET_R, w.maxz + BULLET_R);
		float dx = b.x - nx, dz = b.z - nz;
		return (dx * dx + dz * dz <= BULLET_R * BULLET_R);
	};

	for (auto& w : walls_) {
		if (!hitWall(w))
			continue;
		// どの面で当たったかを prev→cur の移動から推定し、符号反転
		if (prevx < w.minx - BULLET_R && b.x >= w.minx - BULLET_R)
			b.vx *= -1;
		else if (prevx > w.maxx + BULLET_R && b.x <= w.maxx + BULLET_R)
			b.vx *= -1;
		if (prevz < w.minz - BULLET_R && b.z >= w.minz - BULLET_R)
			b.vz *= -1;
		else if (prevz > w.maxz + BULLET_R && b.z <= w.maxz + BULLET_R)
			b.vz *= -1;
		// 押し戻し（単純に前位置へ戻すだけでOK）
		b.x = prevx;
		b.z = prevz;
		break;
	}

	// 命中判定（プレイヤー／敵）
	if (player_.alive && HitTank(b, player_)) {
		player_.alive = false;
	}
	if (enemy_.alive && HitTank(b, enemy_)) {
		enemy_.alive = false;
	}
	if (!player_.alive || !enemy_.alive)
		b.alive = false;
}

bool GameScene::HitBoxExpanded(float x, float z, float rx, float rz) {
	// 点(x,z) が「各壁AABBを rx,rz だけ膨張した領域」に入っているか
	for (auto& w : walls_) {
		if (x >= (w.minx - rx) && x <= (w.maxx + rx) && z >= (w.minz - rz) && z <= (w.maxz + rz)) {
			return true; // 衝突
		}
	}
	return false;
}

bool GameScene::HitTank(const BulletXZ& b, const TankXZ& t) {
	float nx = Clamp(b.x, t.x - TANK_HALF_W, t.x + TANK_HALF_W);
	float nz = Clamp(b.z, t.z - TANK_HALF_D, t.z + TANK_HALF_D);
	float dx = b.x - nx, dz = b.z - nz;
	return (dx * dx + dz * dz <= BULLET_R * BULLET_R);
}

void GameScene::SyncWT_Tank(TankXZ& t) {
	t.wt.translation_ = {t.x, Y_LIFT, t.z};
	// dir4 を Y 回転へ（0:+X,1:+Z,2:-X,3:-Z）→ ラジアンで (−90°,0°, +90°, 180°) 等 好みで
	static const float kRotY[4] = {0.0f, +3.14159265f * 0.5f, 3.14159265f, -3.14159265f * 0.5f};
	t.wt.rotation_ = {0.0f, kRotY[t.dir4 & 3], 0.0f};
	t.wt.scale_ = {0.8f, 0.6f, 1.0f}; // 車体の見た目
	t.wt.TransferMatrix();
}

void GameScene::MakeWallWT(const BoxXZ& w, KamataEngine::WorldTransform& out) {
	out.Initialize();
	const float cx = (w.minx + w.maxx) * 0.5f;
	const float cz = (w.minz + w.maxz) * 0.5f;
	const float sx = (w.maxx - w.minx);
	const float sz = (w.maxz - w.minz);
	out.translation_ = {cx, Y_LIFT, cz};
	out.scale_ = {sx, 1.0f, sz};
	out.TransferMatrix();
}



void GameScene::DrawCube(const KamataEngine::WorldTransform& wt, const KamataEngine::Camera& cam) {
	if (!cubeModel_)
		return;
	cubeModel_->Draw(wt, cam);
}

void GameScene::BuildWallWT(const BoxXZ& w, KamataEngine::WorldTransform& out) {
	const float cx = (w.minx + w.maxx) * 0.5f;
	const float cz = (w.minz + w.maxz) * 0.5f;
	const float sx = (w.maxx - w.minx);
	const float sz = (w.maxz - w.minz);
	out.translation_ = {cx, Y_LIFT, cz};
	out.scale_ = {sx, 1.0f, sz};
	out.TransferMatrix();
}

void GameScene::ResetAfterPlayerDeath(bool forceNow) {
	// 状態をゲームへ戻す
	phase_ = ScenePhase::Game;
	deathElapsed_ = 0.0f;

	// 弾やタイマを初期化
	bullets_.clear();
	fireTimer_ = 0.0f;

	if (!forceNow) {
		// 例：最低演出時間が経っていなければ return
		if (deathElapsed_ < deathMinTime_)
			return;
	}

	// プレイヤーを初期位置へ
	{
		auto& wt = player_->EditWorldTransform();
		wt.translation_ = playerSpawn_;
		wt.rotation_ = {0, 0, 0}; // 必要なら角度もリセット
		wt.matWorld_ = MakeAffine(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();
	}

	// 敵も全員初期位置へ（速度なども適宜初期化）
	for (size_t i = 0; i < enemies_.size(); ++i) {
		auto& e = enemies_[i];
		auto& wt = e->EditWorldTransform();
		if (i < enemySpawns_.size()) {
			wt.translation_ = enemySpawns_[i];
		}
		// 進行方向リセット（左右反転ロジックに備えて 0 に）
		e->EditVelocity().x = 0.0f;
		e->EditVelocity().y = 0.0f;
		wt.rotation_ = {0, 0, 0};
		wt.matWorld_ = MakeAffine(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();
	}

	// Death演出は消しておく（保険）
	for (auto& dp : deathParticles_) {
		delete dp.wt;
		dp.wt = nullptr;
	}
	deathParticles_.clear();

	// カメラやスカイドームは既存処理に任せる
}

void GameScene::SpawnClearBurst(int count, const KamataEngine::Vector3& center) {
	clearParticles_.reserve(clearParticles_.size() + count);

	for (int i = 0; i < count; ++i) {
		ClearParticle p{};

		// WorldTransform を個別に持つ（死亡時パーティクルと同じ作法）
		p.wt = new KamataEngine::WorldTransform();
		p.wt->Initialize();

		// 初期位置：center 付近に少しばらつき
		p.wt->translation_ = {center.x + frand(-0.5f, +0.5f), center.y + frand(-0.5f, +0.5f), center.z};

		// 初期スケール：ちいさめの紙片
		float s = frand(0.15f, 0.35f);
		p.wt->scale_ = {s, s, s};

		// 速度：放射＋上向き少し（紙吹雪っぽく）
		p.vel.x = frand(-3.5f, +3.5f);
		p.vel.y = frand(+2.0f, +6.0f);

		// 寿命
		p.maxLife = frand(0.9f, 1.6f);
		p.life = p.maxLife;

		// 反映
		p.wt->matWorld_ = MakeAffine(p.wt->scale_, p.wt->rotation_, p.wt->translation_);
		p.wt->TransferMatrix();

		clearParticles_.push_back(p);
	}
}

void GameScene::UpdateClearParticles(float dt) {
	const float gravity = -9.8f * 0.35f; // 軽めの重力
	const float dampVel = 0.985f;        // 速度減衰
	// const float dampRot = 0.97f;         // （使わないなら無視してOK）

	for (auto& p : clearParticles_) {
		if (!p.wt)
			continue;
		p.life -= dt;
		if (p.life <= 0.0f)
			continue;

		// 物理
		p.vel.y += gravity * dt;
		p.vel.x *= dampVel;

		// 位置更新（XYのみ）
		p.wt->translation_.x += p.vel.x * dt;
		p.wt->translation_.y += p.vel.y * dt;

		// 経過によってスケールを少し縮める
		float t = (p.life > 0.0f && p.maxLife > 0.0f) ? (p.life / p.maxLife) : 0.0f;
		float s = (std::max)(0.0f, t) * 0.35f; // 初期0.35前後に合わせる
		p.wt->scale_ = {s, s, s};

		p.wt->matWorld_ = MakeAffine(p.wt->scale_, p.wt->rotation_, p.wt->translation_);
		p.wt->TransferMatrix();
	}

	// 死亡したものをまとめて破棄（メモリ掃除）
	// life<=0 または wt==nullptr を削除
	if (!clearParticles_.empty()) {
		std::vector<ClearParticle> alive;
		alive.reserve(clearParticles_.size());
		for (auto& p : clearParticles_) {
			if (p.life > 0.0f && p.wt) {
				alive.push_back(p);
			} else {
				if (p.wt) {
					delete p.wt;
				}
			}
		}
		clearParticles_.swap(alive);
	}
}

void GameScene::ResetActorsToSpawn() {
	// 弾・タイマ・演出の軽い初期化
	bullets_.clear();
	fireTimer_ = 0.0f;

	// Death/ Clear パーティクル類のクリーンアップ（保険）
	for (auto& dp : deathParticles_) {
		delete dp.wt;
		dp.wt = nullptr;
	}
	deathParticles_.clear();

	for (auto& p : clearParticles_) {
		if (p.wt)
			delete p.wt;
	}
	clearParticles_.clear();

	// プレイヤーをスポーンへ
	{
		auto& wt = player_->EditWorldTransform();
		wt.translation_ = playerSpawn_;
		wt.rotation_ = {0, 0, 0};
		wt.matWorld_ = MakeAffine(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();
	}

	// 敵もスポーンへ
	for (size_t i = 0; i < enemies_.size(); ++i) {
		auto& e = enemies_[i];
		auto& wt = e->EditWorldTransform();

		if (i < enemySpawns_.size()) {
			wt.translation_ = enemySpawns_[i];
		}
		e->EditVelocity().x = 0.0f;
		e->EditVelocity().y = 0.0f;
		wt.rotation_ = {0, 0, 0};
		wt.matWorld_ = MakeAffine(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();
	}
}

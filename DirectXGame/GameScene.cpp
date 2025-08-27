#include "GameScene.h"
#include "ClearScene.h"
#include "Enemy.h"
#include "MapLoader.h"
#include "TileCollision.h"
#include <cmath> // ★ 追加：std::sin / std::cos 用
using namespace KamataEngine;

namespace { // ---- local helpers ----
using KamataEngine::Matrix4x4;
using KamataEngine::Vector3;

inline Matrix4x4 Identity() {
	Matrix4x4 a{};
	for (int i = 0; i < 4; ++i)
		a.m[i][i] = 1.0f;
	return a;
}

inline Matrix4x4 Multiply(const Matrix4x4& A, const Matrix4x4& B) {
	Matrix4x4 C{};
	for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			C.m[r][c] = A.m[r][0] * B.m[0][c] + A.m[r][1] * B.m[1][c] + A.m[r][2] * B.m[2][c] + A.m[r][3] * B.m[3][c];
		}
	}
	return C;
}

inline Matrix4x4 MakeScale(const Vector3& s) {
	Matrix4x4 a = Identity();
	a.m[0][0] = s.x;
	a.m[1][1] = s.y;
	a.m[2][2] = s.z;
	return a;
}
inline Matrix4x4 MakeTranslate(const Vector3& t) {
	Matrix4x4 a = Identity();
	a.m[3][0] = t.x;
	a.m[3][1] = t.y;
	a.m[3][2] = t.z; // ←行ベクトル系（多くの教材エンジンがこれ）
	return a;
}
inline Matrix4x4 MakeRotateX(float rx) {
	Matrix4x4 a = Identity();
	float c = std::cos(rx), s = std::sin(rx);
	a.m[1][1] = c;
	a.m[1][2] = s;
	a.m[2][1] = -s;
	a.m[2][2] = c;
	return a;
}
inline Matrix4x4 MakeRotateY(float ry) {
	Matrix4x4 a = Identity();
	float c = std::cos(ry), s = std::sin(ry);
	a.m[0][0] = c;
	a.m[0][2] = -s;
	a.m[2][0] = s;
	a.m[2][2] = c;
	return a;
}
inline Matrix4x4 MakeRotateZ(float rz) {
	Matrix4x4 a = Identity();
	float c = std::cos(rz), s = std::sin(rz);
	a.m[0][0] = c;
	a.m[0][1] = s;
	a.m[1][0] = -s;
	a.m[1][1] = c;
	return a;
}

// 角度はラジアン。行列の掛け順は S * (Rx*Ry*Rz) * T（教材の行ベクトル想定）
inline Matrix4x4 MakeAffine(const Vector3& s, const Vector3& r, const Vector3& t) {
	Matrix4x4 S = MakeScale(s);
	Matrix4x4 Rx = MakeRotateX(r.x);
	Matrix4x4 Ry = MakeRotateY(r.y);
	Matrix4x4 Rz = MakeRotateZ(r.z);
	Matrix4x4 R = Multiply(Multiply(Rx, Ry), Rz);
	Matrix4x4 T = MakeTranslate(t);
	return Multiply(Multiply(S, R), T);
}

// --- row-vector × row-major の View から Eye を復元 ---
inline KamataEngine::Vector3 ExtractEyeFromView(const KamataEngine::Matrix4x4& V) {
	// V の上左3x3は R^T、最下段(3,0..2) は -eye * R^T
	float Rt00 = V.m[0][0], Rt01 = V.m[0][1], Rt02 = V.m[0][2];
	float Rt10 = V.m[1][0], Rt11 = V.m[1][1], Rt12 = V.m[1][2];
	float Rt20 = V.m[2][0], Rt21 = V.m[2][1], Rt22 = V.m[2][2];

	// R = (R^T)^T
	float R00 = Rt00, R01 = Rt10, R02 = Rt20;
	float R10 = Rt01, R11 = Rt11, R12 = Rt21;
	float R20 = Rt02, R21 = Rt12, R22 = Rt22;

	float tx = V.m[3][0], ty = V.m[3][1], tz = V.m[3][2]; // -eye * R^T

	KamataEngine::Vector3 eye{};
	eye.x = (-tx) * R00 + (-ty) * R01 + (-tz) * R02;
	eye.y = (-tx) * R10 + (-ty) * R11 + (-tz) * R12;
	eye.z = (-tx) * R20 + (-ty) * R21 + (-tz) * R22;
	return eye;
}

// ★ 自機/敵の当たりサイズ（見た目に合わせて微調整）
inline Vector3 PlayerHalfExt() { return {0.45f, 0.90f, 0.0f}; }
inline Vector3 EnemyHalfExt() { return {0.45f, 0.85f, 0.0f}; }

inline bool AABBHit(const Vector3& aPos, const Vector3& aHalf, const Vector3& bPos, const Vector3& bHalf) {
	return (std::abs(aPos.x - bPos.x) <= (aHalf.x + bHalf.x)) && (std::abs(aPos.y - bPos.y) <= (aHalf.y + bHalf.y));
}

inline float frand(float a, float b) {
	float t = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	return a + (b - a) * t;
}

} // namespace

GameScene::~GameScene() {
	delete sprite_;
	delete model_;
	// 自キャラの解放
	delete player_;
	delete debugCamera_;
	delete modelBlock_;
	delete skydome_;
	delete title_;
	delete modelSphere_;
	delete clear_;

	delete goal_;
	goal_ = nullptr;

	// ブロックの WT を解放
	for (std::vector<KamataEngine::WorldTransform*>& row : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* wt : row) {
			delete wt; // nullptr なら何もしない
		}
		row.clear(); // 行ベクタを空に
	}
	worldTransformBlocks_.clear(); // 外側も空に

	// ...既存...
	for (auto& dp : deathParticles_) {
		delete dp.wt;
		dp.wt = nullptr;
	} // ★ 追記
	deathParticles_.clear();

	for (auto& p : clearParticles_) {
		if (p.wt)
			delete p.wt;
	}
	clearParticles_.clear();
}

void GameScene::Initialize() {
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("enem.png");

	// サウンドデータを読み込む
	soundDataHandle_ = Audio::GetInstance()->LoadWave("mokugyo.wav");

	// 音声再生
	voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	Audio::GetInstance()->PlayWave(soundDataHandle_);

	// スプライトインスタンスの生成
	sprite_ = Sprite::Create(textureHandle_, {100, 50});

	// GameScene.cpp の Initialize 内
	// タイトル用スプライト
	titleSprite_ = Sprite::Create(TextureManager::Load("title.png"), {0.0f, 0.0f}); // 画面中央（例）

	// クリア用スプライト
	clearSprite_ = Sprite::Create(TextureManager::Load("clear.png"), {0.0f, 0.0f});

	// ワールドトランスファームの初期化
	worldTransform_.Initialize();

	// 3Dモデル（既存の model_ をブロックにも使い回し）
	model_ = Model::Create();

	modelBlock_ = Model::CreateFromOBJ("cube", true);

	// ★ パーティクル用の球モデルを用意（失敗したらキューブを使う）
	modelSphere_ = Model::CreateFromOBJ("particle", true);
	if (!modelSphere_) {
		OutputDebugStringA("[WARN] sphere.obj が見つからないので、パーティクルは cube で代用します\n");
		modelSphere_ = modelBlock_; // ← 既に表示できているブロックモデルを流用
	}

	// ---- ① CSV読み込み ----
	worldTransformBlocks_.clear(); // 念のためクリア
	mapData_.clear();
	if (!MapLoader::LoadCsv("map.csv", mapData_)) {
		OutputDebugStringA("[WARN] map.csv が読めないので空マップで起動します\n");
		// 読めなかった場合のデフォルト（任意）
		mapData_ = {
		    {1, 0, 1, 0},
            {0, 1, 0, 1}
        };
	}
	MapLoader::NormalizeRect(mapData_);

	// ---- ② マップからWTを生成 ----
	// タイルの間隔や原点は好みで調整
	const float kModelSize = 2.0f; // scale={2,2,2} 時の見かけ1マス（目安）
	const float kGap = 0.6f;       // タイル間の隙間
	const float pitchX = kModelSize + kGap;
	const float pitchY = kModelSize + kGap;
	const float originX = 20.0f; // 画面内オフセット
	const float originY = 6.0f;

	const uint32_t rows = (uint32_t)mapData_.size();
	const uint32_t cols = rows ? (uint32_t)mapData_[0].size() : 0;
	worldTransformBlocks_.assign(rows, std::vector<WorldTransform*>(cols, nullptr));

	for (uint32_t y = 0; y < rows; ++y) {
		for (uint32_t x = 0; x < cols; ++x) {
			if (mapData_[y][x] == 0)
				continue; // 0 は “穴” → nullptr のまま

			auto* wt = new WorldTransform();
			wt->Initialize();
			wt->translation_.x = originX + x * pitchX;
			wt->translation_.y = originY + y * pitchY;
			wt->translation_.z = 10.0f;
			wt->scale_ = {1.0f, 1.0f, 1.0f}; // 小さめにしたいときは 1.6f など
			wt->TransferMatrix();

			worldTransformBlocks_[y][x] = wt;
		}
	}

	// 既存の pitchX/pitchY/originX/originY を保持
	tilePitchX_ = pitchX;
	tilePitchY_ = pitchY;
	tileOriginX_ = originX;
	tileOriginY_ = originY;

	// ★ 当たりは見た目キューブの“半径”で統一
	tileHalfX_ = kModelSize * 0.5f;
	tileHalfY_ = kModelSize * 0.5f;

	// マップを並べるループの直後あたりに追加
	for (int y = 0; y < (int)mapData_.size(); ++y) {
		for (int x = 0; x < (int)mapData_[0].size(); ++x) {
			if (mapData_[y][x] == 2) {
				KamataEngine::Vector3 c{30.0f, 10.0f, 10.0f};
				auto e = std::make_unique<Enemy>();
				e->Initialize(model_, TextureManager::Load("enemy.png"), c);
				e->SetSpeed(1.0f); // 好みで

				enemies_.push_back(std::move(e));

				mapData_[y][x] = 0; // 通行可能にしておく
			}
		}
	}

	// --- ②' ゴール(3)検出：WTを作って描画用に保持し、通行可能にしておく ---
	for (int y = 0; y < (int)mapData_.size(); ++y) {
		for (int x = 0; x < (int)mapData_[0].size(); ++x) {
			if (mapData_[y][x] == 3) {
				goalTx_ = x;
				goalTy_ = y;

				if (!goal_)
					goal_ = new Goal();
				// 位置はプレイヤーと同じく「セル中心から算出したワールド座標」を渡す
				KamataEngine::Vector3 goalPos{25.0f, 42.5f, 10.0f};
				if (goalTex_ == 0) {
					// ひとまず既存の enemy.png を流用。専用の goal.png を用意したら差し替え可
					goalTex_ = KamataEngine::TextureManager::Load("goal.png"); // 無ければ "enemy.png"
					if (!goalTex_)
						goalTex_ = KamataEngine::TextureManager::Load("enemy.png");
				}
				goal_->Initialize(model_, goalTex_, &camera_, goalPos);
			}
		}
	}

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// カメラの初期化
	camera_.Initialize();
	camera_.farZ = 20000.0f; // または SetFar(20000.0f);
	camera_.nearZ = 0.1f;
	camera_.translation_ = {60.0f, 25.0f, -50.0f};

	// 3Dモデルの生成

	// ★ スカイドーム生成（最初は原点固定でOK / 追従は後述）
	skydome_ = new Skydome();
	skydome_->Initialize("skydome", /*scale=*/1200.0f); // シーンに合わせて調整

	// 自キャラの生成
	player_ = new Player();

	// int sx = 0, sy = (int)mapData_.size() - 1; // 最下段の0列目タイル
	KamataEngine::Vector3 spawnPos{
	    originX + 5 * tilePitchX_, // X=5列目のセル中心
	    originY + 2 * tilePitchY_, // Y=2行目のセル中心
	    10.0f};

	// 自キャラの初期化
	player_->Initialize(model_, textureHandle_, &camera_, spawnPos);

	// --- プレイヤー＆敵のスポーン位置を記録 ---
	playerSpawn_ = player_->GetWorldTransform().translation_;

	enemySpawns_.clear();
	enemySpawns_.reserve(enemies_.size());
	for (auto& e : enemies_) {
		enemySpawns_.push_back(e->GetWorldTransform().translation_);
	}

	// ★ タイトル開始
	phase_ = ScenePhase::Title;
	title_ = new TitleScene();
	title_->Initialize(); // TitleScene は内部で title.png を読む実装【】
}

void GameScene::Update() {
	// スペースキーを押した瞬間
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {

		// 音声停止
		Audio::GetInstance()->StopWave(voiceHandle_);
	}

	// ★ タイトル中はタイトルだけ更新して抜ける
	if (phase_ == ScenePhase::Title) {
		if (title_)
			title_->Update();
		// SPACE が押されると finished_ が true になる実装【】
		if (title_ && title_->IsFinished()) {
			delete title_;
			title_ = nullptr;
			phase_ = ScenePhase::Game; // 切り替え

		} else {
			return; // まだタイトル中ならゲームの更新はしない
		}
	}

	// --- ブロックのワールド行列を毎フレーム計算して転送 ---
	for (auto& row : worldTransformBlocks_) {
		for (auto* wt : row) {
			if (!wt)
				continue;

			// 角度はラジアン想定。必要ならここで s/r/t を更新してから…
			wt->matWorld_ = MakeAffine(wt->scale_, wt->rotation_, wt->translation_);
			if (wt->parent_) {
				wt->matWorld_ = Multiply(wt->matWorld_, wt->parent_->matWorld_);
			}
			wt->TransferMatrix(); // GPUへ反映
		}
	}

	// ======= GAME 本編 =======
	if (phase_ == ScenePhase::Game) {
		// 自キャラ
		player_->Update();
		if (goal_)
			goal_->Update();

		// タイルフィールド作成 → 自機×タイル衝突（既存）
		TileField tf;
		tf.originX = tileOriginX_;
		tf.originY = tileOriginY_;
		tf.pitchX = tilePitchX_;
		tf.pitchY = tilePitchY_;
		tf.halfX = tileHalfX_; // ★ 追加
		tf.halfY = tileHalfY_; // ★ 追加
		tf.grid = &mapData_;
		ResolvePlayerVsTilemap(*player_, tf);

		// 敵の更新＆衝突（既存）
		const float dt = 1.0f / 60.0f;
		for (auto& e : enemies_) {
			e->Update(dt);
			ResolveEnemyVsTilemap(*e, tf);

			// 進行方向の「壁 or 足元無し」で反転（既存）
			const auto& wt = e->GetWorldTransform();
			const float ahead = (e->EditVelocity().x >= 0.0f) ? +tf.pitchX * 0.5f : -tf.pitchX * 0.5f;
			int tx = (int)std::floor((wt.translation_.x + ahead - tf.originX) / tf.pitchX + 0.5f);
			int ty = (int)std::floor((wt.translation_.y - tf.pitchY * 0.25f - tf.originY) / tf.pitchY + 0.5f);
			bool wall = IsSolidTile(*tf.grid, ty, tx);
			int txF = (int)std::floor((wt.translation_.x + ahead - tf.originX) / tf.pitchX + 0.5f);
			int tyF = (int)std::floor((wt.translation_.y - tf.pitchY * 0.6f - tf.originY) / tf.pitchY + 0.5f);
			bool noGround = !IsSolidTile(*tf.grid, tyF, txF);
			if (wall || noGround)
				e->EditVelocity().x = -e->EditVelocity().x;
		}

		// ===== 敵×自機の当たり判定 → ヒットで「消滅演出」開始 =====
		const Vector3 pPos = player_->GetWorldTransform().translation_;
		const Vector3 pHalf = PlayerHalfExt();
		bool hit = false;
		for (auto& e : enemies_) {
			const Vector3 ePos = e->GetWorldTransform().translation_;
			if (AABBHit(pPos, pHalf, ePos, EnemyHalfExt())) {
				hit = true;
				break;
			}
		}
		if (hit) {
			// 既存のDPを掃除
			for (auto& dp : deathParticles_) {
				delete dp.wt;
				dp.wt = nullptr;
			}
			deathParticles_.clear();

			// 8方向パーティクル生成
			const float startScale = 0.6f;
			const float lifeSec = 0.8f;
			const float speed = 6.0f;

			for (int i = 0; i < 8; ++i) {
				const float ang = (3.1415926535f / 4.0f) * i; // 45°
				DeathParticle dp;
				dp.wt = new WorldTransform(); // ★ ヒープ確保
				dp.wt->Initialize();
				dp.wt->translation_ = {pPos.x, pPos.y, pPos.z};
				dp.wt->scale_ = {startScale, startScale, startScale};
				dp.vel = {std::cos(ang) * speed, std::sin(ang) * speed, 0.0f};
				dp.life = dp.maxLife = lifeSec;

				// 初回の行列転送
				dp.wt->matWorld_ = MakeAffine(dp.wt->scale_, dp.wt->rotation_, dp.wt->translation_);
				dp.wt->TransferMatrix();

				deathParticles_.push_back(dp);
			}
			phase_ = ScenePhase::Death;
		}

		// ===== ゴール到達判定（タイル3） =====
		if (goalTx_ >= 0 && goalTy_ >= 0) {
			const auto& pwt = player_->GetWorldTransform();
			int ptx = (int)std::floor((pwt.translation_.x - tileOriginX_) / tilePitchX_ + 0.5f);
			int pty = (int)std::floor((pwt.translation_.y - tileOriginY_) / tilePitchY_ + 0.5f);

			if (ptx == goalTx_ && pty == goalTy_) {
				// ★ クリアへ
				phase_ = ScenePhase::Clear;
				if (!clear_) {
					clear_ = new ClearScene();
					clear_->Initialize();
				}

				

				//// 紙吹雪：最初のバースト（原点よりゴール中心が映える）
				// SpawnClearBurst(120, goalWt_ ? goalWt_->translation_ : KamataEngine::Vector3{0, 0, 10});

				// （常時発生させたいなら、あなたが入れているエミッタをONに）
				// clearEmitter_.active = true;
				// clearEmitter_.origin = goalWt_ ? goalWt_->translation_ : KamataEngine::Vector3{0,0,10};
			}
		}

		// ===== ゴール到達判定（座標版） =====
		// プレイヤーが (25, 45, 10) 付近に来たらクリア
		{
			const KamataEngine::Vector3 target{25.0f, 42.5f, 10.0f};

			// 浮動小数の誤差/座標ブレに備えた許容誤差（好みで調整）
			const float eps = 2.0f; // 10cm くらいの感覚
			const float eps2 = eps * eps;

			const auto& pwt = player_->GetWorldTransform();
			const float dx = pwt.translation_.x - target.x;
			const float dy = pwt.translation_.y - target.y;
			const float dz = pwt.translation_.z - target.z;

			if ((dx * dx + dy * dy + dz * dz) <= eps2) {
				// ★ クリアへ
				phase_ = ScenePhase::Clear;
				if (!clear_) {
					clear_ = new ClearScene();
					clear_->Initialize();
				}

				// 紙吹雪（ゴール位置っぽいところを中心に）
				SpawnClearBurst(120, target);
			}
		}

	} else if (phase_ == ScenePhase::Death) {
		const float deltatime = 1.0f / 60.0f;
		const float gravity = -9.8f * 0.4f;
		const float damp = 0.98f;

		bool anyAlive = false;
		for (auto& dp : deathParticles_) {
			if (dp.life <= 0.0f)
				continue;
			dp.life -= deltatime;

			dp.vel.y += gravity * deltatime;
			dp.vel.x *= damp;
			dp.vel.y *= damp;

			dp.wt->translation_.x += dp.vel.x * deltatime;
			dp.wt->translation_.y += dp.vel.y * deltatime;

			const float t = (dp.life > 0.0f) ? (dp.life / dp.maxLife) : 0.0f;
			const float s = 0.6f * t;
			dp.wt->scale_ = {s, s, s};

			dp.wt->matWorld_ = MakeAffine(dp.wt->scale_, dp.wt->rotation_, dp.wt->translation_);
			dp.wt->TransferMatrix();

			if (dp.life > 0.0f)
				anyAlive = true;
		}

		if (!anyAlive) {
			// メモリ解放
			for (auto& dp : deathParticles_) {
				delete dp.wt;
				dp.wt = nullptr;
			}
			deathParticles_.clear();

			//// タイトルに戻す
			// if (!title_) {
			//	title_ = new TitleScene();
			//	title_->Initialize();
			// }

			// === 変更点：タイトルに戻さず、その場でラウンドをリセット ===
			ResetAfterPlayerDeath(/*forceNow=*/true);

			// phase_ = ScenePhase::Title;
		}
	}
	// ===== CLEAR フェーズ更新 =====
	if (phase_ == ScenePhase::Clear) {

		const float dt = 1.0f / 60.0f; // あなたのΔtがあるなら置き換え
		UpdateClearParticles(dt);

		// ★ クリア演出：紙吹雪を中央付近に生成
		{
			KamataEngine::Vector3 center = {/*x=*/0.0f, /*y=*/0.0f, /*z=*/0.0f};
			// もしプレイヤー位置を中心にしたいなら: center = player_->EditWorldTransform().translation_;
			SpawnClearBurst(/*count=*/120, center);
		}

		if (clear_)
			clear_->Update();

		// CLEARからの遷移：SPACE/ENTER で Title に戻す（再挑戦導線）
		if (clear_ && clear_->IsFinished()) {
			// タイトルを再生成して戻す
			if (!title_) {
				title_ = new TitleScene();
				title_->Initialize();
			}
			phase_ = ScenePhase::Title;
			// 次回に備えてゲーム状態を軽く初期化（弾など）
			ResetActorsToSpawn();
			fireTimer_ = 0.0f;
		}
	}

#if defined(_DEBUG)
	// ★ F1 でデバッグカメラ ON/OFF をトグル
	if (Input::GetInstance()->TriggerKey(DIK_F1)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
		OutputDebugStringA(isDebugCameraActive_ ? "[DBG] DebugCamera: ON\n" : "[DBG] DebugCamera: OFF\n");
	}
#endif

	if (isDebugCameraActive_) {
		// ★ デバッグカメラを更新して、その行列を描画用 camera_ へコピー
		if (debugCamera_)
			debugCamera_->Update();

		camera_.TransferMatrix(); // GPUへ反映（あなたの環境のAPIに合わせて）
	} else {
		// いつも通り通常カメラの更新
		camera_.UpdateMatrix(); // もしくは TransferMatrix() 相当
	}

	// アクティブな View 行列を取り出す
	KamataEngine::Matrix4x4 V = (isDebugCameraActive_ && debugCamera_) ? debugCamera_->GetCamera().matView : camera_.matView;

	// Eye を復元して追従させる
	if (skydome_)
		skydome_->UpdateFollowAt(ExtractEyeFromView(V));

	debugCamera_->SetFarZ(camera_.farZ);   // 通常カメラと同じ遠方クリップ面
	debugCamera_->SetNearZ(camera_.nearZ); // 通常カメラと同じ近接クリップ面
}

void GameScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ★ タイトル中はタイトル画面のみ描いて return
	if (phase_ == ScenePhase::Title) {
		if (title_) {
			title_->Draw(); // TitleScene::Draw は 2D スプライトを描く実装【】
		}
		return;
	}

	// --------- 2D（スプライト） ---------
	Sprite::PreDraw(dxCommon->GetCommandList());

	// GameScene::Draw
	if (phase_ == ScenePhase::Clear) {
		clearSprite_->Draw();
	}

	/*if (sprite_) {
	    sprite_->Draw();
	}*/
	Sprite::PostDraw();

	// --------- 3D（モデル） ---------
	Model::PreDraw(dxCommon->GetCommandList());

	// ★ デバッグONなら DebugCamera の Camera、OFFなら通常 camera_
	const KamataEngine::Camera& activeCam = (isDebugCameraActive_ && debugCamera_) ? debugCamera_->GetCamera() : camera_;

	// ★ 空を最初に描く（深度の上書きを避けるため）
	if (skydome_) {
		skydome_->Draw(activeCam);
	}

	for (auto& row : worldTransformBlocks_) { // 外側＝縦方向
		for (auto* wt : row) {                // 内側＝横方向
			if (!wt)
				continue; // 穴あき対応
			modelBlock_->Draw(*wt, activeCam);
		}
	}

	// 単体モデルの可視性テスト（必要なら一時的に有効化）
	// model_->Draw(worldTransform_, debugCamera_->GetCamera(), textureHandle_);

	// ★ フェーズが Game のときだけプレイヤーを描く（Death 中は描かない）
	if (phase_ == ScenePhase::Game && player_) {
		player_->Draw(activeCam);
	}

	// ★ ゴール（静止表示）
	if (goal_) {
		goal_->Draw(activeCam);
	}

	for (auto& e : enemies_) {
		e->Draw(activeCam);
	}

	// ★ Death パーティクルの描画
	if (phase_ == ScenePhase::Death && modelSphere_) {
		for (auto& dp : deathParticles_) {
			if (dp.life <= 0.0f || !dp.wt)
				continue;
			dp.wt->TransferMatrix(); // 念のため
			modelSphere_->Draw(*dp.wt, activeCam);
		}
	}

	if (phase_ == ScenePhase::Clear) {
		// ★ パーティクル描画（modelBlock_ または modelSphere_ でOK）
		if (modelBlock_) {
			for (auto& p : clearParticles_) {
				if (!p.wt || p.life <= 0.0f)
					continue;
				// あなたの描画APIに合わせて呼び出してください
				// 例）modelBlock_->Draw(*p.wt, viewProjection_, textureIdNone_);
				modelBlock_->Draw(*p.wt, activeCam); // ←あなたのエンジンの実際の引数に合わせる
			}
		}

		// ★ クリアUI（テキストなど）
		if (clear_) {
			clear_->Draw();
		}
		return;
	}

	Model::PostDraw();
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

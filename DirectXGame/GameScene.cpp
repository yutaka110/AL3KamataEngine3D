#include "GameScene.h"
#include "MapLoader.h"
#include "TileCollision.h"

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


} // namespace




GameScene::~GameScene() {
	delete sprite_;
	delete model_;
	// 自キャラの解放
	delete player_;
	delete debugCamera_;
	//delete modelSkydome_;
	delete modelBlock_;
	delete skydome_;
	delete title_;
	// ブロックの WT を解放
	for (std::vector<KamataEngine::WorldTransform*>& row : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* wt : row) {
			delete wt; // nullptr なら何もしない
		}
		row.clear(); // 行ベクタを空に
	}
	worldTransformBlocks_.clear(); // 外側も空に
}

void GameScene::Initialize() {
	// ファイル名を指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("mario.jpg");

	

	// サウンドデータを読み込む
	soundDataHandle_ = Audio::GetInstance()->LoadWave("mokugyo.wav");

	// 音声再生
	voiceHandle_ = Audio::GetInstance()->PlayWave(soundDataHandle_, true);

	Audio::GetInstance()->PlayWave(soundDataHandle_);

	// スプライトインスタンスの生成
	sprite_ = Sprite::Create(textureHandle_, {100, 50});

	

	// ワールドトランスファームの初期化
	worldTransform_.Initialize();


	// 3Dモデル（既存の model_ をブロックにも使い回し）
	model_ = Model::Create();

	modelBlock_ = Model::CreateFromOBJ("cube", true);

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
			wt->scale_ = {1.08f, 1.08f, 1.08f}; // 小さめにしたいときは 1.6f など
			wt->TransferMatrix();

			worldTransformBlocks_[y][x] = wt;
		}
	}


	// 既存の pitchX/pitchY/originX/originY を保持
	tilePitchX_ = pitchX;
	tilePitchY_ = pitchY;
	tileOriginX_ = originX;
	tileOriginY_ = originY;

	// half は最初に見つかったブロックの scale から推定
	for (uint32_t y = 0; y < rows; ++y) {
		for (uint32_t x = 0; x < cols; ++x) {
			if (worldTransformBlocks_[y][x]) {
				tileHalfX_ = worldTransformBlocks_[y][x]->scale_.x * 0.5f;
				tileHalfY_ = worldTransformBlocks_[y][x]->scale_.y * 0.5f;
				y = rows;
				break;
			}
		}
	}


	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280,720);
	

	// カメラの初期化
	camera_.Initialize();
	camera_.farZ = 20000.0f; // または SetFar(20000.0f);
	camera_.nearZ = 0.1f;


	// 3Dモデルの生成
    //modelSkydome_ = Model::CreateFromOBJ("skydome", true);
	// ★ スカイドーム生成（最初は原点固定でOK / 追従は後述）
	skydome_ = new Skydome();
	skydome_->Initialize("skydome", /*scale=*/1200.0f); // シーンに合わせて調整


	// 自キャラの生成
	player_ = new Player();
	
	
	//int sx = 0, sy = (int)mapData_.size() - 1; // 最下段の0列目タイル
	KamataEngine::Vector3 spawnPos{30.0f, 22.0f, 10.0f};


	// 自キャラの初期化
	player_->Initialize(model_, textureHandle_, &camera_,spawnPos);

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
		if (title_) title_->Update();
		 // SPACE が押されると finished_ が true になる実装【】
		    if (title_ && title_->IsFinished()) {
			delete title_;
			title_ = nullptr;
			phase_ = ScenePhase::Game; // 切り替え
			
		}
		else{
			return; // まだタイトル中ならゲームの更新はしない
			
		}
		
	}

	// スプライトの今の座標を取得
	Vector2 position = sprite_->GetPosition();

	// 座標を{2,1}移動
	position.x += 2.0f;
	position.y += 1.0f;

	// 移動した座標をスプライトに反映
	sprite_->SetPosition(position);




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


	// 自キャラの更新
	player_->Update();

	// player_->Update(); の直後
	TileField tf;
	tf.originX = tileOriginX_;
	tf.originY = tileOriginY_;
	tf.pitchX = tilePitchX_;
	tf.pitchY = tilePitchY_;
	tf.grid = &mapData_;
	ResolvePlayerVsTilemap(*player_, tf);


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
	if (sprite_) {
		sprite_->Draw();
	}
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

	if (player_) {
		player_->Draw(activeCam);
	}

	Model::PostDraw();
}

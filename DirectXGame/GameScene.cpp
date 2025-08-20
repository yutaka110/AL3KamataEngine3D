#include "GameScene.h"
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
} // namespace


GameScene::~GameScene() {
	delete sprite_;
	delete model_;
	// 自キャラの解放
	delete player_;
	delete debugCamera_;
	delete modelSkydome_;
	delete modelBlock_;

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

// 要素数
	const uint32_t kNumBlockVertical = 10;   // 縦
	const uint32_t kNumBlockHorizontal = 20; // 横

	// ブロック1個分のサイズ
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;

	// 外側ベクタ（縦方向）を行数分リサイズ
	worldTransformBlocks_.resize(kNumBlockVertical);

	for (uint32_t y = 0; y < kNumBlockVertical; ++y) {

		// 内側ベクタ（横方向）を列数分リサイズ
		worldTransformBlocks_[y].resize(kNumBlockHorizontal);

		for (uint32_t x = 0; x < kNumBlockHorizontal; ++x) {

			// 1ブロック生成
			auto* wt = new WorldTransform();
			wt->Initialize();

			// 配置座標（左上基準）
			wt->translation_.x = x * kBlockWidth;
			wt->translation_.y = y * kBlockHeight;
			wt->translation_.z = 10.0f; // 見やすく前に出す
			wt->scale_ = {2.0f, 2.0f, 2.0f};

			wt->TransferMatrix();

			worldTransformBlocks_[y][x] = wt;
		}
	}





	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280,720);

	// カメラの初期化
	camera_.Initialize();

	// 3Dモデルの生成
//	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	// 自キャラの生成
	player_ = new Player();
	// 自キャラの初期化
	player_->Initialize(model_, textureHandle_, &camera_);
}

void GameScene::Update() {
	// スペースキーを押した瞬間
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {

		// 音声停止
		Audio::GetInstance()->StopWave(voiceHandle_);
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

	// デバッグカメラの更新
	debugCamera_->Update();
}

void GameScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// --------- 2D（スプライト） ---------
	Sprite::PreDraw(dxCommon->GetCommandList());
	if (sprite_) {
		sprite_->Draw();
	}
	Sprite::PostDraw();

	// --------- 3D（モデル） ---------
	Model::PreDraw(dxCommon->GetCommandList());

	for (auto& row : worldTransformBlocks_) { // 外側＝縦方向
		for (auto* wt : row) {                // 内側＝横方向
			if (!wt)
				continue; // 穴あき対応
			modelBlock_->Draw(*wt, camera_);
		}
	}


	// 単体モデルの可視性テスト（必要なら一時的に有効化）
	// model_->Draw(worldTransform_, debugCamera_->GetCamera(), textureHandle_);

	if (player_) {
		player_->Draw();
	}

	Model::PostDraw();
}

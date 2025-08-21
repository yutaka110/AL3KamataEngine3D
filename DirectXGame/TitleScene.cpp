#include "TitleScene.h"
#include "KamataEngine.h"

using namespace KamataEngine;

TitleScene::~TitleScene() { delete sprite_; }

void TitleScene::Initialize() {
	// 適当な画像を resources に置いておく（無ければ白板表示でもOK）
	tex_ = TextureManager::Load("mario.jpg");                           // 無ければフォールバックでも良い
	sprite_ = Sprite::Create(tex_, {640.0f - 256.0f, 360.0f - 128.0f}); // 例: 512x256想定
	finished_ = false;
}

void TitleScene::Update() {
	// スペースで進む
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		finished_ = true;
	}
}

void TitleScene::Draw() {
	auto* dx = DirectXCommon::GetInstance();

	// 2D
	Sprite::PreDraw(dx->GetCommandList());
	if (sprite_)
		sprite_->Draw();

	// 画面中央に説明テキストを重ねたければ、別の Sprite/フォント機能で
	// （フォントAPIが無ければ画像で文字を作ってください）

	Sprite::PostDraw();

	// （タイトルは2DだけでOK。3Dは描かない）
}

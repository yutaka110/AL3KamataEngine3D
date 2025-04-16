#include <Windows.h>
#include "KamataEngine.h"
#include"GameScene.h"
// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;

DirectXCommon* dxCommon = DirectXCommon::GetInstance();
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_17_タケイ_ユタカ_AL3");
	
	//ゲームシーンのインスタンスを生成
	GameScene* gameScene = new GameScene();

	//ゲームシーンの初期化
	gameScene->Initialize();

	while (true) {

		if (KamataEngine::Update())
		{
			break;
		}

		//ゲームシーンの更新処理
		gameScene->Update();


		//描画開始
		dxCommon->PreDraw();

		//ゲームシーンの描画処理
		gameScene->Draw();

		//描画終了
		dxCommon->PostDraw();
	}
	
	
	//エンジンの終了処理
	KamataEngine::Finalize();
	delete gameScene;
	gameScene = nullptr;

	return 0;
}


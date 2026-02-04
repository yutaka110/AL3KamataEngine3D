#include <Windows.h>
#include "KamataEngine.h"
#include"GameScene.h"
// Windowsアプリでのエントリーポイント(main関数)
using namespace KamataEngine;

DirectXCommon* dxCommon = DirectXCommon::GetInstance();
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_17_タケイ_ユタカ_見透しの舞台");
	
	//ゲームシーンのインスタンスを生成
	GameScene* gameScene = new GameScene();

	//ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	//ゲームシーンの初期化
	gameScene->Initialize();

	while (true) {

		if (KamataEngine::Update())
		{
			break;
		}

		//Imgui受付開始
		imguiManager->Begin();

		//ゲームシーンの更新処理
		gameScene->Update();

		//Imgui受付終了
		imguiManager->End();

		//描画開始
		dxCommon->PreDraw();

		//ゲームシーンの描画処理
		gameScene->Draw();

		//ImGui描画
		imguiManager->Draw();

		//描画終了
		dxCommon->PostDraw();
	}
	
	
	//エンジンの終了処理
	KamataEngine::Finalize();
	delete gameScene;
	gameScene = nullptr;

	return 0;
}


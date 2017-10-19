#include "DyerScene.h"
#include "editor-support/cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include "Editor.h"

using namespace cocostudio::timeline;

namespace DyerEditor
{
	// on "init" you need to initialize your instance
	bool DyerScene::init()
	{
		// 1. super init first
		if (!Scene::init())
		{
			return false;
		}		
		auto rootNode = CSLoader::createNode("res/Editor.csb");
		addChild(rootNode);
		auto editor = new Editor(rootNode);
		editor->init();

		return true;
	}

}
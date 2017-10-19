#pragma once

#include "editor-support/cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include <unordered_map>
#include <map>
#include "DyerParser.h"

namespace DyerEditor
{
	class Editor : public cocos2d::Ref
	{
	public:
		Editor(cocos2d::Node* root);
		~Editor();

		bool init();
		void update(float dt);

	private:
		void onSlide(cocos2d::Ref* ref, cocos2d::ui::Slider::EventType type);
		
		void onSelFile(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type);
		void onSelCase(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type);
		void onSelPart(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type);

		void onBtnSave(cocos2d::Ref* ref);
		void onBtnRestore(cocos2d::Ref* ref);
		void onBtnAdd(cocos2d::Ref* ref);			
		void onBtnSrc(cocos2d::Ref* ref);
		void onBtnCase(cocos2d::Ref* ref);
		void onBtnTips(cocos2d::Ref* ref);

		void onCheck(cocos2d::Ref* ref, cocos2d::ui::CheckBoxEventType type);

		void onBrowserSource(cocos2d::Ref* ref);

	private:
		void _createShowSprite();		
		void _refreshHSLInfo(bool bSlider);
		void _colorShader(cocos2d::Sprite* sprite);
		void _clearDyerData();
		void __removeAddItem();

	private:
		void _updateCaseCells();
		void _updatePartCells();

	private:
		int caseLoadIdx; //用于加载计数
		int partLoadIdx;
		int fileLoadIdx;

		std::string curFpath;
		std::string curFdir;
		std::string curFname;
		int curFindex;

		unsigned char curCaseId;
		unsigned char curPartId;

		bool curIsAnimation;

		bool bSilderDirty;

	private:		
		std::vector<cocos2d::Sprite*> aniSprtes;

	private:
		cocos2d::Node* _root;
		cocos2d::ui::Text *valR, *valG, *valB, *textAnimation;
		cocos2d::ui::Slider *sliR, *sliG, *sliB;
		cocos2d::ui::ListView *lstCase, *lstPart, *lstFile;
		cocos2d::ui::Button *btnSave, *btnRestore, *btnAdd, *btnCase, *btnSrc, *btnBrowser, *btnTips, *btnClose;
		cocos2d::ui::ScrollView *scrollShow, *tipView;
		cocos2d::ui::CheckBox *checkAnimation;

		cocos2d::Texture2D *blockTexture;
		DyerCaseVec dyerVec;

		typedef std::map<unsigned char, std::string> PartNameMap;
		PartNameMap partNameMap;

		std::vector<std::string> files;
	};
}
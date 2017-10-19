#pragma once

#include "editor-support/cocostudio/CocoStudio.h"
#include "ui/CocosGUI.h"
#include <unordered_map>

namespace DyerEditor
{
	class FileCell : public cocos2d::ui::Layout
	{
	public:
		CREATE_FUNC(FileCell);
		virtual bool init();
		void setData(const std::string& fileName, const std::string& textStr);		
		std::string getFileName();
	private:
		std::string fname;
		cocos2d::Sprite *sprite;
		cocos2d::ui::Text *text;
	};


	
	class CaseCell : public cocos2d::ui::Layout
	{
	public:
		CREATE_FUNC(CaseCell);
		virtual bool init();

		enum { NOR, DATA, SEL};
		void setState(char state);
		void setCallback(std::function<void(cocos2d::ui::Layout*)> callb);
		void setCaseId(unsigned char caseId, bool showBtn);
		unsigned char getCaseId();
		void onBtnClick(cocos2d::Ref* sender);

		void setString(std::string& str);
		
	private:
		cocos2d::LayerColor* layer;
		char state;
		unsigned char caseId;
		cocos2d::ui::Button *btnRemove;
		cocos2d::ui::Text *text;
		std::function<void(cocos2d::ui::Layout*)> callback;
	};
}
#include "EditorCell.h"
#include "cocos2d.h"
USING_NS_CC;

namespace DyerEditor
{
	#define BINDCONTROL(root, node) node = dynamic_cast<decltype(node)>(root->getChildByName(#node))	

	bool FileCell::init()
	{
		bool ret = cocos2d::ui::Layout::init();
		if (ret)
		{
			fname = "";			

			setTouchEnabled(true);
			auto rootNode = CSLoader::createNode("res/FileCell.csb");
			setSize(rootNode->getContentSize());
			rootNode->setAnchorPoint(cocos2d::Vec2(0, 0));

			cocos2d::LayerColor* layer = cocos2d::LayerColor::create(Color4B::Color4B(100, 66, 100, 255));
			layer->setContentSize(getSize());
			addChild(layer);

			addChild(rootNode);

			BINDCONTROL(rootNode, sprite);
			BINDCONTROL(rootNode, text);
			text->ignoreContentAdaptWithSize(false);
			text->setContentSize(cocos2d::Size(320, 116));
			
		}
		return ret;
	}

	void FileCell::setData(const std::string& fileName, const std::string& textStr)
	{
		if (fname.compare(fileName) != 0)
		{
			sprite->setTexture(fileName);
			sprite->setScale(1);
			cocos2d::Size size = sprite->getContentSize();
			int max = size.width;
			if (max < size.height)
			{
				max = size.height;
			}
			sprite->setScale(150.0 / max);

			text->setString(textStr);
			fname = fileName;
		}		
	}

	std::string FileCell::getFileName()
	{
		return fname;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CaseCell::init()
	{
		bool ret = cocos2d::ui::Layout::init();
		if (ret)
		{
			caseId = -1;
			callback = nullptr;
			setTouchEnabled(true);
			auto root = CSLoader::createNode("res/CaseCell.csb");
			setSize(root->getContentSize());
			root->setAnchorPoint(cocos2d::Vec2(0, 0));

			layer = cocos2d::LayerColor::create();
			layer->setContentSize(getSize());
			addChild(layer);
			addChild(root);

			BINDCONTROL(root, btnRemove);
			BINDCONTROL(root, text);
			state = -1;
			setState(NOR);
			btnRemove->addClickEventListener(std::bind(&CaseCell::onBtnClick, this, std::placeholders::_1));
		}
		return ret;
	}
	void CaseCell::setCallback(std::function<void(cocos2d::ui::Layout*)> callb)
	{
		callback = callb;
	}
	void CaseCell::setCaseId(unsigned char caseId, bool showBtn)
	{
		if (this->caseId != caseId)
		{
			this->caseId = caseId;
			text->setString(CCString::createWithFormat("Solution: %d", caseId)->getCString());
			btnRemove->setVisible(showBtn);
		}		
	}
	void CaseCell::onBtnClick(cocos2d::Ref* sender)
	{
		if (callback)
		{
			callback(this);
		}
	}

	void CaseCell::setString(std::string& str)
	{
		text->setString(str);
	}

	unsigned char CaseCell::getCaseId()
	{
		return caseId;
	}

	void CaseCell::setState(char state)
	{
		if (this->state == state)
		{
			return;
		}
		this->state == state;
		if (NOR == state)
		{
			layer->setLayerColor(Color4B::BLUE);
		}
		else if (DATA == state)
		{
			layer->setLayerColor(Color4B::MAGENTA);
		}
		else
		{
			layer->setLayerColor(Color4B::ORANGE);
		}
	}

}
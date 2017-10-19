#include "Editor.h"
#include "json/document.h"
#include "json/filestream.h"
#include "EditorCell.h"
#include "DyerParser.h"


#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#include <ShlObj.h>
#include <regex>
#else
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

const float fEPSINON = 0.00001;
//const double dEPSINON = 0.00000001;

#include "cocos2d.h"

static DyerParser* dyerParser = new DyerParser();

bool endsWith(const std::string& str, const std::string& pattern)
{
	size_t thisLen = str.length();
	size_t patternLen = pattern.length();
	if (thisLen < patternLen || patternLen == 0)
		return false;

	std::string endOfThis = str.substr(thisLen - patternLen, patternLen);
	return (endOfThis == pattern);
}

void splitFilename(const std::string& qualifiedName, std::string& outBasename, std::string& outPath)
{
	std::string path = qualifiedName;
	// Replace \ with / first
	std::replace(path.begin(), path.end(), '\\', '/');
	// split based on final /
	size_t i = path.find_last_of('/');
	size_t j = path.find_last_of('.');

	outPath.clear();
	if (i == std::string::npos && j != std::string::npos)
	{
		outBasename = path.substr(0, j);
	}
	else if (i != std::string::npos && j != std::string::npos)
	{
		outPath = path.substr(0, i + 1);
		outBasename = path.substr(i + 1, j - i - 1);
	}
	else
	{
		outBasename = path;
	}
}


std::string replaceAll(const char* pszText, const char* pszSrc, const char* pszDest)
{
	std::string ret;

	if (pszText && pszSrc && pszDest)
	{
		ret = pszText;
		std::string::size_type pos = 0;
		std::string::size_type srcLen = strlen(pszSrc);
		std::string::size_type desLen = strlen(pszDest);
		pos = ret.find(pszSrc, pos);
		while ((pos != std::string::npos))
		{
			ret = ret.replace(pos, srcLen, pszDest);
			pos = ret.find(pszSrc, (pos + desLen));
		}
	}

	return ret;
}

void dfsFolder(std::string folderPath, std::vector<std::string> &fileVec, std::string& suffix)
{
	bool useSuffix = suffix.size() > 0 ? true : false;
#ifdef WIN32
	_finddata_t FileInfo;
	std::string strfind = folderPath + "\\*";
	long Handle = _findfirst(strfind.c_str(), &FileInfo);

	if (Handle == -1L)
	{
		CCLOG("can not match the folder path");
		return;
	}

	do{
		//判断是否有子目录
		if (FileInfo.attrib & _A_SUBDIR)
		{
			//这个语句很重要
			if ((strcmp(FileInfo.name, ".") != 0) && (strcmp(FileInfo.name, "..") != 0) && (strcmp(FileInfo.name, ".svn") != 0))
			{
				std::string newPath = folderPath + "\\" + FileInfo.name;
				dfsFolder(newPath, fileVec, suffix);
			}
		}
		else
		{
			std::string filename = (folderPath + "\\" + FileInfo.name);
			if (useSuffix && endsWith(filename, suffix))
				fileVec.push_back(filename);
		}
	} while (_findnext(Handle, &FileInfo) == 0);

	_findclose(Handle);
#else
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	if ((dp = opendir(folderPath.c_str())) == NULL) {
		CCLOG("cannot open directory: %s\n", folderPath.c_str());		
		return;
	}
	chdir(folderPath.c_str());
	
	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) {

			if (strcmp(".", entry->d_name) == 0 ||
				strcmp("..", entry->d_name) == 0 || 
				strcmp(".svn", entry->d_name) == 0)
				continue;
			
			dfsFolder((folderPath + "/" + entry->d_name), fileVec, suffix);
		}
		else {
			std::string filename = entry->d_name;
			if (useSuffix && endsWith(filename, suffix))				
				fileVec.push_back((folderPath + "/" + filename));
		}
	}
	chdir("..");
	closedir(dp);
#endif
}
//////////////////////////////////////////////////////////////////////////
namespace DyerEditor
{
	unsigned int CONFIG_MAX_CASE_ID = 0;
	cocos2d::Node* bindControl(cocos2d::Node *root, const char* name)
	{
		cocos2d::Node* n = root->getChildByName(name);
		if (n == nullptr)
		{
			auto children = root->getChildren();
			for (const auto& child : children)
			{
				n = bindControl(child, name);
				if (n)
				{
					return n;
				}
			}
		}
		return n;
	}
#define BINDCONTROL(root, node) node = dynamic_cast<decltype(node)>(bindControl(root, #node));
//#define BINDCONTROL(root, node) node = dynamic_cast<decltype(node)>(root->getChildByName(#node))
	
	Editor::Editor(cocos2d::Node* root)
	{
		root->setContentSize(Director::getInstance()->getVisibleSize());
		cocos2d::ui::Helper::doLayout(root);
		BINDCONTROL(root, valR);
		BINDCONTROL(root, valG);
		BINDCONTROL(root, valB);

		BINDCONTROL(root, sliR);
		BINDCONTROL(root, sliG);
		BINDCONTROL(root, sliB);

		BINDCONTROL(root, btnAdd);
		BINDCONTROL(root, btnCase);
		BINDCONTROL(root, lstCase);
		BINDCONTROL(root, lstPart);

		BINDCONTROL(root, btnSrc);
		BINDCONTROL(root, btnBrowser);		
		BINDCONTROL(root, lstFile);

		BINDCONTROL(root, scrollShow);

		BINDCONTROL(root, btnSave);
		BINDCONTROL(root, btnRestore);

		BINDCONTROL(root, checkAnimation);
		BINDCONTROL(root, textAnimation);

		BINDCONTROL(root, btnTips);
		BINDCONTROL(root, btnClose);
		BINDCONTROL(root, tipView);
		
		using namespace cocos2d::ui;
		sliR->addEventListenerSlider(this, sliderpercentchangedselector(Editor::onSlide));
		sliG->addEventListenerSlider(this, sliderpercentchangedselector(Editor::onSlide));
		sliB->addEventListenerSlider(this, sliderpercentchangedselector(Editor::onSlide));
		
		lstFile->addEventListenerListView(this, listvieweventselector(Editor::onSelFile));
		lstCase->addEventListenerListView(this, listvieweventselector(Editor::onSelCase));
		lstPart->addEventListenerListView(this, listvieweventselector(Editor::onSelPart));

		btnSave->addClickEventListener(std::bind(&Editor::onBtnSave, this, std::placeholders::_1));
		btnRestore->addClickEventListener(std::bind(&Editor::onBtnRestore, this, std::placeholders::_1));
		btnAdd->addClickEventListener(std::bind(&Editor::onBtnAdd, this, std::placeholders::_1));
		btnCase->addClickEventListener(std::bind(&Editor::onBtnCase, this, std::placeholders::_1));
		btnSrc->addClickEventListener(std::bind(&Editor::onBtnSrc, this, std::placeholders::_1));
		btnBrowser->addClickEventListener(std::bind(&Editor::onBrowserSource, this, std::placeholders::_1));
		btnTips->addClickEventListener(std::bind(&Editor::onBtnTips, this, std::placeholders::_1));
		btnClose->addClickEventListener(std::bind(&Editor::onBtnTips, this, std::placeholders::_1));

		checkAnimation->addEventListenerCheckBox(this, checkboxselectedeventselector(Editor::onCheck));

		lstFile->setItemsMargin(6);
		lstCase->setItemsMargin(6);
		lstPart->setItemsMargin(6);

		curFpath = "";
		curFdir = "";
		curFname = "";
		curFindex = -1;

		curCaseId = -1;
		curPartId = -1;

		curIsAnimation = true;
		blockTexture = nullptr;
	}
	Editor::~Editor()
	{
		cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
		_clearDyerData();
	}

	bool Editor::init()
	{
		CONFIG_MAX_CASE_ID = 10;
		std::string colorPath = "";
		std::string srcPath = "";
		if (cocos2d::FileUtils::getInstance()->isFileExist("res/dyerEditor.json"))
		{
			std::string jsonStr = cocos2d::FileUtils::getInstance()->getStringFromFile("res/dyerEditor.json");
			rapidjson::Document jsonDict;
			jsonDict.Parse<0>(jsonStr.c_str());
			if (jsonDict.HasParseError())
			{
				CCLOG("WARNNING GetParseError %d\n", jsonDict.GetParseError());				
			}
			else
			{
				if (!jsonDict["SOLUTION_NUM"].IsNull())
				{
					CONFIG_MAX_CASE_ID = jsonDict["SOLUTION_NUM"].GetInt();
				}
				if (!jsonDict["COLOR_PATH"].IsNull())
				{
					colorPath = jsonDict["COLOR_PATH"].GetString();
				}
				if (!jsonDict["SRC_PATH"].IsNull())
				{
					srcPath = jsonDict["SRC_PATH"].GetString();
				}
			}			
		}		
		
		//color config
		rapidjson::Document doc;
		std::string colorJson;
		if (cocos2d::FileUtils::getInstance()->isFileExist(colorPath))
		{
			colorJson = cocos2d::FileUtils::getInstance()->getStringFromFile(colorPath);
		}
		else
		{
			colorJson = cocos2d::FileUtils::getInstance()->getStringFromFile("res/color.json");
		}		
		doc.Parse<0>(colorJson.c_str());
		if (doc.HasParseError() || !doc.IsArray())
		{
			CCLOG("WARNNING colorPath[%s] GetParseError %d\n", colorPath.c_str(), doc.GetParseError());
			return false;
		}

		partNameMap.clear();
		files.clear();

		int j = 0;//不明白，为什么不能直接用0
		unsigned char partArrCount = doc.Size();
		for (unsigned char i = 0; i < partArrCount; i++)
		{
			rapidjson::Value& val = doc[i];
			
			int partId = val[j].GetInt();
			partNameMap[partId] = val[1].GetString();
		}
		
		if (srcPath.empty())
		{
			srcPath = UserDefault::getInstance()->getStringForKey("SRC_PATH");
		}
		std::string suffix = ".arm";
		dfsFolder(srcPath, files, suffix);

		caseLoadIdx = 0;
		partLoadIdx = 0;
		fileLoadIdx = 0;
		bSilderDirty = false;
		cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, 0, false);		
		
		return true;
	}


	void Editor::update(float dt)
	{
		unsigned char partCount = partNameMap.size();
		unsigned int fileCount = files.size();
		
		if (caseLoadIdx < CONFIG_MAX_CASE_ID)
		{
			CaseCell* cell = CaseCell::create();			
			cell->setCaseId(caseLoadIdx + 1, false);
			lstCase->pushBackCustomItem(cell);
			++caseLoadIdx;
		}
		if (partLoadIdx < partCount)
		{
			std::string name = partNameMap[partLoadIdx + 1];
			CaseCell* cell = CaseCell::create();			
			cell->setString(name);
			lstPart->pushBackCustomItem(cell);
			++partLoadIdx;
		}
		if (fileLoadIdx < fileCount)
		{
			FileCell* cell = FileCell::create();
			std::string file = files[fileLoadIdx];
			std::string path = replaceAll(file.c_str(), "/", "/ ");
			cell->setData(file, path);
			lstFile->pushBackCustomItem(cell);
			++fileLoadIdx;
		}

		if (bSilderDirty)
		{
			if (aniSprtes.empty())
			{
				return;
			}
			bSilderDirty = false;


			float h = (sliR->getPercent() - 50.0)*3.6;
			float s = (sliG->getPercent() - 50.0)*0.02;
			float l = (sliB->getPercent() - 50.0)*0.02;

			DyerData* dd = dyerVec[curCaseId - 1];
			dd->harr[curPartId] = h;
			dd->sarr[curPartId] = s;
			dd->larr[curPartId] = l;

			_refreshHSLInfo(false);
		}
					
	}


	void Editor::onSlide(cocos2d::Ref* ref, cocos2d::ui::Slider::EventType type)
	{
		if (type == cocos2d::ui::Slider::EventType::ON_PERCENTAGE_CHANGED)
		{
			bSilderDirty = true;
		}
	}

	void Editor::onBtnSave(cocos2d::Ref* ref)
	{
		DyerParser::saveToFile(curFdir + "solution.armi", dyerVec);
	}

	void Editor::onBtnRestore(cocos2d::Ref* ref)
	{
		if (dyerVec.empty()) return;
		unsigned short size = aniSprtes.size();
		for (unsigned short i = 0; i < size; i++)
		{
			Sprite* s = aniSprtes[i];
			dyerParser->restore(s);			
		}
		DyerData* dd = dyerVec[curCaseId - 1];
		memset(dd->harr, 0, dd->len*sizeof(float));
		memset(dd->sarr, 0, dd->len*sizeof(float));
		memset(dd->larr, 0, dd->len*sizeof(float));		
		_refreshHSLInfo(true);
		_updatePartCells();
	}

	void Editor::__removeAddItem()
	{		
		lstCase->removeLastItem();

		unsigned char idx = dyerVec.size() - 1;
		delete dyerVec[idx];
		dyerVec.pop_back();
	}

	void Editor::onBtnAdd(cocos2d::Ref* ref)
	{
		auto items = lstCase->getItems();
		if (items.size()+1 > dyerVec.size())
		{
			DyerData *dd = new DyerData(partNameMap.size() + 1);
			dyerVec.push_back(dd);
		}
		CaseCell* cell = CaseCell::create();		
		cell->setCaseId(items.size() + 1, true);

		cell->setCallback([&](cocos2d::ui::Layout* sender){
			__removeAddItem();
		});
		lstCase->pushBackCustomItem(cell);		
	}

	void Editor::onBtnSrc(cocos2d::Ref* ref)
	{
		lstFile->setVisible(!lstFile->isVisible());
	}
	void Editor::onBtnCase(cocos2d::Ref* ref)
	{
		lstCase->setVisible(!lstCase->isVisible());
		lstPart->setVisible(!lstCase->isVisible());		
	}


	void Editor::onBtnTips(cocos2d::Ref* ref)
	{
		tipView->setVisible(!tipView->isVisible());		
	}

	void Editor::onSelCase(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type)
	{
		if (type != cocos2d::ui::ListViewEventType::LISTVIEW_ONSELECTEDITEM_END) return;

		lstCase->setVisible(false);
		lstPart->setVisible(true);

		int tempIdx = lstCase->getCurSelectedIndex() + 1;
		if (tempIdx == curCaseId) return;

		curPartId = 1;
		
		curCaseId = tempIdx;		
		
		_updateCaseCells();
		_updatePartCells();
		
		_refreshHSLInfo(true);
		
	}

	void Editor::onSelPart(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type)
	{
		if (type != cocos2d::ui::ListViewEventType::LISTVIEW_ONSELECTEDITEM_END) return;
		if (curCaseId > dyerVec.size()) return;
		
		int tempIdx = lstPart->getCurSelectedIndex() + 1;
		if (tempIdx == curPartId) return;
		
		curPartId = tempIdx;
		_updatePartCells();
		
		DyerData* dd = dyerVec[curCaseId - 1];
		float h = dd->harr[curPartId];
		float s = dd->sarr[curPartId];
		float l = dd->larr[curPartId];

		valR->setString(CCString::createWithFormat("%.2f", h)->getCString());
		valG->setString(CCString::createWithFormat("%.2f", s)->getCString());
		valB->setString(CCString::createWithFormat("%.2f", l)->getCString());
		
		sliR->setPercent(h / 3.6 + 50.0);
		sliG->setPercent(s / 0.02 + 50.0);
		sliB->setPercent(l / 0.02 + 50.0);		
	}

	void Editor::onSelFile(cocos2d::Ref* ref, cocos2d::ui::ListViewEventType type)
	{
		if (type != cocos2d::ui::ListViewEventType::LISTVIEW_ONSELECTEDITEM_END) return;
		int tempIdx = lstFile->getCurSelectedIndex() ;
		lstFile->setVisible(false);
		if (curFindex != tempIdx)
		{
			//重置
			curFindex = tempIdx;
			curFpath = files[curFindex];
			
			splitFilename(curFpath, curFname, curFdir);			

			curCaseId = 1;
			curPartId = 1;
			
			_clearDyerData();
			if (!DyerParser::getDataFromFile(curFdir + "solution.armi", dyerVec))
			{
				unsigned char partCount = partNameMap.size();
				dyerVec.resize(CONFIG_MAX_CASE_ID);

				for (char i = 0; i < CONFIG_MAX_CASE_ID; i++)
				{
					dyerVec[i] = new DyerData(partCount + 1);
				}
			}

			_updateCaseCells();
			_updatePartCells();

			auto items = lstCase->getItems();
			int total = items.size();
			while (total < dyerVec.size())
			{
				onBtnAdd(nullptr);
				total++;
			}
			

			blockTexture = cocos2d::Director::getInstance()->getTextureCache()->addImage(curFdir + curFname + ".block");
			cocos2d::SpriteFrameCache::getInstance()->addSpriteFramesWithFile(curFdir + curFname + ".plist");
			if (cocos2d::FileUtils::getInstance()->isFileExist(curFdir + curFname + "_a.plist"))
			{
				cocos2d::AnimationCache::getInstance()->addAnimationsWithFile(curFdir + curFname + "_a.plist");
			}			
			
			_createShowSprite();

			_refreshHSLInfo(true);
		}
	}

	void Editor::onCheck(cocos2d::Ref* ref, cocos2d::ui::CheckBoxEventType type)
	{
		bool bTemp = checkAnimation->isSelected();		
		if (bTemp)
		{
			textAnimation->setString("Animation");
		}
		else
		{
			textAnimation->setString("Static Img");
		}
		if (curIsAnimation != bTemp)
		{
			curIsAnimation = bTemp;
			_createShowSprite();
			if (blockTexture == nullptr) return;

			unsigned short size = aniSprtes.size();
			for (unsigned short i = 0; i < size; i++)
			{
				Sprite* s = aniSprtes[i];
				_colorShader(s);
			}
		}
	}

	void Editor::_createShowSprite()
	{
		aniSprtes.clear();
		scrollShow->removeAllChildren();
		if (curFindex == -1 || curFpath == "")
		{
			return;
		}

		cocos2d::Vec2 pos = cocos2d::Vec2::ZERO;		
		pos.x = scrollShow->getSize().width*0.5;
		if (cocos2d::FileUtils::getInstance()->isFileExist(curFdir + curFname + "_a.plist"))
		{
			if (curIsAnimation)
			{
				std::string path = cocos2d::FileUtils::getInstance()->fullPathForFilename(curFdir + curFname + "_a.plist");
				cocos2d::ValueMap dict = cocos2d::FileUtils::getInstance()->getValueMapFromFile(path);
				if (dict.find("animations") == dict.end())
				{
					CCLOG("path[%s] : No animations were found in provided dictionary");
					return;
				}

				cocos2d::Vec2 offset = cocos2d::Vec2::ZERO;
				const cocos2d::Value& animations = dict.at("animations");
				const cocos2d::ValueMap& animationsDict = animations.asValueMap();

				for (auto itr = animationsDict.cbegin(); itr != animationsDict.cend(); ++itr)
				{
					auto animation = cocos2d::AnimationCache::getInstance()->getAnimation(itr->first.c_str());
					auto ani = cocos2d::Animate::create(animation);
					Sprite* sprite = cocos2d::Sprite::create();
					sprite->runAction(cocos2d::RepeatForever::create(ani));
					ani->update(0.1);
					sprite->setAnchorPoint(cocos2d::Vec2(0.5, 0));
					auto frames = animation->getFrames();
					cocos2d::AnimationFrame *frame = frames.front();
					cocos2d::Rect r = frame->getSpriteFrame()->getRect();
					if (offset.y < r.size.height)
					{
						offset.y += r.size.height;
					}
					sprite->setPosition(pos);
					pos.y += r.size.height + 10;
					scrollShow->addChild(sprite);
					aniSprtes.push_back(sprite);
				}
				scrollShow->setInnerContainerSize(cocos2d::Size(pos.x * 2, pos.y + offset.y + 10));
			}
			else
			{
				pos.y = 10;
				Sprite* s = Sprite::create(curFpath);
				s->setAnchorPoint(cocos2d::Vec2(0.5, 0));
				s->setPosition(pos);
				scrollShow->addChild(s);
				aniSprtes.push_back(s);
				scrollShow->setInnerContainerSize(cocos2d::Size(pos.x * 2, s->getContentSize().height + 20));
			}
		}
		else
		{
			textAnimation->setString("Static Img");
			checkAnimation->setSelected(false);
			curIsAnimation = false;
			pos.y = 10;
			Sprite* s = Sprite::create(curFpath);
			s->setAnchorPoint(cocos2d::Vec2(0.5, 0));
			s->setPosition(pos);
			scrollShow->addChild(s);
			aniSprtes.push_back(s);
			scrollShow->setInnerContainerSize(cocos2d::Size(pos.x * 2, s->getContentSize().height + 20));
		}
		
	}

	void Editor::_refreshHSLInfo(bool bSlider)
	{
		if (blockTexture == nullptr || dyerVec.size() < curCaseId)
		{
			valR->setString("0.0");
			valG->setString("0.0");
			valB->setString("0.0");
			sliR->setPercent(50.0);
			sliG->setPercent(50.0);
			sliB->setPercent(50.0);
			return;
		}

		
		DyerData* dd = dyerVec[curCaseId - 1];
		float h = dd->harr[curPartId];
		float s = dd->sarr[curPartId];
		float l = dd->larr[curPartId];

		valR->setString(CCString::createWithFormat("%.2f", h)->getCString());
		valG->setString(CCString::createWithFormat("%.2f", s)->getCString());
		valB->setString(CCString::createWithFormat("%.2f", l)->getCString());		

		unsigned short size = aniSprtes.size();
		for (unsigned short i = 0; i < size; i++)
		{
			Sprite* s = aniSprtes[i];
			_colorShader(s);
		}
		if (bSlider)
		{
			sliR->setPercent(h / 3.6 + 50.0);
			sliG->setPercent(s / 0.02 + 50.0);
			sliB->setPercent(l / 0.02 + 50.0);
		}
	}

	void Editor::_colorShader(cocos2d::Sprite* sprite)
	{		
		cocos2d::GLProgram* program = cocos2d::GLProgram::createWithByteArrays(cc_dyer_vsh, cc_dyer_fsh);
		cocos2d::GLProgramState* pState = GLProgramState::getOrCreateWithGLProgram(program);
		sprite->setGLProgramState(pState);
		

		pState->setUniformTexture("u_dTex", blockTexture);
		
		GLint hs = program->getUniformLocationForName("u_dHs");
		GLint ss = program->getUniformLocationForName("u_dSs");
		GLint ls = program->getUniformLocationForName("u_dLs");
		
		DyerData* dd = dyerVec[curCaseId - 1];
		program->setUniformLocationWith1fv(hs, dd->harr, dd->len);
		program->setUniformLocationWith1fv(ss, dd->sarr, dd->len);
		program->setUniformLocationWith1fv(ls, dd->larr, dd->len);
	}

	void Editor::_clearDyerData()
	{
		unsigned char size = dyerVec.size();
		for (unsigned char i = 0; i < size; i++)
		{
			delete dyerVec[i];
		}
		dyerVec.clear();
	}	

	void Editor::_updateCaseCells()
	{
		btnCase->setTitleText(CCString::createWithFormat("Solution: %d", curCaseId)->getCString());
		auto items = lstCase->getItems();
		int total = items.size();
		for (int i = 0; i < total; i++)
		{
			CaseCell* c = (CaseCell*)items.at(i);
			if (i == curCaseId-1)
			{
				c->setState(CaseCell::SEL);
			}
			else
			{
				c->setState(CaseCell::NOR);
			}
		}
	}

	void Editor::_updatePartCells()
	{
		DyerData* dd = nullptr;
		if (curCaseId < dyerVec.size())
		{
			dd = dyerVec[curCaseId - 1];
		}
		
		auto items = lstPart->getItems();
		int total = items.size();
		for (int i = 0; i < total; i++)
		{
			CaseCell* c = (CaseCell*)items.at(i);
			if (i == curPartId - 1)
			{
				c->setState(CaseCell::SEL);
			}
			else
			{
				if (dd && ((abs(dd->harr[i + 1]) > fEPSINON) ||
					(abs(dd->sarr[i + 1]) > fEPSINON) ||
					(abs(dd->larr[i + 1]) > fEPSINON)))
				{
					c->setState(CaseCell::DATA);
				}
				else
				{
					c->setState(CaseCell::NOR);
				}
			}
		}
	}

	void Editor::onBrowserSource(cocos2d::Ref* ref)
	{
		lstFile->removeAllItems();
		files.clear();
		_clearDyerData();
		curFpath = "";
		curFdir = "";
		curFname = "";
		curFindex = -1;
		curCaseId = -1;
		curPartId = -1;

		_createShowSprite();
		_refreshHSLInfo(true);
#ifdef _WIN32
		BROWSEINFOA bs;
		char buff[MAX_PATH] = "\0";
		memset(&bs, 0, sizeof(BROWSEINFOA));
		bs.pszDisplayName = buff;
		bs.pidlRoot = NULL;
		bs.lpszTitle = "select dir";
		bs.ulFlags = BIF_RETURNONLYFSDIRS;
		bs.lParam = NULL;
		bs.lpfn = NULL;
		bs.hwndOwner = NULL;
		auto handle = SHBrowseForFolderA(&bs);
		if (handle)
		{
			SHGetPathFromIDListA(handle, buff);

			std::string suffix = ".arm";
			dfsFolder(buff, files, suffix);
			fileLoadIdx = 0;
			UserDefault::getInstance()->setStringForKey("SRC_PATH", buff);
			/*
			int fileCount = files.size();
			for (int i=0; i < fileCount; i++)
			{
				FileCell* cell = FileCell::create();
				std::string file = files[i];
				std::string path = replaceAll(file.c_str(), "/", "/ ");
				cell->setData(file, path);
				lstFile->pushBackCustomItem(cell);
			}
			*/
		}
#endif // !_WIN32

	}

}
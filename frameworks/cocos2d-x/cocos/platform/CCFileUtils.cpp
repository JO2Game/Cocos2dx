/****************************************************************************
Copyright (c) 2010-2013 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "platform/CCFileUtils.h"

#include <stack>

#include "base/CCData.h"
#include "base/ccMacros.h"
#include "base/CCDirector.h"
#include "platform/CCSAXParser.h"
#include "base/ccUtils.h"

#include "tinyxml2.h"
#ifdef MINIZIP_FROM_SYSTEM
#include <minizip/unzip.h>
#else // from our embedded sources
#include "unzip.h"
#endif
#include <sys/stat.h>

NS_CC_BEGIN

// Implement DictMaker

#if (CC_TARGET_PLATFORM != CC_PLATFORM_IOS) && (CC_TARGET_PLATFORM != CC_PLATFORM_MAC)

typedef enum
{
    SAX_NONE = 0,
    SAX_KEY,
    SAX_DICT,
    SAX_INT,
    SAX_REAL,
    SAX_STRING,
    SAX_ARRAY
}SAXState;

typedef enum
{
    SAX_RESULT_NONE = 0,
    SAX_RESULT_DICT,
    SAX_RESULT_ARRAY
}SAXResult;

class DictMaker : public SAXDelegator
{
public:
    SAXResult _resultType;
    ValueMap _rootDict;
    ValueVector _rootArray;

    std::string _curKey;   ///< parsed key
    std::string _curValue; // parsed value
    SAXState _state;

    ValueMap*  _curDict;
    ValueVector* _curArray;

    std::stack<ValueMap*> _dictStack;
    std::stack<ValueVector*> _arrayStack;
    std::stack<SAXState>  _stateStack;

public:
    DictMaker()
        : _resultType(SAX_RESULT_NONE)
    {
    }

    ~DictMaker()
    {
    }

    ValueMap dictionaryWithContentsOfFile(const std::string& fileName)
    {
        _resultType = SAX_RESULT_DICT;
        SAXParser parser;

        CCASSERT(parser.init("UTF-8"), "The file format isn't UTF-8");
        parser.setDelegator(this);

        parser.parse(fileName);
        return _rootDict;
    }

    ValueMap dictionaryWithDataOfFile(const char* filedata, int filesize)
    {
        _resultType = SAX_RESULT_DICT;
        SAXParser parser;

        CCASSERT(parser.init("UTF-8"), "The file format isn't UTF-8");
        parser.setDelegator(this);

        parser.parse(filedata, filesize);
        return _rootDict;
    }

    ValueVector arrayWithContentsOfFile(const std::string& fileName)
    {
        _resultType = SAX_RESULT_ARRAY;
        SAXParser parser;

        CCASSERT(parser.init("UTF-8"), "The file format isn't UTF-8");
        parser.setDelegator(this);

        parser.parse(fileName);
        return _rootArray;
    }

    void startElement(void *ctx, const char *name, const char **atts)
    {
        CC_UNUSED_PARAM(ctx);
        CC_UNUSED_PARAM(atts);
        const std::string sName(name);
        if( sName == "dict" )
        {
            if(_resultType == SAX_RESULT_DICT && _rootDict.empty())
            {
                _curDict = &_rootDict;
            }

            _state = SAX_DICT;

            SAXState preState = SAX_NONE;
            if (! _stateStack.empty())
            {
                preState = _stateStack.top();
            }

            if (SAX_ARRAY == preState)
            {
                // add a new dictionary into the array
                _curArray->push_back(Value(ValueMap()));
                _curDict = &(_curArray->rbegin())->asValueMap();
            }
            else if (SAX_DICT == preState)
            {
                // add a new dictionary into the pre dictionary
                CCASSERT(! _dictStack.empty(), "The state is wrong!");
                ValueMap* preDict = _dictStack.top();
                (*preDict)[_curKey] = Value(ValueMap());
                _curDict = &(*preDict)[_curKey].asValueMap();
            }

            // record the dict state
            _stateStack.push(_state);
            _dictStack.push(_curDict);
        }
        else if(sName == "key")
        {
            _state = SAX_KEY;
        }
        else if(sName == "integer")
        {
            _state = SAX_INT;
        }
        else if(sName == "real")
        {
            _state = SAX_REAL;
        }
        else if(sName == "string")
        {
            _state = SAX_STRING;
        }
        else if (sName == "array")
        {
            _state = SAX_ARRAY;

            if (_resultType == SAX_RESULT_ARRAY && _rootArray.empty())
            {
                _curArray = &_rootArray;
            }
            SAXState preState = SAX_NONE;
            if (! _stateStack.empty())
            {
                preState = _stateStack.top();
            }

            if (preState == SAX_DICT)
            {
                (*_curDict)[_curKey] = Value(ValueVector());
                _curArray = &(*_curDict)[_curKey].asValueVector();
            }
            else if (preState == SAX_ARRAY)
            {
                CCASSERT(! _arrayStack.empty(), "The state is wrong!");
                ValueVector* preArray = _arrayStack.top();
                preArray->push_back(Value(ValueVector()));
                _curArray = &(_curArray->rbegin())->asValueVector();
            }
            // record the array state
            _stateStack.push(_state);
            _arrayStack.push(_curArray);
        }
        else
        {
            _state = SAX_NONE;
        }
    }

    void endElement(void *ctx, const char *name)
    {
        CC_UNUSED_PARAM(ctx);
        SAXState curState = _stateStack.empty() ? SAX_DICT : _stateStack.top();
        const std::string sName((char*)name);
        if( sName == "dict" )
        {
            _stateStack.pop();
            _dictStack.pop();
            if ( !_dictStack.empty())
            {
                _curDict = _dictStack.top();
            }
        }
        else if (sName == "array")
        {
            _stateStack.pop();
            _arrayStack.pop();
            if (! _arrayStack.empty())
            {
                _curArray = _arrayStack.top();
            }
        }
        else if (sName == "true")
        {
            if (SAX_ARRAY == curState)
            {
                _curArray->push_back(Value(true));
            }
            else if (SAX_DICT == curState)
            {
                (*_curDict)[_curKey] = Value(true);
            }
        }
        else if (sName == "false")
        {
            if (SAX_ARRAY == curState)
            {
                _curArray->push_back(Value(false));
            }
            else if (SAX_DICT == curState)
            {
                (*_curDict)[_curKey] = Value(false);
            }
        }
        else if (sName == "string" || sName == "integer" || sName == "real")
        {
            if (SAX_ARRAY == curState)
            {
                if (sName == "string")
                    _curArray->push_back(Value(_curValue));
                else if (sName == "integer")
                    _curArray->push_back(Value(atoi(_curValue.c_str())));
                else
                    _curArray->push_back(Value(utils::atof(_curValue.c_str())));
            }
            else if (SAX_DICT == curState)
            {
                if (sName == "string")
                    (*_curDict)[_curKey] = Value(_curValue);
                else if (sName == "integer")
                    (*_curDict)[_curKey] = Value(atoi(_curValue.c_str()));
                else
                    (*_curDict)[_curKey] = Value(utils::atof(_curValue.c_str()));
            }

            _curValue.clear();
        }

        _state = SAX_NONE;
    }

    void textHandler(void *ctx, const char *ch, int len)
    {
        CC_UNUSED_PARAM(ctx);
        if (_state == SAX_NONE)
        {
            return;
        }

        SAXState curState = _stateStack.empty() ? SAX_DICT : _stateStack.top();
        const std::string text = std::string((char*)ch,len);

        switch(_state)
        {
        case SAX_KEY:
            _curKey = text;
            break;
        case SAX_INT:
        case SAX_REAL:
        case SAX_STRING:
            {
                if (curState == SAX_DICT)
                {
                    CCASSERT(!_curKey.empty(), "key not found : <integer/real>");
                }

                _curValue.append(text);
            }
            break;
        default:
            break;
        }
    }
};

ValueMap FileUtils::getValueMapFromFile(const std::string& filename)
{
    const std::string fullPath = fullPathForFilename(filename);
    DictMaker tMaker;
    return tMaker.dictionaryWithContentsOfFile(fullPath);
}

ValueMap FileUtils::getValueMapFromData(const char* filedata, int filesize)
{
    DictMaker tMaker;
    return tMaker.dictionaryWithDataOfFile(filedata, filesize);
}

ValueVector FileUtils::getValueVectorFromFile(const std::string& filename)
{
    const std::string fullPath = fullPathForFilename(filename);
    DictMaker tMaker;
    return tMaker.arrayWithContentsOfFile(fullPath);
}


/*
 * forward statement
 */
static tinyxml2::XMLElement* generateElementForArray(const ValueVector& array, tinyxml2::XMLDocument *doc);
static tinyxml2::XMLElement* generateElementForDict(const ValueMap& dict, tinyxml2::XMLDocument *doc);

/*
 * Use tinyxml2 to write plist files
 */
bool FileUtils::writeToFile(const ValueMap& dict, const std::string &fullPath)
{
    return writeValueMapToFile(dict, fullPath);
}

bool FileUtils::writeValueMapToFile(const ValueMap& dict, const std::string& fullPath)
{
    tinyxml2::XMLDocument *doc = new (std::nothrow)tinyxml2::XMLDocument();
    if (nullptr == doc)
        return false;

    tinyxml2::XMLDeclaration *declaration = doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
    if (nullptr == declaration)
    {
        delete doc;
        return false;
    }

    doc->LinkEndChild(declaration);
    tinyxml2::XMLElement *docType = doc->NewElement("!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
    doc->LinkEndChild(docType);

    tinyxml2::XMLElement *rootEle = doc->NewElement("plist");
    rootEle->SetAttribute("version", "1.0");
    if (nullptr == rootEle)
    {
        delete doc;
        return false;
    }
    doc->LinkEndChild(rootEle);

    tinyxml2::XMLElement *innerDict = generateElementForDict(dict, doc);
    if (nullptr == innerDict)
    {
        delete doc;
        return false;
    }
    rootEle->LinkEndChild(innerDict);

    bool ret = tinyxml2::XML_SUCCESS == doc->SaveFile(getSuitableFOpen(fullPath).c_str());

    delete doc;
    return ret;
}

bool FileUtils::writeValueVectorToFile(const ValueVector& vecData, const std::string& fullPath)
{
    tinyxml2::XMLDocument *doc = new (std::nothrow)tinyxml2::XMLDocument();
    if (nullptr == doc)
        return false;

    tinyxml2::XMLDeclaration *declaration = doc->NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\"");
    if (nullptr == declaration)
    {
        delete doc;
        return false;
    }

    doc->LinkEndChild(declaration);
    tinyxml2::XMLElement *docType = doc->NewElement("!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\"");
    doc->LinkEndChild(docType);

    tinyxml2::XMLElement *rootEle = doc->NewElement("plist");
    rootEle->SetAttribute("version", "1.0");
    if (nullptr == rootEle)
    {
        delete doc;
        return false;
    }
    doc->LinkEndChild(rootEle);

    tinyxml2::XMLElement *innerDict = generateElementForArray(vecData, doc);
    if (nullptr == innerDict)
    {
        delete doc;
        return false;
    }
    rootEle->LinkEndChild(innerDict);

    bool ret = tinyxml2::XML_SUCCESS == doc->SaveFile(getSuitableFOpen(fullPath).c_str());

    delete doc;
    return ret;
}

/*
 * Generate tinyxml2::XMLElement for Object through a tinyxml2::XMLDocument
 */
static tinyxml2::XMLElement* generateElementForObject(const Value& value, tinyxml2::XMLDocument *doc)
{
    // object is String
    if (value.getType() == Value::Type::STRING)
    {
        tinyxml2::XMLElement* node = doc->NewElement("string");
        tinyxml2::XMLText* content = doc->NewText(value.asString().c_str());
        node->LinkEndChild(content);
        return node;
    }

    // object is integer
    if (value.getType() == Value::Type::INTEGER)
    {
        tinyxml2::XMLElement* node = doc->NewElement("integer");
        tinyxml2::XMLText* content = doc->NewText(value.asString().c_str());
        node->LinkEndChild(content);
        return node;
    }

    // object is real
    if (value.getType() == Value::Type::FLOAT || value.getType() == Value::Type::DOUBLE)
    {
        tinyxml2::XMLElement* node = doc->NewElement("real");
        tinyxml2::XMLText* content = doc->NewText(value.asString().c_str());
        node->LinkEndChild(content);
        return node;
    }

    //object is bool
    if (value.getType() == Value::Type::BOOLEAN) {
        tinyxml2::XMLElement* node = doc->NewElement(value.asString().c_str());
        return node;
    }

    // object is Array
    if (value.getType() == Value::Type::VECTOR)
        return generateElementForArray(value.asValueVector(), doc);

    // object is Dictionary
    if (value.getType() == Value::Type::MAP)
        return generateElementForDict(value.asValueMap(), doc);

    CCLOG("This type cannot appear in property list");
    return nullptr;
}

/*
 * Generate tinyxml2::XMLElement for Dictionary through a tinyxml2::XMLDocument
 */
static tinyxml2::XMLElement* generateElementForDict(const ValueMap& dict, tinyxml2::XMLDocument *doc)
{
    tinyxml2::XMLElement* rootNode = doc->NewElement("dict");

    for (const auto &iter : dict)
    {
        tinyxml2::XMLElement* tmpNode = doc->NewElement("key");
        rootNode->LinkEndChild(tmpNode);
        tinyxml2::XMLText* content = doc->NewText(iter.first.c_str());
        tmpNode->LinkEndChild(content);

        tinyxml2::XMLElement *element = generateElementForObject(iter.second, doc);
        if (element)
            rootNode->LinkEndChild(element);
    }
    return rootNode;
}

/*
 * Generate tinyxml2::XMLElement for Array through a tinyxml2::XMLDocument
 */
static tinyxml2::XMLElement* generateElementForArray(const ValueVector& array, tinyxml2::XMLDocument *pDoc)
{
    tinyxml2::XMLElement* rootNode = pDoc->NewElement("array");

    for(const auto &value : array) {
        tinyxml2::XMLElement *element = generateElementForObject(value, pDoc);
        if (element)
            rootNode->LinkEndChild(element);
    }
    return rootNode;
}

#else

/* The subclass FileUtilsApple should override these two method. */
ValueMap FileUtils::getValueMapFromFile(const std::string& filename) {return ValueMap();}
ValueMap FileUtils::getValueMapFromData(const char* filedata, int filesize) {return ValueMap();}
ValueVector FileUtils::getValueVectorFromFile(const std::string& filename) {return ValueVector();}
bool FileUtils::writeToFile(const ValueMap& dict, const std::string &fullPath) {return false;}

#endif /* (CC_TARGET_PLATFORM != CC_PLATFORM_IOS) && (CC_TARGET_PLATFORM != CC_PLATFORM_MAC) */

// Implement FileUtils
FileUtils* FileUtils::s_sharedFileUtils = nullptr;

void FileUtils::destroyInstance()
{
    CC_SAFE_DELETE(s_sharedFileUtils);
}

void FileUtils::setDelegate(FileUtils *delegate)
{
    if (s_sharedFileUtils)
        delete s_sharedFileUtils;

    s_sharedFileUtils = delegate;
}

FileUtils::FileUtils()
    : _writablePath("")
	// add by James
	, _firstSearchPath("")
	, _decodeCallback(nullptr)
	, _isEqualMd5Callback(nullptr)
	, m_filePath2CodeCall(nullptr)
{
}

FileUtils::~FileUtils()
{
}

bool FileUtils::writeStringToFile(const std::string& dataStr, const std::string& fullPath)
{
    Data data;
    data.fastSet((unsigned char*)dataStr.c_str(), dataStr.size());

    bool rv = writeDataToFile(data, fullPath);

    data.fastSet(nullptr, 0);
    return rv;
}

bool FileUtils::writeDataToFile(const Data& data, const std::string& fullPath)
{
    size_t size = 0;
    const char* mode = "wb";

    CCASSERT(!fullPath.empty() && data.getSize() != 0, "Invalid parameters.");

    auto fileutils = FileUtils::getInstance();
    do
    {
        // Read the file from hardware
        FILE *fp = fopen(fileutils->getSuitableFOpen(fullPath).c_str(), mode);
        CC_BREAK_IF(!fp);
        size = data.getSize();

        fwrite(data.getBytes(), size, 1, fp);

        fclose(fp);

        return true;
    } while (0);

    return false;
}

bool FileUtils::init()
{
    //_searchPathArray.push_back(_defaultResRootPath);
    _searchResolutionsOrderArray.push_back("");
    return true;
}

void FileUtils::purgeCachedEntries()
{
    _fullPathCache.clear();
	m_filecodeMap.clear();
}

static Data getData(const std::string& filename, bool forString)
{
    if (filename.empty())
    {
        return Data::Null;
    }

    Data ret;
    unsigned char* buffer = nullptr;
    size_t size = 0;
    size_t readsize;
    const char* mode = nullptr;

    if (forString)
        mode = "rt";
    else
        mode = "rb";

    auto fileutils = FileUtils::getInstance();
    do
    {
        // Read the file from hardware
        std::string fullPath = fileutils->fullPathForFilename(filename);
		//fullPath = fileutils->getFilecodeRet(filename);
        FILE *fp = fopen(fileutils->getSuitableFOpen(fullPath).c_str(), mode);
        CC_BREAK_IF(!fp);
        fseek(fp,0,SEEK_END);
        size = ftell(fp);
        fseek(fp,0,SEEK_SET);

        if (forString)
        {
            buffer = (unsigned char*)malloc(sizeof(unsigned char) * (size + 1));
            buffer[size] = '\0';
        }
        else
        {
            buffer = (unsigned char*)malloc(sizeof(unsigned char) * size);
        }

        readsize = fread(buffer, sizeof(unsigned char), size, fp);
        fclose(fp);

        if (forString && readsize < size)
        {
            buffer[readsize] = '\0';
        }
    } while (0);

    if (nullptr == buffer || 0 == readsize)
    {
        CCLOG("Get data from file %s failed", filename.c_str());
    }
    else
    {
		// add by James
		if (FileUtils::getInstance()->_decodeCallback)
		{
			unsigned int len;
			unsigned char* result = FileUtils::getInstance()->_decodeCallback((unsigned char*)buffer, size, &len);
			if (result)
			{
				ret.fastSet(result, len);
				return ret;
			}
		}
        ret.fastSet(buffer, readsize);
    }

    return ret;
}

std::string FileUtils::getStringFromFile(const std::string& filename)
{
    Data data = getData(filename, true);
    if (data.isNull())
        return "";

    std::string ret((const char*)data.getBytes());
    return ret;
}

Data FileUtils::getDataFromFile(const std::string& filename)
{
    return getData(filename, false);
}

unsigned char* FileUtils::getFileData(const std::string& filename, const char* mode, ssize_t *size)
{
    unsigned char * buffer = nullptr;
    CCASSERT(!filename.empty() && size != nullptr && mode != nullptr, "Invalid parameters.");
    *size = 0;
    do
    {
        // read the file from hardware
        std::string fullPath = fullPathForFilename(filename);
		//fullPath = getFilecodeRet(filename);
        FILE *fp = fopen(getSuitableFOpen(fullPath).c_str(), mode);
        CC_BREAK_IF(!fp);

        fseek(fp,0,SEEK_END);
        *size = ftell(fp);
        fseek(fp,0,SEEK_SET);
        buffer = (unsigned char*)malloc(*size);
        *size = fread(buffer,sizeof(unsigned char), *size,fp);
        fclose(fp);
    } while (0);

    if (!buffer)
    {
        std::string msg = "Get data from file(";
        msg.append(filename).append(") failed!");

        CCLOG("%s", msg.c_str());
    }
	// add by James
	else if (FileUtils::getInstance()->_decodeCallback)
	{
		unsigned int len;
		unsigned char* result = FileUtils::getInstance()->_decodeCallback((unsigned char*)buffer, *size, &len);
		if (result)
		{
			*size = len;
			return result;
		}
	}
    return buffer;
}

unsigned char* FileUtils::getFileDataFromZip(const std::string& zipFilePath, const std::string& filename, ssize_t *size)
{
    unsigned char * buffer = nullptr;
    unzFile file = nullptr;
    *size = 0;

    do
    {
        CC_BREAK_IF(zipFilePath.empty());

        file = unzOpen(FileUtils::getInstance()->getSuitableFOpen(zipFilePath).c_str());
        CC_BREAK_IF(!file);

        // FIXME: Other platforms should use upstream minizip like mingw-w64
#ifdef MINIZIP_FROM_SYSTEM
        int ret = unzLocateFile(file, filename.c_str(), NULL);
#else
        int ret = unzLocateFile(file, filename.c_str(), 1);
#endif
        CC_BREAK_IF(UNZ_OK != ret);

        char filePathA[260];
        unz_file_info fileInfo;
        ret = unzGetCurrentFileInfo(file, &fileInfo, filePathA, sizeof(filePathA), nullptr, 0, nullptr, 0);
        CC_BREAK_IF(UNZ_OK != ret);

        ret = unzOpenCurrentFile(file);
        CC_BREAK_IF(UNZ_OK != ret);

        buffer = (unsigned char*)malloc(fileInfo.uncompressed_size);
        int CC_UNUSED readedSize = unzReadCurrentFile(file, buffer, static_cast<unsigned>(fileInfo.uncompressed_size));
        CCASSERT(readedSize == 0 || readedSize == (int)fileInfo.uncompressed_size, "the file size is wrong");

        *size = fileInfo.uncompressed_size;
        unzCloseCurrentFile(file);
    } while (0);

    if (file)
    {
        unzClose(file);
    }

    return buffer;
}

std::string FileUtils::getNewFilename(const std::string &filename) const
{
    std::string newFileName;

    // in Lookup Filename dictionary ?
    auto iter = _filenameLookupDict.find(filename);

    if (iter == _filenameLookupDict.end())
    {
        newFileName = filename;
    }
    else
    {
        newFileName = iter->second.asString();
    }
    return newFileName;
}

std::string FileUtils::getPathForFilename(const std::string& filename, const std::string& resolutionDirectory, const std::string& searchPath) const
{
    std::string file = filename;
    std::string file_path = "";
    size_t pos = filename.find_last_of("/");
    if (pos != std::string::npos)
    {
        file_path = filename.substr(0, pos+1);
        file = filename.substr(pos+1);
    }

    // searchPath + file_path + resourceDirectory
    std::string path = searchPath;
    path += file_path;
    path += resolutionDirectory;

    path = getFullPathForDirectoryAndFilename(path, file);

    return path;
}


static bool replaceAll(const std::string& pszText, const std::string& pszSrc, const std::string& pszDest, std::string& out)
{
	out = pszText;	
	std::string::size_type pos = 0;
	std::string::size_type srcLen = pszSrc.length();
	std::string::size_type desLen = pszDest.length();
	pos = out.find(pszSrc, pos);
	if (pos == std::string::npos){
		return false;
	}
	while ((pos != std::string::npos)){
		out = out.replace(pos, srcLen, pszDest);
		pos = out.find(pszSrc, (pos + desLen));
	}
	return true;
}

std::string FileUtils::fullPathForFilename(const std::string &filename) const
{
    if (filename.empty()){
        return "";
    }
	// Already Cached ?
	std::unordered_map<std::string, std::string>::iterator cacheIter = _fullPathCache.find(filename);
	if (cacheIter != _fullPathCache.end()){
		return cacheIter->second;
	}
	/************************************************************************/
	/* Ê×ÒªÎªÉèÖÃµÄµÚÒ»ËÑË÷Ä¿Â¼ Ô­ÎÄ¼þ>¼ÓÃÜÎÄ¼þ
	/************************************************************************/

	std::string tmpPath;
	std::string codePath;
	/************************************************************************/
	/* 绝对路径下搜索
	/************************************************************************/
	if (isAbsolutePath(filename)){
        tmpPath = this->getPathForFilename(filename, "", "");
        if (tmpPath.length() > 0){
		//if (isFileExistInternal(filename)) {
			_fullPathCache.insert(std::make_pair(filename, tmpPath));
			return filename;
		}
		/************************************************************************/
		/* 是否转换成编码文件
		/************************************************************************/
		if (m_filePath2CodeCall){
			/************************************************************************/
			/* ÔÚÊ×ÒªÂ·¾¶ÏÂ,È·¶¨ÊÇ·ñ´æÔÚ¶ÔÓ¦md5
			/************************************************************************/
			if (!_firstSearchPath.empty() && replaceAll(filename, _firstSearchPath, "", tmpPath)){
				tmpPath = m_filePath2CodeCall(tmpPath);
				if (!tmpPath.empty()){
					tmpPath = _firstSearchPath + tmpPath;
                    tmpPath = this->getPathForFilename(tmpPath, "", "");
                    if (tmpPath.length() > 0){
					//if (isFileExistInternal(tmpPath)) {
						_fullPathCache.insert(std::make_pair(filename, tmpPath));
						return tmpPath;
					}
				}
			}
			/************************************************************************/
			/* ÔÚÆäËüÂ·¾¶ÏÂ,È·¶¨ÊÇ·ñ´æÔÚ¶ÔÓ¦md5
			/************************************************************************/
			std::vector<std::string>::const_iterator itr = m_absoluteSearchPathArray.begin();
			while (itr != m_absoluteSearchPathArray.end()){
				if (replaceAll(filename, *itr, "", tmpPath)){
					tmpPath = m_filePath2CodeCall(tmpPath);
					if (!tmpPath.empty()){
						tmpPath = *itr + tmpPath;
                        tmpPath = this->getPathForFilename(tmpPath, "", "");
                        if (tmpPath.length() > 0){
						//if (isFileExistInternal(tmpPath)) {
							_fullPathCache.insert(std::make_pair(filename, tmpPath));
							return tmpPath;
						}
					}
				}
				++itr;
			}
		}
		return "";
	}
	if (m_filePath2CodeCall){
		codePath = m_filePath2CodeCall(filename);		
	}
	/************************************************************************/
	/* ÔÚÊ×ÒªÂ·¾¶ÏÂ,È·¶¨ÊÇ·ñ´æÔÚ¶ÔÓ¦ÎÄ¼þ
	/************************************************************************/
	if (!_firstSearchPath.empty()){
		tmpPath = _firstSearchPath + filename;
        tmpPath = this->getPathForFilename(tmpPath, "", "");
        if (tmpPath.length() > 0){
		//if (isFileExistInternal(tmpPath)) {
			_fullPathCache.insert(std::make_pair(filename, tmpPath));
			return tmpPath;
		}
        if (!codePath.empty()){// && isFileExistInternal(tmpPath)){
            tmpPath = _firstSearchPath + codePath;
            tmpPath = this->getPathForFilename(tmpPath, "", "");
            if (tmpPath.length() > 0){
                _fullPathCache.insert(std::make_pair(filename, tmpPath));
                return tmpPath;
            }
		}
	}
	/************************************************************************/
	/* Ö±½ÓÔÚÏà¶ÔÂ·¾¶ÏÂ,³¢ÊÔÈ·¶¨ÊÇ·ñ´æÔÚ¶ÔÓ¦md5
	/************************************************************************/
	tmpPath = _defaultResRootPath + filename;
    tmpPath = this->getPathForFilename(tmpPath, "", "");
    if (tmpPath.length() > 0){
	//if (isFileExistInternal(tmpPath)) {
		_fullPathCache.insert(std::make_pair(filename, tmpPath));
		return tmpPath;
	}	
	if (!codePath.empty()){
		tmpPath = _defaultResRootPath + codePath;
        tmpPath = this->getPathForFilename(tmpPath, "", "");
        if (tmpPath.length() > 0){
		//if (isFileExistInternal(tmpPath)) {
			_fullPathCache.insert(std::make_pair(filename, tmpPath));
			return tmpPath;
		}
	}
	/************************************************************************/
	/* ÔÚÆäËüÂ·¾¶ÏÂ,È·¶¨ÊÇ·ñ´æÔÚ¶ÔÓ¦ÎÄ¼þ
	/************************************************************************/
	std::vector<std::string>::const_reverse_iterator itr = m_absoluteSearchPathArray.rbegin();
	while (itr != m_absoluteSearchPathArray.rend()){
		tmpPath = *itr + filename;
        tmpPath = this->getPathForFilename(tmpPath, "", "");
        if (tmpPath.length() > 0){
		//if (isFileExistInternal(tmpPath)) {
			_fullPathCache.insert(std::make_pair(filename, tmpPath));
			return tmpPath;
		}
		if (!codePath.empty()){
			tmpPath = *itr + codePath;
            tmpPath = this->getPathForFilename(tmpPath, "", "");
            if (tmpPath.length() > 0){
			//if (isFileExistInternal(tmpPath)) {
				_fullPathCache.insert(std::make_pair(filename, tmpPath));
				return tmpPath;
			}
		}
		++itr;
	}
	/************************************************************************/
	/* ÔÚÏà¶ÔÂ·¾¶ÏÂ´¦Àí
	/************************************************************************/
	for (const auto& searchIt : _searchPathArray){
		/************************************************************************/
		/* µÚÒ»Ê×Òª¸ùÄ¿Â¼
		/************************************************************************/
		if (!_firstSearchPath.empty()){
			tmpPath = _firstSearchPath + searchIt + filename;
            tmpPath = this->getPathForFilename(tmpPath, "", "");
            if (tmpPath.length() > 0){
			//if (isFileExistInternal(tmpPath)) {
				_fullPathCache.insert(std::make_pair(filename, tmpPath));
				return tmpPath;
			}
		}
		/************************************************************************/
		/* Ä¬ÈÏ¸ùÄ¿Â¼
		/************************************************************************/
		tmpPath = _defaultResRootPath + searchIt + filename;
        tmpPath = this->getPathForFilename(tmpPath, "", "");
        if (tmpPath.length() > 0){
		//if (isFileExistInternal(tmpPath)) {
			_fullPathCache.insert(std::make_pair(filename, tmpPath));
			return tmpPath;
		}

		if (m_filePath2CodeCall){
			codePath = m_filePath2CodeCall(searchIt + filename);			
		}
		if (!codePath.empty()){
			/************************************************************************/
			/* µÚÒ»Ê×Òª¸ùÄ¿Â¼
			/************************************************************************/
			if (!_firstSearchPath.empty()){
				tmpPath = _firstSearchPath + codePath;
                tmpPath = this->getPathForFilename(tmpPath, "", "");
                if (tmpPath.length() > 0){
				//if (isFileExistInternal(tmpPath)) {
					_fullPathCache.insert(std::make_pair(filename, tmpPath));
					return tmpPath;
				}
			}
			/************************************************************************/
			/* Ä¬ÈÏ¸ùÄ¿Â¼
			/************************************************************************/
			tmpPath = _defaultResRootPath + codePath;
            tmpPath = this->getPathForFilename(tmpPath, "", "");
            if (tmpPath.length() > 0){
			//if (isFileExistInternal(tmpPath)) {
				_fullPathCache.insert(std::make_pair(filename, tmpPath));
				return tmpPath;
			}
		}

		/************************************************************************/
		/* ÆäËüÏà¶Ô¸ùÄ¿Â¼
		/************************************************************************/
		itr = m_absoluteSearchPathArray.rbegin();
		while (itr != m_absoluteSearchPathArray.rend()){
			tmpPath = *itr + searchIt + filename;
            tmpPath = this->getPathForFilename(tmpPath, "", "");
            if (tmpPath.length() > 0){
			//if (isFileExistInternal(tmpPath)) {
				_fullPathCache.insert(std::make_pair(filename, tmpPath));
				return tmpPath;
			}
			if (!codePath.empty()){
				tmpPath = *itr + codePath;
                tmpPath = this->getPathForFilename(tmpPath, "", "");
                if (tmpPath.length() > 0){
				//if (isFileExistInternal(tmpPath)) {
					_fullPathCache.insert(std::make_pair(filename, tmpPath));
					return tmpPath;
				}
			}
			++itr;
		}
	}


	/*
    if (isAbsolutePath(filename))
    {
        return filename;
    }

    // Already Cached ?
	std::unordered_map<std::string, std::string>::iterator cacheIter = _fullPathCache.find(filename);
    if(cacheIter != _fullPathCache.end())
    {
        return cacheIter->second;
    }

    // Get the new file name.
	const std::string newFilename = (getNewFilename(filename));

    std::string fullpath;
	// add by James
	if (!_firstSearchPath.empty())
	{
		fullpath = this->getPathForFilename(newFilename, "", _firstSearchPath);
		if (!fullpath.empty())
		{
			// Using the filename passed in as key.
			_fullPathCache.insert(std::make_pair(filename, fullpath));
			return fullpath;
		}
	}

    for (const auto& searchIt : _searchPathArray)
    {
        for (const auto& resolutionIt : _searchResolutionsOrderArray)
        {
            fullpath = this->getPathForFilename(newFilename, resolutionIt, searchIt);

            if (!fullpath.empty())
            {
                // Using the filename passed in as key.
                _fullPathCache.insert(std::make_pair(filename, fullpath));
                return fullpath;
            }
        }
    }
	
	// add by James
	if (m_filePath2CodeCall){
		std::string tmpFilename = m_filePath2CodeCall(filename);
		if (!tmpFilename.empty()){
			std::string fullpath;
			if (!_firstSearchPath.empty()){
				fullpath = this->getPathForFilename(tmpFilename, "", _firstSearchPath);
				if (!fullpath.empty()){
					// Using the filename passed in as key.
					_fullPathCache.insert(std::make_pair(filename, filename));
					m_filecodeMap[filename] = fullpath;
					return fullpath;
				}
			}

			for (const auto& searchIt : _searchPathArray){
				for (const auto& resolutionIt : _searchResolutionsOrderArray){
					fullpath = this->getPathForFilename(tmpFilename, resolutionIt, searchIt);
					if (!fullpath.empty()){
						// Using the filename passed in as key.
						_fullPathCache.insert(std::make_pair(filename, filename));
						m_filecodeMap[filename] = fullpath;
						return fullpath;
					}
				}
			}
		}
	}
	*/
    if(isPopupNotify()){
        CCLOG("cocos2d: fullPathForFilename: No file found at %s. Possible missing file.", filename.c_str());
    }

    // The file wasn't found, return empty string.
    return "";
}

std::string FileUtils::fullPathFromRelativeFile(const std::string &filename, const std::string &relativeFile)
{
    return relativeFile.substr(0, relativeFile.rfind('/')+1) + getNewFilename(filename);
}

void FileUtils::setSearchResolutionsOrder(const std::vector<std::string>& searchResolutionsOrder)
{
    bool existDefault = false;
    _fullPathCache.clear();
	m_filecodeMap.clear();
    _searchResolutionsOrderArray.clear();
    for(const auto& iter : searchResolutionsOrder)
    {
        std::string resolutionDirectory = iter;
        if (!existDefault && resolutionDirectory == "")
        {
            existDefault = true;
        }

        if (resolutionDirectory.length() > 0 && resolutionDirectory[resolutionDirectory.length()-1] != '/')
        {
            resolutionDirectory += "/";
        }

        _searchResolutionsOrderArray.push_back(resolutionDirectory);
    }

    if (!existDefault)
    {
        _searchResolutionsOrderArray.push_back("");
    }
}

void FileUtils::addSearchResolutionsOrder(const std::string &order,const bool front)
{
    std::string resOrder = order;
    if (!resOrder.empty() && resOrder[resOrder.length()-1] != '/')
        resOrder.append("/");

    if (front) {
        _searchResolutionsOrderArray.insert(_searchResolutionsOrderArray.begin(), resOrder);
    } else {
        _searchResolutionsOrderArray.push_back(resOrder);
    }
}

const std::vector<std::string>& FileUtils::getSearchResolutionsOrder() const
{
    return _searchResolutionsOrderArray;
}

const std::vector<std::string>& FileUtils::getSearchPaths() const
{
    return _searchPathArray;
}

void FileUtils::setWritablePath(const std::string& writablePath)
{
    _writablePath = writablePath;
}

void FileUtils::setDefaultResourceRootPath(const std::string& path)
{
    _defaultResRootPath = path;
}

void FileUtils::setSearchPaths(const std::vector<std::string>& searchPaths)
{
    _fullPathCache.clear();
	m_filecodeMap.clear();
    _searchPathArray.clear();

	for (const auto& iter : searchPaths){
		addSearchPath(iter);
	}

	/*
	bool existDefaultRootPath = false;
    for (const auto& iter : searchPaths)
    {
        std::string prefix;
        std::string path;

        if (!isAbsolutePath(iter))
        { // Not an absolute path
            prefix = _defaultResRootPath;
        }
        path = prefix + (iter);
        if (!path.empty() && path[path.length()-1] != '/')
        {
            path += "/";
        }
        if (!existDefaultRootPath && path == _defaultResRootPath)
        {
            existDefaultRootPath = true;
        }
        _searchPathArray.push_back(path);
    }

    if (!existDefaultRootPath)
    {
        //CCLOG("Default root path doesn't exist, adding it.");
        _searchPathArray.push_back(_defaultResRootPath);
    }
	*/
}

void FileUtils::addSearchPath(const std::string &searchpath,const bool front)
{
	unsigned int len = searchpath.length();
	std::string tmpPath = searchpath;
	if (len > 0 && searchpath[len - 1] != '/'){
		tmpPath += '/';
	}
	if (isAbsolutePath(searchpath)){
		
		std::vector<std::string>::iterator absItr = m_absoluteSearchPathArray.begin();
		while (absItr != m_absoluteSearchPathArray.end()){
			if (*absItr == tmpPath){
				return;
			}
			absItr++;
		}
		m_absoluteSearchPathArray.push_back(tmpPath);
		return;
	}
	std::vector<std::string>::iterator seaItr = _searchPathArray.begin();
	while (seaItr != _searchPathArray.end()){
		if (*seaItr == tmpPath){
			return;
		}
		seaItr++;
	}
	_searchPathArray.push_back(tmpPath);
	
	
	/*
    std::string prefix;
    if (!isAbsolutePath(searchpath))
        prefix = _defaultResRootPath;

    std::string path = prefix + searchpath;
    if (!path.empty() && path[path.length()-1] != '/')
    {
        path += "/";
    }
    if (front) {
        _searchPathArray.insert(_searchPathArray.begin(), path);
    } else {
        _searchPathArray.push_back(path);
    }
	*/
}

void FileUtils::setFilenameLookupDictionary(const ValueMap& filenameLookupDict)
{
    _fullPathCache.clear();
	m_filecodeMap.clear();
    _filenameLookupDict = filenameLookupDict;
}

void FileUtils::loadFilenameLookupDictionaryFromFile(const std::string &filename)
{
    const std::string fullPath = fullPathForFilename(filename);
    if (!fullPath.empty())
    {
        ValueMap dict = FileUtils::getInstance()->getValueMapFromFile(fullPath);
        if (!dict.empty())
        {
            ValueMap& metadata =  dict["metadata"].asValueMap();
            int version = metadata["version"].asInt();
            if (version != 1)
            {
                CCLOG("cocos2d: ERROR: Invalid filenameLookup dictionary version: %d. Filename: %s", version, filename.c_str());
                return;
            }
            setFilenameLookupDictionary( dict["filenames"].asValueMap());
        }
    }
}

std::string FileUtils::getFullPathForDirectoryAndFilename(const std::string& directory, const std::string& filename) const
{
    // get directory+filename, safely adding '/' as necessary
    std::string ret = directory;
	unsigned int len = directory.size();
	if (len>0 && directory[len - 1] != '/'){
        ret += '/';
    }
    ret += filename;

    // if the file doesn't exist, return an empty string
    if (!isFileExistInternal(ret)) {
        return "";
    }
    return ret;
}

bool FileUtils::isFileExist(const std::string& filename) const
{
	std::string fullpath = fullPathForFilename(filename);
	if (fullpath.empty())
		return false;
	else
		return true;

	/*
    if (isAbsolutePath(filename))
    {
        return isFileExistInternal(filename);
    }
    else
    {
        std::string fullpath = fullPathForFilename(filename);
        if (fullpath.empty())
            return false;
        else
            return true;
    }
	*/
}

bool FileUtils::isAbsolutePath(const std::string& path) const
{
    return (path[0] == '/');
}

bool FileUtils::isDirectoryExist(const std::string& dirPath) const
{
    CCASSERT(!dirPath.empty(), "Invalid path");

    if (isAbsolutePath(dirPath))
    {
        return isDirectoryExistInternal(dirPath);
    }

    // Already Cached ?
    auto cacheIter = _fullPathCache.find(dirPath);
    if( cacheIter != _fullPathCache.end() )
    {
        return isDirectoryExistInternal(cacheIter->second);
    }

    std::string fullpath;
    for (const auto& searchIt : _searchPathArray)
    {
        for (const auto& resolutionIt : _searchResolutionsOrderArray)
        {
            // searchPath + file_path + resourceDirectory
            fullpath = searchIt + dirPath + resolutionIt;
            if (isDirectoryExistInternal(fullpath))
            {
                _fullPathCache.insert(std::make_pair(dirPath, fullpath));
                return true;
            }
        }
    }
    return false;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
// windows os implement should override in platform specific FileUtiles class
bool FileUtils::isDirectoryExistInternal(const std::string& dirPath) const
{
    CCASSERT(false, "FileUtils not support isDirectoryExistInternal");
    return false;
}

bool FileUtils::createDirectory(const std::string& path)
{
    CCASSERT(false, "FileUtils not support createDirectory");
    return false;
}

bool FileUtils::removeDirectory(const std::string& path)
{
    CCASSERT(false, "FileUtils not support removeDirectory");
    return false;
}

bool FileUtils::removeFile(const std::string &path)
{
    CCASSERT(false, "FileUtils not support removeFile");
    return false;
}

bool FileUtils::renameFile(const std::string &oldfullpath, const std::string& newfullpath)
{
    CCASSERT(false, "FileUtils not support renameFile");
    return false;
}

bool FileUtils::renameFile(const std::string &path, const std::string &oldname, const std::string &name)
{
    CCASSERT(false, "FileUtils not support renameFile");
    return false;
}

std::string FileUtils::getSuitableFOpen(const std::string& filenameUtf8) const
{
    CCASSERT(false, "getSuitableFOpen should be override by platform FileUtils");
    return filenameUtf8;
}

long FileUtils::getFileSize(const std::string &filepath)
{
    CCASSERT(false, "getFileSize should be override by platform FileUtils");
    return 0;
}

#else
// default implements for unix like os
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

bool FileUtils::isDirectoryExistInternal(const std::string& dirPath) const
{
    struct stat st;
    if (stat(dirPath.c_str(), &st) == 0)
    {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

bool FileUtils::createDirectory(const std::string& path)
{
    CCASSERT(!path.empty(), "Invalid path");

    if (isDirectoryExist(path))
        return true;

    // Split the path
    size_t start = 0;
    size_t found = path.find_first_of("/\\", start);
    std::string subpath;
    std::vector<std::string> dirs;

    if (found != std::string::npos)
    {
        while (true)
        {
            subpath = path.substr(start, found - start + 1);
            if (!subpath.empty())
                dirs.push_back(subpath);
            start = found+1;
            found = path.find_first_of("/\\", start);
            if (found == std::string::npos)
            {
                if (start < path.length())
                {
                    dirs.push_back(path.substr(start));
                }
                break;
            }
        }
    }

    DIR *dir = NULL;

    // Create path recursively
    subpath = "";
    for (int i = 0; i < dirs.size(); ++i)
    {
        subpath += dirs[i];
        dir = opendir(subpath.c_str());

        if (!dir)
        {
            // directory doesn't exist, should create a new one

            int ret = mkdir(subpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            if (ret != 0 && (errno != EEXIST))
            {
                // current directory can not be created, sub directories can not be created too
                // should return
                return false;
            }
        }
        else
        {
            // directory exists, should close opened dir
            closedir(dir);
        }
    }
    return true;
}

bool FileUtils::removeDirectory(const std::string& path)
{
    // FIXME: Why using subclassing? an interface probably will be better
    // to support different OS
    // FileUtils::removeDirectory is subclassed on iOS/tvOS
    // and system() is not available on tvOS
#if !defined(CC_PLATFORM_IOS)
    if (path.size() > 0 && path[path.size() - 1] != '/')
    {
        CCLOGERROR("Fail to remove directory, path must terminate with '/': %s", path.c_str());
        return false;
    }

    std::string command = "rm -r ";
    // Path may include space.
    command += "\"" + path + "\"";
    if (system(command.c_str()) >= 0)
        return true;
    else
#endif
        return false;
}

bool FileUtils::removeFile(const std::string &path)
{
    if (remove(path.c_str())) {
        return false;
    } else {
        return true;
    }
}

bool FileUtils::renameFile(const std::string &oldfullpath, const std::string &newfullpath)
{
    CCASSERT(!oldfullpath.empty(), "Invalid path");
    CCASSERT(!newfullpath.empty(), "Invalid path");

    int errorCode = rename(oldfullpath.c_str(), newfullpath.c_str());

    if (0 != errorCode)
    {
        CCLOGERROR("Fail to rename file %s to %s !Error code is %d", oldfullpath.c_str(), newfullpath.c_str(), errorCode);
        return false;
    }
    return true;
}

bool FileUtils::renameFile(const std::string &path, const std::string &oldname, const std::string &name)
{
    CCASSERT(!path.empty(), "Invalid path");
    std::string oldPath = path + oldname;
    std::string newPath = path + name;

    return this->renameFile(oldPath, newPath);
}

std::string FileUtils::getSuitableFOpen(const std::string& filenameUtf8) const
{
    return filenameUtf8;
}


long FileUtils::getFileSize(const std::string &filepath)
{
    CCASSERT(!filepath.empty(), "Invalid path");

    std::string fullpath = filepath;
    if (!isAbsolutePath(filepath))
    {
        fullpath = fullPathForFilename(filepath);
        if (fullpath.empty())
            return 0;
    }

    struct stat info;
    // Get data associated with "crt_stat.c":
    int result = stat(fullpath.c_str(), &info);

    // Check if statistics are valid:
    if (result != 0)
    {
        // Failed
        return -1;
    }
    else
    {
        return (long)(info.st_size);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
// Notification support when getFileData from invalid file path.
//////////////////////////////////////////////////////////////////////////
static bool s_popupNotify = true;

void FileUtils::setPopupNotify(bool notify)
{
    s_popupNotify = notify;
}

bool FileUtils::isPopupNotify() const
{
    return s_popupNotify;
}

std::string FileUtils::getFileExtension(const std::string& filePath) const
{
    std::string fileExtension;
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos)
    {
        fileExtension = filePath.substr(pos, filePath.length());

        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
    }

    return fileExtension;
}

std::string FileUtils::getFilecodeRet(const std::string& filename)
{
	FILE_CODE_MAP::iterator itr = m_filecodeMap.find(filename);
	if (itr != m_filecodeMap.end()){
		return itr->second;
	}

	itr = _fullPathCache.find(filename);
	if (itr != _fullPathCache.end()){
		return itr->second;
	}

	if (isAbsolutePath(filename)){
		if (isFileExistInternal(filename)){
			return filename;
		}

		// add by James
		if (m_filePath2CodeCall){
			std::string tmpFilename = m_filePath2CodeCall(filename);
			if (!tmpFilename.empty()){
				std::string fullpath;
				if (!_firstSearchPath.empty()){
					fullpath = this->getPathForFilename(tmpFilename, "", _firstSearchPath);
					if (!fullpath.empty()){
						// Using the filename passed in as key.
						_fullPathCache.insert(std::make_pair(filename, filename));
						m_filecodeMap[filename] = fullpath;
						return fullpath;
					}
				}

				for (const auto& searchIt : _searchPathArray){
					for (const auto& resolutionIt : _searchResolutionsOrderArray){
						fullpath = this->getPathForFilename(filename, resolutionIt, searchIt);
						if (!fullpath.empty()){
							// Using the filename passed in as key.
							_fullPathCache.insert(std::make_pair(filename, filename));
							m_filecodeMap[filename] = fullpath;
							return fullpath;
						}
					}
				}
			}
		}
	}

	return "";
}

void FileUtils::setFilepath2CodeCall(const std::function<std::string(const std::string&)>& call)
{
	m_filePath2CodeCall = call;
}

void FileUtils::setFirstSearchPath(std::string& path)
{
	unsigned int len = path.length();	
	if (len > 0){
		if ( path[len - 1] != '/')
			_firstSearchPath = path + '/';
		else
			_firstSearchPath = path;
	}
}

NS_CC_END

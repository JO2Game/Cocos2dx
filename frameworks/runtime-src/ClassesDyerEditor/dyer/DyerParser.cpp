#include "DyerParser.h"

#define STRINGIFY(A)  #A

#include "shader/shaderDyer_vsh.h"
#include "shader/shaderDyer_fsh.h"
#include "shader/shaderRestore_fsh.h"


void _splitFilename(const std::string& qualifiedName, std::string& outBasename, std::string& outPath)
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

unsigned char _readUnsignedChar(unsigned char* buff)
{
	if (buff)
	{
		unsigned char byte = *buff;
		return byte;
	}
	return 0;
}

void _writeUnsignedChar(unsigned char val, unsigned char* buff)
{
	if (buff)
	{
		memcpy(buff, &val, 1);
	}
}

float _readFloat(unsigned char* buff)
{
	if (buff)
	{		
		float f = 0.0f;
		::memcpy(&f, buff, sizeof(float));
		return f;
	}
	return 0;	
}
void _writeFloat(float val, unsigned char* buff)
{
	if (buff)
	{
		memcpy(buff, &val, sizeof(float));
	}	
}

//////////////////////////////////////////////////////////////////////////

DyerData::DyerData(char lenght, float* h, float* s, float* l) :len(lenght), harr(h), sarr(s), larr(l)
{
}

DyerData::DyerData(char lenght) : len(lenght)
{
	harr = new float[len]();
	sarr = new float[len]();
	larr = new float[len]();
}

DyerData::~DyerData()
{
	if (harr)
	{
		delete harr;
		harr = nullptr;
	}
	if (sarr)
	{
		delete sarr;
		sarr = nullptr;
	}
	if (larr)
	{
		delete larr;
		larr = nullptr;
	}
}
//////////////////////////////////////////////////////////////////////////
void DyerParser::saveToFile(const std::string& path, DyerCaseVec& vec)
{

	unsigned char size = vec.size();
	char len = 0;
	if (size > 0)
	{
		DyerData* dd = vec.front();
		len = dd->len;
	}
	else
	{
		return;
	}
	unsigned int total = len *sizeof(float) * 3 * size + 2;
	unsigned char* buff = new unsigned char[total];
	unsigned int buffseek = 0;	
	//memcpy(buff + buffseek, (char*)size, 1);	
	buff[buffseek] = size;
	buffseek += 1;

	//memcpy(buff + buffseek, (char*)len, 1);
	buff[buffseek] = len;
	buffseek += 1;

	int offset = len*sizeof(float);
	for (unsigned char i = 0; i < size; i++)
	{
		DyerData* dd = vec[i];		
		memcpy(buff + buffseek, dd->harr, offset);
		buffseek += offset;

		memcpy(buff + buffseek, dd->sarr, offset);
		buffseek += offset;

		memcpy(buff + buffseek, dd->larr, offset);
		buffseek += offset;
	}

	FILE* fp = fopen(path.c_str(), "wb");
	if (fp)
	{
		fwrite(buff, buffseek, 1, fp);
		fclose(fp);		
	}
	delete [] buff;
}

bool DyerParser::getDataFromFile(const std::string& path, DyerCaseVec& vec)
{
	if (!FileUtils::getInstance()->isFileExist(path))
	{
		return false;
	}
	Data d = FileUtils::getInstance()->getDataFromFile(path);
	if (d.isNull() || d.getSize()<3)
	{
		return false;
	}
	unsigned char* buff = d.getBytes();
	unsigned char caseCount = _readUnsignedChar(buff);
	buff += 1;
	vec.resize(caseCount);
	unsigned char len = _readUnsignedChar(buff);
	buff += 1;
	if (caseCount < 1 || len < 1)
	{
		return false;
	}
	int offset = len*sizeof(float);
	for (unsigned char i = 0; i < caseCount; i++)
	{
		
		float *h = new float[len];
		float *s = new float[len];
		float *l = new float[len];

		memcpy(h, buff, offset);
		buff += offset;
		memcpy(s, buff, offset);
		buff += offset;
		memcpy(l, buff, offset);
		buff += offset;

		DyerData *dd = new DyerData(len, h, s, l);
		vec[i] = dd;
	}
	return true;
}

bool DyerParser::dyer(cocos2d::Sprite* sprite, std::string srcPath, unsigned char solutionId)
{
	std::string fname;
	std::string fdir;
	_splitFilename(srcPath, fname, fdir);
	std::string solutionPath = fdir + "solution.armi";
	std::vector<DyerData*> vec;	
	DYER_SOLUTION_MAP::iterator itr = dyer_solution_map.find(solutionPath);
	if (itr == dyer_solution_map.end())
	{		
		if (!getDataFromFile(solutionPath, vec))
		{
			return false;
		}
		dyer_solution_map[solutionPath] = vec;
	}
	else
	{
		vec = itr->second;
	}
	

	if (solutionId < vec.size())
	{
		std::string blockPath = fdir + fname + ".block";
		Texture2D* blockTexture = Director::getInstance()->getTextureCache()->getTextureForKey(blockPath);
		if (!blockTexture)
		{
			blockTexture = Director::getInstance()->getTextureCache()->addImage(blockPath);
			if (!blockTexture)
			{
				return false;
			}
		}
		cocos2d::GLProgram* program = cocos2d::GLProgram::createWithByteArrays(cc_dyer_vsh, cc_dyer_fsh);
		cocos2d::GLProgramState* pState = GLProgramState::getOrCreateWithGLProgram(program);
		sprite->setGLProgramState(pState);


		pState->setUniformTexture("u_dTex", blockTexture);

		GLint hs = program->getUniformLocationForName("u_dHs");
		GLint ss = program->getUniformLocationForName("u_dSs");
		GLint ls = program->getUniformLocationForName("u_dLs");

		DyerData* dd = vec[solutionId];
		program->setUniformLocationWith1fv(hs, dd->harr, dd->len);
		program->setUniformLocationWith1fv(ss, dd->sarr, dd->len);
		program->setUniformLocationWith1fv(ls, dd->larr, dd->len);
		return true;
	}
	return false;
}

void DyerParser::restore(cocos2d::Sprite* sprite)
{	
	cocos2d::GLProgram* program = cocos2d::GLProgram::createWithByteArrays(cc_dyer_vsh, cc_dyer_restore_fsh);
	cocos2d::GLProgramState* pState = GLProgramState::getOrCreateWithGLProgram(program);
	sprite->setGLProgramState(pState);
}


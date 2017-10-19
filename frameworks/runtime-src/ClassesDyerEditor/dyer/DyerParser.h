#ifndef __DYER_PARSER_H__
#define __DYER_PARSER_H__

#include <string>
#include <vector>
#include <unordered_map>
//#include "jo_utils/JOSingleton.h"

#include "cocos2d.h"
USING_NS_CC;

extern CC_DLL const GLchar * cc_dyer_vsh;
extern CC_DLL const GLchar * cc_dyer_fsh;
extern CC_DLL const GLchar * cc_dyer_restore_fsh;

namespace cocos2d
{
	class Sprite;
}

struct DyerData
{
	DyerData(char lenght);
	DyerData(char lenght, float* h, float* s, float* l);
	~DyerData();

	//三个数组必须同等长度, 长度少于char即127
	char len;
	
	float *harr;
	float *sarr;
	float *larr;
};

typedef std::vector<DyerData*> DyerCaseVec;

class DyerParser //: public JOSingleton<DyerParser>
{
public:
	DyerParser(){};
	virtual ~DyerParser(){};
public:
	static void saveToFile(const std::string& path, DyerCaseVec& vec);
	static bool getDataFromFile(const std::string& path, DyerCaseVec& vec);

	bool dyer(cocos2d::Sprite* sprite, std::string srcPath, unsigned char solutionId);
	void restore(cocos2d::Sprite* sprite);

private:

private:
	typedef std::unordered_map< std::string, std::vector<DyerData*> > DYER_SOLUTION_MAP;
	DYER_SOLUTION_MAP dyer_solution_map;
};
	

#endif //__DYER_PARSER_H__
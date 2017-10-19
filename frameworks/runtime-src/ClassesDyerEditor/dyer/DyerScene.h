#ifndef __DYER_SCENE_H__
#define __DYER_SCENE_H__

#include "cocos2d.h"
USING_NS_CC;
namespace DyerEditor
{
	class DyerScene : public Scene
	{
	public:
		// implement the "static create()" method manually
		CREATE_FUNC(DyerScene);
		// Here's a difference. Method 'init' in cocos2d-x returns bool, instead of returning 'id' in cocos2d-iphone
		virtual bool init();		
	};
}

#endif // __DYER_SCENE_H__

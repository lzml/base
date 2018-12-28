#pragma once
#include <functional>
/*************************************************************************
* 文 件 名：adsCxx11Tool.h

* 描    述：c++11新特性

* 备    注：
*************************************************************************/

/******************************资源清理***********************************/
// 使用说明
/* 
Acquire Resource1
ON_SCOPE_EXIT([&] { Release Resource1 })

Acquire Resource2
ON_SCOPE_EXIT([&] { Release Resource2 })
…
*/

class ScopeGuard
{
public:
	explicit ScopeGuard(std::function<void()> onExitScope)
		: onExitScope_(onExitScope), dismissed_(false)
	{ }

	~ScopeGuard()
	{
		if (!dismissed_)
		{
			onExitScope_();
		}
	}

	void Dismiss()
	{
		dismissed_ = true;
	}

private:
	std::function<void()> onExitScope_;
	bool dismissed_;

private: // noncopyable
	ScopeGuard(ScopeGuard const&);
	ScopeGuard& operator=(ScopeGuard const&);
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback) ScopeGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)(callback)
/************************************************************************/
#ifndef AD_GLOBAL_H_
#define AD_GLOBAL_H_

#include <stdint.h>

namespace base {

	extern bool g_bAddfolder;

}  // namespace base

#define MAX_PRE_BOOKMARKS_NUM	20
#define C_ADSE_BOOKMARKER L"00020136"
#define Pre_cdn_bookmarks_dir "\\ADSAFESE\\bkicon"
#define Pre_cdn_bookmarks_Adsafese "\\ADSafeSe\\bkicon\\ADSafeSe.png"
#define Pre_cdn_bookmarks_stamp "\\ADSafeSe\\User Data\\Default\\cdnBookmarks.ini"

typedef struct PreBookmarksData
{
	int nSum;
	WCHAR* pwzTitle[MAX_PRE_BOOKMARKS_NUM];
	WCHAR* pwzUrl[MAX_PRE_BOOKMARKS_NUM];
};

extern PreBookmarksData* g_pPreBookmarksData;


#endif  // AD_GLOBAL_H_

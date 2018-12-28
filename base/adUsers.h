#ifndef AD_USERS_H_
#define AD_USERS_H_

#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <io.h>

// bookmarks
#include "components\keyed_service\core\keyed_service_export.h"
#include "components\keyed_service\core\keyed_service_factory.h"
#include "chrome\browser\ui\views\bookmarks\bookmark_bar_view.h"
#include "chrome\browser\ui\views\frame\browser_view.h"
#include "chrome\browser\ui\browser.h"
#include "chrome\browser\bookmarks\bookmark_model_factory.h"
#include "chrome\browser\ui\browser_finder.h"
#include "base\md5.h"
#include "chrome/browser/ui/browser_list.h"


// bookmarks sync
#include "chrome/browser/ui/browser_commands.h"
#include "components/bookmarks/browser/bookmark_model.h"

// passwords
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/password_manager/password_store_win.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"
#include "components/password_manager/core/browser/login_database.h"

// extensions
#include "extensions/browser/extension_prefs_factory.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model_factory.h"
#include "extensions/browser/extension_registry_factory.h"
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions\browser\extension_pref_value_map_factory.h"

// songqiqi 20161117 切换界面
#include "chrome\browser\ui\views\toolbar\toolbar_view.h"
#include "chrome\browser\ui\views\toolbar\browser_actions_container.h"
#include "chrome\browser\ui\views\frame\left_bookmark_view.h"

#include "components\history\core\browser\history_service.h"
#include "chrome\browser\history\history_service_factory.h"
#include "components\history\core\browser\history_client.h"
#include "chrome\browser\history\chrome_history_client.h"

#include "adGlobalData.h"
#include "base\scope_guard.h"

#include "components\bookmarks\browser\bookmark_model_observer.h"

/*songqiqi 20161102 export api*/
bool BookmarksLoadDone();	//上一次书签是否加载成功
void AlterBookmarks(bool bLogin, const char* pcszUserIdMd5);// 切换书签
void AlterPasswords(bool bLogin, const char* pcszUserIdMd5);// 切换密码
// 书签MD5-128值 小写字母 失败时返回 return std::string("");
std::string CalOrgBKsDigest(const char* pcszUserIdMd5);
/*Done */

/*songqiqi 20161102 internel api*/
std::string CalOrgBKsDigestWithPath(char* szFilePath);
BOOL CreateAccountDir(const char* pcszUserIdMd5);
/*Done*/

bool BookmarksLoadDone()
{
	Browser* browser_Active = chrome::FindLastActive();
	Profile* profile = browser_Active->profile();

	bookmarks::BookmarkModel* org_BookmarkModel = BookmarkModelFactory::GetForProfile(profile);
	return org_BookmarkModel->loaded();
}

void AlterPasswords(bool bLogin, const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);

	if (!CreateAccountDir(pcszUserIdMd5))
	{
		base::ThreadRestrictions::SetIOAllowed(false);
		return;
	}

	Browser* browser_ = chrome::FindLastActive();
	Profile* profile = browser_->profile();

	PasswordStoreFactory* pwdStoreFactory = PasswordStoreFactory::GetInstance();
	base::SupportsUserData * context = pwdStoreFactory->GetContextToUsePublic((content::BrowserContext *)profile);
	scoped_refptr<RefcountedKeyedService> service;

	if (!context)
	{
		return;
	}
	
	if (bLogin)
	{
		WCHAR wzUserIdMd5[40];
		ZeroMemory(wzUserIdMd5, 80);
		MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);
		service = pwdStoreFactory->BuildServiceInstanceForWithUser(static_cast<content::BrowserContext*>(context), wzUserIdMd5);
	}
	else
	{
		service = pwdStoreFactory->BuildServiceInstanceForPublic(static_cast<content::BrowserContext*>(context));
	}

	PasswordStoreWin* ps = static_cast<PasswordStoreWin*>(
		pwdStoreFactory->GetPasswordStoreService(context).get());

	pwdStoreFactory->DisassociateAndChange(context, service);

	if (NULL != ps)
	{
		ps->Setlogin_dbNull();
	}
	base::ThreadRestrictions::SetIOAllowed(false);
}

void AlterBookmarks(bool bLogin, const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);
	ON_SCOPE_EXIT([] {	base::ThreadRestrictions::SetIOAllowed(false); });

	if (!CreateAccountDir(pcszUserIdMd5))
	{
		return;
	}

	BookmarkModelFactory* bookmarkModelFactory = BookmarkModelFactory::GetInstance();
	KeyedServiceFactory* keyedServiceFactory = (KeyedServiceFactory*)(bookmarkModelFactory);

	Browser* browser_Active = chrome::FindLastActive();
	if (!browser_Active)
	{
		return;
	}

	//获取老的model
	bookmarks::BookmarkModel* bookmark_model_old = BookmarkModelFactory::GetForProfile(browser_Active->profile());


	// static
	Profile* profile = browser_Active->profile();

	bool incognito = profile->IsOffTheRecord();
	
	if (incognito)
	{

		return;
	}

	WCHAR wzUserIdMd5[40];
	ZeroMemory(wzUserIdMd5, 80);
	MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);

	// generate new model_
	KeyedService* keyedService = bookmarkModelFactory->BuildServiceInstanceForPublic((content::BrowserContext*)profile, bLogin, wzUserIdMd5);

	BrowserList* browser_list_impl = BrowserList::GetInstance();
	if (NULL == browser_list_impl)
	{
		return;
	}

	// change mapping_ 
	keyedServiceFactory->DisassociateAndChange((base::SupportsUserData*)profile, keyedService);

	for (size_t index = 0; index < chrome::GetTotalBrowserCount(); index++)
	{
		Browser* browser_ = browser_list_impl->get(index);

		bookmarks::BookmarkModel* bookmark_model_new = BookmarkModelFactory::GetForProfile(browser_->profile());

		// static
		BrowserView* browserview_ = BrowserView::GetBrowserViewForBrowser(browser_);
		BookmarkBarView* bookmark_bar_view_ = browserview_->GetBookmarkBarView();

		// clear view buttons
		bookmark_bar_view_->ClearButtonsView();

		// ChangeModel
		//bookmark_bar_view_->ChangeModel();

		ContentsContainerView*  contentsView = browserview_->GetBookmarkAndDevToolsView();
		 LeftBookmarkView* leftView = contentsView->GetLeftBookmarkView();
		 leftView->ClearView();
		// leftView->ChangeModel();
		base::ObserverList<bookmarks::BookmarkModelObserver>* modelObserverList = bookmark_model_old->GetObserverList();
	//	base::FOR_EACH_OBSERVER(bookmarks::BookmarkModelObserver, *modelObserverList, ChangeModel(bookmark_model_new));
		do 
		{
			if ((modelObserverList)->might_have_observers())
			{
				base::ObserverListBase<bookmarks::BookmarkModelObserver>::Iterator it_inside_observer_macro(modelObserverList);
				bookmarks::BookmarkModelObserver* obs;
				while ((obs = it_inside_observer_macro.GetNext()) != nullptr)
				{
					obs->ChangeModel(bookmark_model_new);
				}
			}
		} while (0);
		 
		


	}

	//删除老的model
	//delete bookmark_model_old;
	
	// 20161124 由于bookmark_model改变 导致点击图标时接收不到信号 不会刷新view
	//history::HistoryService* historyService = HistoryServiceFactory::GetForProfileWithoutCreating(profile);
	//bookmarks::BookmarkModel* model_new = BookmarkModelFactory::GetForProfile(profile);
	//history::HistoryClient* historyClient = NULL;

	//if (historyService && model_new)
	//{
	//	historyService->ExchangeHistoryClient(new ChromeHistoryClient(model_new));

	//	historyClient = historyService->GetHistoryClient();

	//	if (historyClient)
	//	{
	//		historyClient->OnHistoryServiceCreated(historyService);
	//	}
	//}
}

std::string CalOrgBKsDigestWithPath(char* szFilePath)
{
	DWORD dwError = 0;
	HANDLE hFile = CreateFileA(szFilePath,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwError = GetLastError();
		return std::string("");
	}

	DWORD dwFileSize = GetFileSize(hFile, 0);

	DWORD dwReadBytes = 0;
	char *text_content = (char*)malloc(dwFileSize+10);
	if ( NULL == text_content)
	{
		dwError = GetLastError();
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		return std::string("");
	}

	ZeroMemory(text_content, dwFileSize + 10);

	BOOL bReadResult = FALSE;
	bReadResult = ReadFile(hFile, text_content, dwFileSize, &dwReadBytes, NULL);
	if ( !bReadResult || dwReadBytes != dwFileSize )
	{
		dwError = GetLastError();
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		free(text_content);
		text_content = NULL;

		return std::string("");
	}

	base::MD5Context md5;
	base::MD5Digest digest;
	base::MD5Init(&md5);
	base::StringPiece data(text_content, dwFileSize);

	base::MD5Update(&md5, data);// (const unsigned char *)text_section, text_size);
	base::MD5Final(&digest, &md5);

	//善后工作
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
	free(text_content);
	text_content = NULL;

	return base::MD5DigestToBase16(digest);
}

std::string CalOrgBKsDigest(const char* pcszUserIdMd5)
{
	char szLBookmarks[MAX_PATH];
	ZeroMemory(szLBookmarks, MAX_PATH);
	char *pszTailPath = "\\ADSafeSe\\User Data\\Default\\";
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szLBookmarks);
	strcat(szLBookmarks, pszTailPath);
	strcat(szLBookmarks, pcszUserIdMd5);
	strcat(szLBookmarks, "\\Bookmarks");

	if (-1 == (_access(szLBookmarks, 0)))
	{
		return std::string("");
	}
	else
	{
		return CalOrgBKsDigestWithPath(szLBookmarks);
	}
}

BOOL CreateAccountDir(const char* pcszUserIdMd5)
{
	if (NULL == pcszUserIdMd5)
	{
		return FALSE;
	}

	char szAccountDir[MAX_PATH];
	ZeroMemory(szAccountDir, MAX_PATH);
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szAccountDir);
	strcat(szAccountDir, "\\ADSafeSe\\User Data\\Default\\");
	strcat(szAccountDir, pcszUserIdMd5);

	if (!PathIsDirectoryA(szAccountDir))
	{
		return CreateDirectoryA(szAccountDir, NULL);
	}

	return TRUE;
}

#endif  // AD_USERS_H_

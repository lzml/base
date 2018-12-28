#ifndef AD_EXTENSIONS_H_
#define AD_EXTENSIONS_H_

#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <io.h>

#include "components\keyed_service\core\keyed_service_export.h"
#include "components\keyed_service\core\keyed_service_factory.h"
#include "chrome\browser\ui\views\frame\browser_view.h"
#include "chrome\browser\ui\browser.h"
#include "chrome\browser\ui\browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_commands.h"
#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"

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
#include "chrome\browser\ui\browser.h"
#include "chrome\browser\browser_process.h"

// songqiqi 20161122 关闭其余browsers
#include "chrome\browser\ui\views\frame\browser_shutdown.h"
//#include "ui\views\widget\desktop_aura\desktop_native_widget_aura.h"

#include "adGlobalData.h"

void NewBrowserWindow(bool bLogin, const char* pcszUserIdMd5);

void AlterExtensions(bool bLogin, const char* pcszUserIdMd5);// 切换插件
void AlterExtensions_Just_Dir(bool bLogin, const char* pcszUserIdMd5);// 切换插件—仅仅切换安装目录 
BOOL CreateAccountDir2(const char* pcszUserIdMd5);

void NewBrowserWindow(bool bLogin, const char* pcszUserIdMd5)
{
	/*
	需要修复:
		1 ※※※ 切换用户时， 关闭先前的browser  over
		2 ※※	 由于是最近一次打开的配置文件 所以如果没有退出登录 就会直接读取登录时候的目录 over
	*/

	base::ThreadRestrictions::SetIOAllowed(true);

	Profile* profile = NULL;
	ProfileManager* profile_manager = NULL;
	Browser* new_browser = NULL;
	base::FilePath newProfile_dir;

	do {

		profile_manager = g_browser_process->profile_manager();
		const base::FilePath user_data_dir = profile_manager->user_data_dir();

		if (bLogin)
		{
			newProfile_dir = user_data_dir.AppendASCII(pcszUserIdMd5);
		}
		else
		{
			newProfile_dir = user_data_dir.AppendASCII("Default");
		}

		profile = profile_manager->GetProfile(newProfile_dir);
		if (NULL == profile)
		{
			break;
		}

		new_browser = new Browser(Browser::CreateParams(Browser::TYPE_TABBED, profile));
		if (NULL == new_browser)
		{
			break;
		}

		if (bLogin)
		{
			chrome::AddTabAt(new_browser, GURL("http://www.ad-safe.com/"), -1, true);
		}
		else
		{
			chrome::AddTabAt(new_browser, GURL("http://nba.hupu.com/"), -1, true);
		}
	
		new_browser->window()->Show();

	} while (false);

	BrowserList* browser_list_impl = NULL;
	size_t nBrowserCount = chrome::GetTotalBrowserCount();

	do
	{
		if (1 == nBrowserCount)
		{
			break;
		}

		browser_list_impl = BrowserList::GetInstance();
		if (NULL == browser_list_impl)
		{
			break;
		}

		for (size_t index = 0; index < nBrowserCount; index++)
		{
			Browser* browser_ = browser_list_impl->get(index);
			Profile* profile_old = browser_->profile();
			if (profile != profile_old)
			{
				DestroyBrowserWebContents(browser_);
				//aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(), nullptr);
				//DesktopNativeWidgetAura::OnHostClosed();
			}
		}

	} while (false);

	


	base::ThreadRestrictions::SetIOAllowed(false);
}

void AlterExtensions(bool bLogin, const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);

	Browser* browser_Active = NULL;
	Profile* profile = NULL;
	content::BrowserContext* browserContext = NULL;

	WCHAR wzUserIdMd5[40];
	ZeroMemory(wzUserIdMd5, 80);
	MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);

	WCHAR wzUserExtensionsPath[MAX_PATH];
	ZeroMemory(wzUserExtensionsPath, 2 * MAX_PATH);
	WCHAR *pwzTailPath1 = L"\\ADSafeSe\\User Data\\Default\\";
	WCHAR *pwzTailPath2 = L"Extensions";
	SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wzUserExtensionsPath);
	wcscat(wzUserExtensionsPath, pwzTailPath1);

	if (bLogin)
	{
		wcscat(wzUserExtensionsPath, wzUserIdMd5);
		wcscat(wzUserExtensionsPath, L"\\");
	}

	wcscat(wzUserExtensionsPath, pwzTailPath2);

	ExtensionPrefValueMapFactory* extensionPrefValueMapFactory = NULL;
	extensions::ExtensionPrefsFactory* extensionPrefsFactory = NULL;
	
	extensions::ExtensionRegistryFactory* extensionRegistryFactory = NULL;
	ToolbarActionsModelFactory* toolbarActionsModelFactory = NULL;

	KeyedService* KeyedService_ToolbarActionsModel = NULL;
	KeyedService* KeyedService_ExtensionPrefs = NULL;
	KeyedService* KeyedService_ExtensionRegistryFactory = NULL;
	KeyedService* KeyedService_ExtensionPrefValueMapFactory = NULL;

	ExtensionService* org_extensionService = NULL;

	BrowserView* browserView = NULL;
	ToolbarView* toolbarView = NULL;
	BrowserActionsContainer* browserActionsContainer = NULL;
	ToolbarActionsBar* toolbarActionsBar = NULL;

	do
	{
		if (!CreateAccountDir2(pcszUserIdMd5))
		{
			break;
		}

		extensionPrefsFactory = extensions::ExtensionPrefsFactory::GetInstance();
		if (NULL == extensionPrefsFactory)
		{
			break;
		}
		
		toolbarActionsModelFactory = ToolbarActionsModelFactory::GetInstance();
		if (NULL == toolbarActionsModelFactory)
		{
			break;
		}
		
		extensionRegistryFactory = extensions::ExtensionRegistryFactory::GetInstance();
		if (NULL == extensionRegistryFactory)
		{
			break;
		}
		
		extensionPrefValueMapFactory = ExtensionPrefValueMapFactory::GetInstance();
		if (NULL == extensionPrefValueMapFactory)
		{
			break;
		}

		browser_Active = chrome::FindLastActive();
		if (NULL == browser_Active)
		{
			break;
		}

		profile = browser_Active->profile();
		if (NULL == profile)
		{
			break;
		}

		browserContext = profile;


		// Returns the BrowserView used for the specified Browser.
		browserView = BrowserView::GetBrowserViewForBrowser(browser_Active);

		// Accessor for the Toolbar.
		toolbarView = browserView->toolbar();

		browserActionsContainer = toolbarView->browser_actions();

		toolbarActionsBar = browserActionsContainer->toolbar_actions_bar();

		//toolbarActionsBar->DeleteActions(); ReloadModel();

		// change toolbarActionsBar 中对应的model_

		// 获取原始 extension_service
		org_extensionService = extensions::ExtensionSystem::Get(browserContext)->extension_service();
		org_extensionService->Change_install_directory(wzUserExtensionsPath);
		
		// 1
		KeyedService_ExtensionPrefValueMapFactory =
			extensionPrefValueMapFactory->BuildServiceInstanceFor_Public_ExtensionPrefValueMapFactory(browserContext);

		if (NULL == KeyedService_ExtensionPrefValueMapFactory)
		{
			break;
		}
		extensionPrefValueMapFactory->DisassociateAndChange((base::SupportsUserData*)browserContext, KeyedService_ExtensionPrefValueMapFactory);

		// 2
		KeyedService_ExtensionPrefs =
			extensionPrefsFactory->BuildServiceInstanceFor_Pubilc_ExtensionPrefsFactory(browserContext, bLogin, pcszUserIdMd5);
		
		if (NULL == KeyedService_ExtensionPrefs)
		{
			break;
		}
		extensionPrefsFactory->DisassociateAndChange((base::SupportsUserData*)browserContext, KeyedService_ExtensionPrefs);
		
		KeyedService_ExtensionRegistryFactory =
			extensionRegistryFactory->BuildServiceInstanceFor_Public_ExtensionRegistryFactory(browserContext);
		
		if (NULL == KeyedService_ExtensionRegistryFactory)
		{
			break;
		}
		
		// songqiqi 20161114
		extensionRegistryFactory->DisassociateAndChange((base::SupportsUserData*)browserContext, KeyedService_ExtensionRegistryFactory);
		toolbarActionsModelFactory->DisassociateAndChange((base::SupportsUserData*)browserContext, KeyedService_ToolbarActionsModel);

		toolbarActionsBar->DeleteActions();
		toolbarActionsBar->ReloadModel();

	} while (false);

	base::ThreadRestrictions::SetIOAllowed(false);
}

void AlterExtensions_Just_Dir(bool bLogin, const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);

	Browser* browser_Active = NULL;
	Profile* profile = NULL;
	content::BrowserContext* browserContext = NULL;

	WCHAR wzUserIdMd5[40];
	ZeroMemory(wzUserIdMd5, 80);
	MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);

	WCHAR wzUserExtensionsPath[MAX_PATH];
	ZeroMemory(wzUserExtensionsPath, 2 * MAX_PATH);
	WCHAR *pwzTailPath1 = L"\\ADSafeSe\\User Data\\Default\\";
	WCHAR *pwzTailPath2 = L"Extensions";
	SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wzUserExtensionsPath);
	wcscat(wzUserExtensionsPath, pwzTailPath1);

	if (bLogin)
	{
		wcscat(wzUserExtensionsPath, wzUserIdMd5);
		wcscat(wzUserExtensionsPath, L"\\");
	}

	wcscat(wzUserExtensionsPath, pwzTailPath2);

	ExtensionService* org_extensionService = NULL;
	do
	{
		if (!CreateAccountDir2(pcszUserIdMd5))
		{
			break;
		}

		browser_Active = chrome::FindLastActive();
		if (NULL == browser_Active)
		{
			break;
		}

		profile = browser_Active->profile();
		if (NULL == profile)
		{
			break;
		}

		browserContext = profile;

		// 获取原始 extension_service
		org_extensionService = extensions::ExtensionSystem::Get(browserContext)->extension_service();

		org_extensionService->Change_install_directory(wzUserExtensionsPath);

	} while (false);

	base::ThreadRestrictions::SetIOAllowed(false);
}

BOOL CreateAccountDir2(const char* pcszUserIdMd5)
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

#endif  // AD_EXTENSIONS_H_



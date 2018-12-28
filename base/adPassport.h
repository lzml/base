#ifndef AD_PASSPORT_H_
#define AD_PASSPORT_H_

#include <Windows.h>
#include <io.h>
#include <Shlobj.h>
#include <wincrypt.h>

#include "components/autofill/core/common/password_form.h"
#include "chrome/browser/importer/profile_writer.h"

#include "base\md5.h"
#include "third_party\sqlite\sqlite3.h"
#include "chrome\browser\ui\browser.h"
#include "chrome\browser\ui\browser_finder.h"

#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/password_manager/password_store_win.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"
#include "components/password_manager/core/browser/login_database.h"

#include "chrome\browser\ui\views\frame\browser_view.h"

/*songqiqi 20161102 export api*/
void ExportPwds(const char* pcszUserIdMd5);
void ImportPwds(const char* pcszUserIdMd5);
// 书签MD5-128值 小写字母 失败时返回 return std::string("");
std::string CalOrgPwdsDigest(const char* pcszUserIdMd5, bool bLoginData = false);
/*Done*/

/*songqiqi 20161102 internel api*/
bool GetPasswordsFromSql3(std::vector<autofill::PasswordForm> *forms, bool exportPwd, const char* pcszUserIdMd5);
std::string CalOrgPwdsDigestWithPath(char* szFilePath, bool bLoginData);
bool ExistLoginDataX(const char* pcszUserIdMd5, bool bLoginData);
/*Done*/

static std::string s_strUserIdMd5("");

bool ExistLoginDataX(const char* pcszUserIdMd5, bool bLoginData)
{
	char szUserPwdsPath[MAX_PATH];
	ZeroMemory(szUserPwdsPath, MAX_PATH);
	char *pszTailPath = "\\ADSafeSe\\User Data\\Default\\";
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szUserPwdsPath);
	strcat(szUserPwdsPath, pszTailPath);
	strcat(szUserPwdsPath, pcszUserIdMd5);
	if (bLoginData)
	{
		strcat(szUserPwdsPath, "\\Login Data");
	}
	else
	{
		strcat(szUserPwdsPath, "\\LoginDataUser");
	}

	if (-1 == (_access(szUserPwdsPath, 0)))
	{
		return false;
	}
	else
	{
		return true;
	}
}

std::string CalOrgPwdsDigest(const char* pcszUserIdMd5, bool bLoginData)
{
	char szUserPwdsPath[MAX_PATH];
	ZeroMemory(szUserPwdsPath, MAX_PATH);
	char *pszTailPath = "\\ADSafeSe\\User Data\\Default\\";
	SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szUserPwdsPath);
	strcat(szUserPwdsPath, pszTailPath);
	strcat(szUserPwdsPath, pcszUserIdMd5);
	if (bLoginData)
	{
		strcat(szUserPwdsPath, "\\Login Data");
	}
	else
	{
		strcat(szUserPwdsPath, "\\LoginDataUser");
	}

	if (-1 == (_access(szUserPwdsPath, 0)))
	{
		return std::string("");
	}
	else
	{
		return CalOrgPwdsDigestWithPath(szUserPwdsPath, bLoginData);
	}
}

std::string CalOrgPwdsDigestWithPath(char* szFilePath, bool bLoginData)
{
	DWORD dwError = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	if (bLoginData)
	{
		hFile = CreateFileA(szFilePath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}
	else
	{
		hFile = CreateFileA(szFilePath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	}

	if (INVALID_HANDLE_VALUE == hFile)
	{
		dwError = GetLastError();
		return std::string("");
	}

	DWORD dwFileSize = GetFileSize(hFile, 0);

	DWORD dwReadBytes = 0;
	char *text_content = (char*)malloc(dwFileSize + 10);
	if (NULL == text_content)
	{
		dwError = GetLastError();
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		return std::string("");
	}

	ZeroMemory(text_content, dwFileSize + 10);

	BOOL bReadResult = FALSE;
	bReadResult = ReadFile(hFile, text_content, dwFileSize, &dwReadBytes, NULL);
	if (!bReadResult || dwReadBytes != dwFileSize)
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

/*
函数名称: MixMem
函数说明：加解密内存到内存
作 成 者：丁立富
作成日期：$DATE$
返 回 值: TRUE 加解密成功；FALSE 加解密失败
参    数:
bEncry :true 加密；false 解密
szSrc 需要处理的内存地址
dwSrcSize  需要处理的内存长度
szDst 处理以后的字符串
dwDstSize 处理以后的字符串长度
*/
typedef BOOL(WINAPI *lpfnMixMem)(bool bEncry, IN WCHAR* szSrc, IN DWORD dwSrcSize, OUT WCHAR*& szDst, OUT DWORD& dwDstSize);
typedef void (WINAPI *lpfnReleaseNewBuf)(LPVOID);

// songqiqi 20160919 获取密码 GetPasswordsFromSql3
bool GetPasswordsFromSql3(std::vector<autofill::PasswordForm> *forms, bool exportPwd, const char* pcszUserIdMd5)
{
	HANDLE hAdsCdn = ::LoadLibraryA("AdsCdn.dll");
	lpfnMixMem MixMem = NULL;
	lpfnReleaseNewBuf ReleaseNewBuf = NULL;

	if (NULL == hAdsCdn)
	{
		return false;
	}else
	{
		MixMem = (lpfnMixMem)GetProcAddress(HMODULE(hAdsCdn), "MixMem");
		ReleaseNewBuf = (lpfnReleaseNewBuf)GetProcAddress(HMODULE(hAdsCdn), "ReleaseNewBuf");
		if (NULL == MixMem || NULL == ReleaseNewBuf)
		{
			CloseHandle(hAdsCdn);
			return false;
		}
	}

	bool bReturnVal = false;

	WCHAR wzUserIdMd5[40];
	ZeroMemory(wzUserIdMd5, 80);
	MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);

	const WCHAR* lpcwOrgLoginData = L"\\ADSafeSe\\User Data\\Default\\";

	WCHAR wzLoginDataPwds[MAX_PATH];
	ZeroMemory(wzLoginDataPwds, MAX_PATH * 2);

	SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wzLoginDataPwds);

	wcscat(wzLoginDataPwds, lpcwOrgLoginData);
	wcscat(wzLoginDataPwds, wzUserIdMd5);

	if (exportPwd)
	{
		wcscat(wzLoginDataPwds, L"\\Login Data");
	}
	else
	{
		wcscat(wzLoginDataPwds, L"\\LoginDataUser");
	}

	const char* lpcszQueryStm = "SELECT origin_url, signon_realm, ssl_valid, username_element, username_value, password_element, password_value, action_url FROM logins";
	sqlite3 * pdb_pwds = NULL;
	int nRst = 0;
	sqlite3_stmt *pStmt;
	bool bOpenDB = false;

	char szLoginDataPwds[MAX_PATH];
	ZeroMemory(szLoginDataPwds, MAX_PATH);

	do
	{
		// 密码文件不存在的时候 退出
		if (-1 == (_waccess(wzLoginDataPwds, 0)))
		{
			break;
		}

		WideCharToMultiByte(
			CP_UTF8,
			0,
			wzLoginDataPwds,
			wcsnlen_s(wzLoginDataPwds, MAX_PATH),
			szLoginDataPwds,
			MAX_PATH,
			NULL,
			NULL
		);

		// 密码文件不存在的时候 退出
		if (-1 == (_access(szLoginDataPwds, 0)))
		{
			break;
		}

		// 以只读方式打开数据库 避免没有权限的问题
		nRst = sqlite3_open_v2(szLoginDataPwds, &pdb_pwds, SQLITE_OPEN_READONLY, NULL);

		//nRst = sqlite3_open16(wzLoginDataPwds, &pdb_pwds);
		if (nRst != SQLITE_OK)
		{
			break;
		}

		bOpenDB = true;

		nRst = sqlite3_prepare_v2(pdb_pwds, lpcszQueryStm, -1, &pStmt, NULL);

		if (nRst != SQLITE_OK)
		{
			break;
		}

		DATA_BLOB input;
		DATA_BLOB output;

		char szPassword[MAX_PATH];
		WCHAR wzPassword[MAX_PATH];
		DWORD dwError = 0;
		BOOL bCryptUnprotectData = false;

		BOOL bMixMem = false;
		WCHAR* lpwzDstBuf = NULL;
		DWORD dwDstSize = 0;
		WCHAR* lpwzTmp = NULL;

		while (sqlite3_step(pStmt) == SQLITE_ROW)
		{
			autofill::PasswordForm form;
			bCryptUnprotectData = false;

			// 0 origin_url---VARCHAR NOT NULL---GURL
			form.origin = GURL((char *)sqlite3_column_text(pStmt, 0));
			// 1 signon_realm---VARCHAR NOT NULL---std::string
			form.signon_realm = std::string((char *)sqlite3_column_text(pStmt, 1));
			// 2 ssl_valid---INTEGER NOT NULL---bool
			if (0 == sqlite3_column_int(pStmt, 2))
			{
				form.ssl_valid = false;
			}
			else
			{
				form.ssl_valid = true;
			}
			// 3 username_element---VARCHAR---base:string16
			form.username_element = base::string16((WCHAR*)sqlite3_column_text16(pStmt, 3));
			// 4 username_value---VARCHAR---base:string16
			form.username_value = base::string16((WCHAR*)sqlite3_column_text16(pStmt, 4));
			// 5 password_element---VARCHAR---base:string16
			form.password_element = base::string16((WCHAR*)sqlite3_column_text16(pStmt, 5));

			if (exportPwd)
			{
				/*以下是加密数据的解决方案*/
				//6 password_value---BLOB---base:string16
				input.pbData = (unsigned char *)sqlite3_column_blob(pStmt, 6);
				input.cbData = sqlite3_column_bytes(pStmt, 6);

				bCryptUnprotectData = CryptUnprotectData(&input, NULL, NULL, NULL, NULL, 0, &output);
				if (bCryptUnprotectData)
				{
					ZeroMemory(szPassword, MAX_PATH);
					ZeroMemory(wzPassword, MAX_PATH * 2);
					memcpy_s(szPassword, MAX_PATH, output.pbData, output.cbData);
					MultiByteToWideChar(CP_ACP, 0, szPassword, MAX_PATH, wzPassword, MAX_PATH);

					bMixMem = MixMem(true, wzPassword, wcslen(wzPassword) ,lpwzDstBuf, dwDstSize);
					if ( !bMixMem )
					{
						break;
					}

					form.password_value = lpwzDstBuf;
					ReleaseNewBuf(lpwzDstBuf);
				}
				else
				{
					dwError = GetLastError();
					break;
				}
			}
			else
			{
				lpwzTmp = (WCHAR*)sqlite3_column_text16(pStmt, 6);
				bMixMem = MixMem(false, lpwzTmp, wcslen(lpwzTmp), lpwzDstBuf, dwDstSize);
				if (!bMixMem)
				{
					break;
				}

				form.password_value = lpwzDstBuf;
				ReleaseNewBuf(lpwzDstBuf);

				// 6 password_value---VARCHAR---base:string16
				//form.password_value = base::string16();
			}

			// 7 action_url---VARCHAR NOT NULL---GURL
			form.action = GURL((char *)sqlite3_column_text(pStmt, 7));

			if (!form.username_value.empty() || !form.password_value.empty())
			{
				forms->push_back(form);
			}
		}

		sqlite3_finalize(pStmt);

		bReturnVal = true;

	} while (false);

	if (bOpenDB && NULL != pdb_pwds)
	{
		sqlite3_close(pdb_pwds);
	}

	return bReturnVal;
}

password_manager::LoginDatabase* GetLoginDB()
{
	Browser* browser_ = chrome::FindLastActive();
	Profile* profile = browser_->profile();

	PasswordStoreFactory* pwdStoreFactory = PasswordStoreFactory::GetInstance();
	base::SupportsUserData * context = pwdStoreFactory->GetContextToUsePublic((content::BrowserContext *)profile);

	if (NULL == context)
	{
		return NULL;
	}

	PasswordStoreWin* ps = static_cast<PasswordStoreWin*>(
		pwdStoreFactory->GetPasswordStoreService(context).get());

	if (NULL == ps)
	{
		return NULL;
	}

	password_manager::LoginDatabase* login_db = ps->login_db();

	return login_db;

}

// songqiqi 20160921 导出密码 exportpasswords
void ExportPwds(const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);
	bool bExistLoginDataUser = false;
	bool bExistLoginData = ExistLoginDataX(pcszUserIdMd5, true);

	if ( !bExistLoginData)
	{
		base::ThreadRestrictions::SetIOAllowed(false);
		return;
	}

	std::string strNowUserIdMd5 = CalOrgPwdsDigest(pcszUserIdMd5, true);
	
	if (strNowUserIdMd5.empty())
	{
		base::ThreadRestrictions::SetIOAllowed(false);
		return;
	}
	
	if (0 == s_strUserIdMd5.compare(strNowUserIdMd5) )
	{
		bExistLoginDataUser = ExistLoginDataX(pcszUserIdMd5, false);
		if (bExistLoginDataUser)	//存在UserData
		{
			base::ThreadRestrictions::SetIOAllowed(false);
			return;
		}
	}
	else
	{
		s_strUserIdMd5 = strNowUserIdMd5;
	}

	// 判断结束 开始导入操作
	HANDLE hAdsCdn = ::LoadLibraryA("AdsCdn.dll");
	lpfnMixMem MixMem = NULL;
	lpfnReleaseNewBuf ReleaseNewBuf = NULL;

	if (NULL == hAdsCdn)
	{
		base::ThreadRestrictions::SetIOAllowed(false);
		return;
	}
	else
	{
		MixMem = (lpfnMixMem)GetProcAddress(HMODULE(hAdsCdn), "MixMem");
		ReleaseNewBuf = (lpfnReleaseNewBuf)GetProcAddress(HMODULE(hAdsCdn), "ReleaseNewBuf");
		if (NULL == MixMem || NULL == ReleaseNewBuf)
		{
			CloseHandle(hAdsCdn);
			base::ThreadRestrictions::SetIOAllowed(false);
			return;
		}
	}

	WCHAR wzUserIdMd5[40];
	ZeroMemory(wzUserIdMd5, 80);
	MultiByteToWideChar(CP_ACP, 0, pcszUserIdMd5, strlen(pcszUserIdMd5), wzUserIdMd5, 40);

	const WCHAR* lpcwNewLoginData = L"\\ADSafeSe\\User Data\\Default\\";

	WCHAR wzLoginDataPwds[MAX_PATH];
	ZeroMemory(wzLoginDataPwds, MAX_PATH * 2);

	SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wzLoginDataPwds);

	wcscat(wzLoginDataPwds, lpcwNewLoginData);
	wcscat(wzLoginDataPwds, wzUserIdMd5);
	wcscat(wzLoginDataPwds, L"\\LoginDataUser");

	const char* lpcszCrtTbl =
		"CREATE TABLE logins (\
		origin_url VARCHAR NOT NULL,\
		action_url VARCHAR,\
		username_element VARCHAR,\
		username_value VARCHAR,\
		password_element VARCHAR,\
		password_value VARCHAR,\
		signon_realm VARCHAR NOT NULL,\
		ssl_valid INTEGER NOT NULL,\
		UNIQUE (origin_url, username_element, username_value,\
		password_element, signon_realm))";

	const char* lpcszInsertPwds =
		"INSERT INTO logins \
		(origin_url, action_url, username_element, username_value, password_element, password_value, signon_realm, ssl_valid) \
		VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

	bool bOpenDB = true;
	sqlite3 * newdb_ = NULL;
	int nRst = 0;
	char *errmsg = NULL;
	bool bFirstReset = true;

	ScopedVector<autofill::PasswordForm> forms;
	WCHAR wzLoginDataPwdsMix[MAX_PATH];

	do
	{
		password_manager::LoginDatabase* login_db = GetLoginDB();
		if (NULL == login_db)
		{
			break;
		}

		login_db->GetAutoSignInLoginsExport(&forms);

		if (forms.empty())
		{
			break;
		}

		//  删除数据库文件
		DeleteFileW(wzLoginDataPwds);

		// 创建数据库
		nRst = sqlite3_open16(wzLoginDataPwds, &newdb_);
		if (nRst != SQLITE_OK)
		{
			newdb_ = NULL;
			break;
		}

		bOpenDB = true;

		// 创建表
		nRst = sqlite3_exec(
			newdb_,         /* An open database */
			lpcszCrtTbl,    /* SQL to be evaluated */
			NULL,			/* Callback function */
			NULL,           /* 1st argument to callback */
			&errmsg         /* Error msg written here */
		);

		if (nRst != SQLITE_OK)
		{
			break;
		}

		// 插入数据

		// 1 sqlite3_prepare_v2
		sqlite3_stmt *pStmt;
		nRst = sqlite3_prepare_v2(newdb_, lpcszInsertPwds, -1, &pStmt, NULL);

		if (nRst != SQLITE_OK)
		{
			break;
		}

		BOOL bMixMem = false;
		WCHAR* lpwzDstBuf = NULL;
		DWORD dwDstSize = 0;

		int nSslValid = 0;
		for (size_t i = 0; i < forms.size(); ++i)
		{
			if (bFirstReset)
			{
				bFirstReset = false;
			}
			else
			{
				sqlite3_reset(pStmt);
			}

			// 2 sqlite3_bind_*()

			// 2.1 origin_url VARCHAR NOT NULL,
			if (forms[i]->origin.spec().empty())
			{
				continue;
			}
			nRst = sqlite3_bind_text(pStmt, 1, forms[i]->origin.spec().c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.2 action_url VARCHAR
			nRst = sqlite3_bind_text(pStmt, 2, forms[i]->action.spec().c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.3 username_element VARCHAR
			nRst = sqlite3_bind_text16(pStmt, 3, forms[i]->username_element.c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.4 username_value VARCHAR
			nRst = sqlite3_bind_text16(pStmt, 4, forms[i]->username_value.c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.5 password_element VARCHAR
			nRst = sqlite3_bind_text16(pStmt, 5, forms[i]->password_element.c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			const WCHAR* lpcwzPwdsUnCropt = forms[i]->password_value.c_str();
			bMixMem = MixMem(true, const_cast<wchar_t*>(lpcwzPwdsUnCropt), wcslen(lpcwzPwdsUnCropt), lpwzDstBuf, dwDstSize);
			if (!bMixMem)
			{
				continue;
			}

			ZeroMemory(wzLoginDataPwdsMix, MAX_PATH * 2);
			wcscpy(wzLoginDataPwdsMix, lpwzDstBuf);

			// 2.6 password_value VARCHAR
			nRst = sqlite3_bind_text16(pStmt, 6, wzLoginDataPwdsMix, -1, NULL);
			ReleaseNewBuf(lpwzDstBuf);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.7 signon_realm VARCHAR NOT NULL
			if (forms[i]->signon_realm.empty())
			{
				continue;
			}
			nRst = sqlite3_bind_text(pStmt, 7, forms[i]->signon_realm.c_str(), -1, NULL);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 2.8 ssl_valid INTEGER NOT NULL
			if (forms[i]->ssl_valid)
			{
				nSslValid = 1;
			}
			else
			{
				nSslValid = 0;
			}
			nRst = sqlite3_bind_int(pStmt, 8, nSslValid);
			if (nRst != SQLITE_OK)
			{
				continue;
			}

			// 3 sqlite3_step()
			sqlite3_step(pStmt);

			// 4 sqlite3_reset()
		}

		sqlite3_finalize(pStmt);

	} while (false);

	if (bOpenDB && NULL != newdb_)
	{
		sqlite3_close(newdb_);
	}
	base::ThreadRestrictions::SetIOAllowed(false);
}

void ImportPwds(const char* pcszUserIdMd5)
{
	base::ThreadRestrictions::SetIOAllowed(true);
	Browser* browser_ = chrome::FindLastActive();
	// songqiqi 20160920 尝试从这里
	std::vector<autofill::PasswordForm> forms;
	GetPasswordsFromSql3(&forms, false, pcszUserIdMd5);
	scoped_refptr<ProfileWriter> pw = new ProfileWriter(browser_->profile());

	for (size_t i = 0; i < forms.size(); ++i)
	{
		pw->AddPasswordForm(forms[i]);
	}

	base::ThreadRestrictions::SetIOAllowed(false);
}


#endif  // AD_PASSPORT_H_

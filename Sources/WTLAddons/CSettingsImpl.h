#pragma once

#include <fstream>
#include <map>
#include "CTextFile.h"

#define MAX_KEY_NAME  255
#define MAX_KEY_VALUE 1000

typedef map<wstring, wstring> CMapSettings;

//////////////////////////////////////////////////////////////////////////
// ����� ��� ������ � ����������� ���������
//
class CSettingsImpl
{
	public:

		// ����������
		CSettingsImpl()
		{
			m_hkMain = HKEY_CURRENT_USER;
		}

		CSettingsImpl(BOOL bHKLM)
		{
			if (TRUE == bHKLM)
			{
				m_hkMain = HKEY_LOCAL_MACHINE;
			}
			else
			{
				m_hkMain = HKEY_CURRENT_USER;
			}
		}

		// ����������
		~CSettingsImpl()
		{
			;
		}

	// �������� ������
	private:
		HKEY         m_hkMain;      // ���� ������� ��� �������� ��������
		wstring      m_wsCompany;   // ������������ ��������
		wstring      m_wsProgram;   // ������������ ���������
		wstring      m_wsKeyPath;   // ������ � ����� ������� ��� �������� ���������
		CMapSettings m_mapSettings; // ������ �������� ���������
		BOOL         m_bChanged;    // ������, ���� ��������� ���� ��������
		BOOL         m_bFile;       // ������, ���� ��������� �������� � �����
		wstring      m_wsIni;       // ������ ����� � ini-�����

	// ������ ������
	private:

		//////////////////////////////////////////////////////////////////////////
		// ������������ ���� � ����������
		//
		inline wstring GetRegKeyPath()
		{
			ATLASSERT(!m_wsCompany.empty() && !m_wsProgram.empty());

			return L"Software\\" + m_wsCompany + L"\\" + m_wsProgram;
		}

		//////////////////////////////////////////////////////////////////////////
		// ������ ����������
		//
		INT ReadSettings()
		{
			ATLASSERT(!m_wsCompany.empty() && !m_wsProgram.empty());
			if (m_wsCompany.empty() || m_wsProgram.empty())
			{
				return 0;
			}

			// ��������� ��������, ������ �������� ���������
			wstring       wsPath;
			vector<WCHAR> pwcPath(MAX_PATH);

			if (!GetModuleFileName(GetModuleHandle(NULL), &pwcPath.front(), MAX_PATH))
			{
				return 0;
			}

			wsPath = &pwcPath.front();
			wsPath.erase(wsPath.rfind(L"\\") + 1);

			// �������� � ������� �������� INI �����
			m_wsIni = wsPath + m_wsProgram + L".ini";
			ATLASSERT(!m_wsIni.empty());

			CTextFileRead fileIni(m_wsIni.c_str());

			// ���� ���� ����, �� ������ �������� �� �����
			if (fileIni.IsOpen())
			{
				m_bFile = TRUE;

				wstring wsLine;

				while (!fileIni.Eof())
				{
					fileIni.ReadLine(wsLine);

					// ���������� ������ ������
					if (wsLine.empty())
					{
						continue;
					}

					// ���������� �������� ������
					if (L'[' == wsLine[0])
					{
						continue;
					}

					// ����� = (�����)
					wstring::size_type indexEqual = wsLine.find(L'=');

					// ���������� ������, ���� �� ����� =
					if (indexEqual == wstring::npos)
					{
						continue;
					}

					// �������� �������� ���������
					wstring wsName = wsLine.substr(0, indexEqual);

					wsName.erase(0, wsName.find_first_not_of(L' '));
					wsName.erase(wsName.find_last_not_of(L' ') + 1, wstring::npos);

					// �������� �������� ���������
					wstring wsValue = wsLine.substr(++indexEqual);
					
					wsValue.erase(0, wsValue.find_first_not_of(L' '));
					wsValue.erase(wsValue.find_last_not_of(L' ') + 1, wstring::npos);

					// ���������� ��������
					m_mapSettings.insert(make_pair(wsName, wsValue));
				}

				fileIni.Close();
			}

			// ���� ini ����� ���, �� ������ �������� �� �������
			else
			{
				HKEY hKeySetting = NULL;
				
				// ������������ ���� � ����������
				m_wsKeyPath  = GetRegKeyPath();

				while (TRUE, TRUE)
				{
					// ��������� ������
					if (ERROR_SUCCESS != RegOpenKeyEx(m_hkMain, m_wsKeyPath.c_str(), 0, KEY_READ, &hKeySetting))
					{
						break;
					}

					// ��������� ���������� ���������
					DWORD dwValues = 0;

					if (ERROR_SUCCESS != RegQueryInfoKey(hKeySetting, NULL, NULL, NULL, NULL, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL))
					{
						break;
					}

					if (0 < dwValues)
					{
						vector<WCHAR> pwcKeyName(MAX_KEY_NAME);
						DWORD         dwLengthName;
						DWORD         dwTypeValue;
						vector<WCHAR> pwcKeyValue(MAX_KEY_VALUE);
						DWORD         dwLengthValue;

						// ���� �� ���� ��������� ����� �������
						for (DWORD i = 0; i < dwValues; i++)
						{
							pwcKeyName[0]  = 0;
							dwLengthName   = MAX_KEY_NAME;
							pwcKeyValue[0] = 0;
							dwLengthValue  = MAX_KEY_VALUE * sizeof(WCHAR);

							// �������� ������������ � �������� ��������
							if (ERROR_SUCCESS != RegEnumValue(hKeySetting, i, &pwcKeyName.front(), &dwLengthName, NULL, &dwTypeValue, (LPBYTE)&pwcKeyValue.front(), &dwLengthValue))
							{
								break;
							}

							// ���� �� ������, �� ����������
							if (REG_SZ != dwTypeValue)
							{
								continue;
							}

							// ���������� ��������
							m_mapSettings.insert(make_pair(&pwcKeyName.front(), &pwcKeyValue.front()));
						}
					}

					break;
				}

				// ��������� ������
				if (NULL != hKeySetting)
				{
					ATLVERIFY(ERROR_SUCCESS == RegCloseKey(hKeySetting));
				}
			}

			return (DWORD)m_mapSettings.size();
		}


		//////////////////////////////////////////////////////////////////////////
		// ������ ����������
		//
		INT WriteSettings()
		{
			// �������� ��������� ����������
			if (FALSE == m_bChanged)
			{
				return 0;
			}

			// �������� ������ ��� �������� ����������
			ATLASSERT(!m_wsCompany.empty() && !m_wsProgram.empty());
			if (m_wsCompany.empty() || m_wsProgram.empty())
			{
				return 0;
			}

			// �������� ������� ����������
			if (0 == m_mapSettings.size())
			{
				return 0;
			}

			// ������ � ����
			if (TRUE == m_bFile)
			{
				ATLASSERT(!m_wsIni.empty());
				CTextFileWrite fileIni(m_wsIni.c_str(), CTextFileWrite::UTF_8);

				if (fileIni.IsOpen())
				{
					for (CMapSettings::iterator setting = m_mapSettings.begin(); setting != m_mapSettings.end(); setting++)
					{
						fileIni << setting->first.c_str() << L" = " << setting->second.c_str();
						fileIni.WriteEndl();
					}

					fileIni.Close();
				}
			}

			// ������ � ������
			else
			{
				HKEY hKeySetting = NULL;

				while (TRUE, TRUE)
				{
					ATLASSERT(!m_wsKeyPath.empty());
					
					// ��������� ������
					if (ERROR_SUCCESS != RegOpenKeyEx(m_hkMain, m_wsKeyPath.c_str(), 0, KEY_SET_VALUE, &hKeySetting))
					{
						// ������� �����
						if (ERROR_SUCCESS != RegCreateKey(m_hkMain, m_wsKeyPath.c_str(), &hKeySetting))
						{
							break;
						}
					}

					// ���������� ������
					for (CMapSettings::iterator setting = m_mapSettings.begin(); setting != m_mapSettings.end(); setting++)
					{
						if (ERROR_SUCCESS != RegSetValueEx(hKeySetting, setting->first.c_str(), 0, REG_SZ, (PBYTE)(setting->second.c_str()), (DWORD)(setting->second.size() * sizeof(wstring::value_type))))
						{
							ATLTRACE(L"ERROR::RegSetKeyValue %s %s", setting->first.c_str(), setting->second.c_str());
						}
					}		

					break;
				}

				if (NULL != hKeySetting)
				{
					ATLVERIFY(ERROR_SUCCESS == RegCloseKey(hKeySetting));
				}
			}

			return 0;
		}

	public:
		
		//////////////////////////////////////////////////////////////////////////
		// �������� ��������
		//
		VOID OpenSettings(wstring wsCompany, wstring wsProgram)
		{
			m_wsCompany = wsCompany;
			m_wsProgram = wsProgram;
			m_wsIni     = L"";
			m_wsKeyPath = L"";
			m_bChanged  = FALSE;
			m_bFile     = FALSE;

			ReadSettings();
		}


		//////////////////////////////////////////////////////////////////////////
		// �������� ��������
		//
		VOID CloseSettings(BOOL bSave = TRUE)
		{
			m_bChanged = TRUE;
			if ((TRUE == bSave) && (TRUE == m_bChanged))
			{
				WriteSettings();
			}

			m_wsCompany = L"";
			m_wsProgram = L"";
			m_wsIni     = L""; 
			m_wsKeyPath = L"";

			m_mapSettings.clear();
		}


		//////////////////////////////////////////////////////////////////////////
		// �������� ��������� ��������
		//
		wstring GetSettingsString(wstring wsName, wstring wsDefault = L"")
		{
			ATLASSERT(!wsName.empty());
			CMapSettings::iterator settings = m_mapSettings.find(wsName);

			if (m_mapSettings.end() != settings)
			{
				return settings->second;
			}

			m_mapSettings.insert(make_pair(wsName, wsDefault));

			return wsDefault;
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ��������� ��������
		//
		VOID SetSettingsString(wstring wsName, wstring wsValue = L"")
		{
			ATLASSERT(!wsName.empty());

			m_mapSettings[wsName] = wsValue;
		}


		//////////////////////////////////////////////////////////////////////////
		// �������� ������������� ��������
		//
		INT GetSettingsInteger(wstring wsName, INT iDefault = 0)
		{
			ATLASSERT(!wsName.empty());

			vector<WCHAR> wcDefault(16);
			_itow_s(iDefault, &wcDefault.front(), 15, 10);

			return _wtoi(GetSettingsString(wsName, &wcDefault.front()).c_str());
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ������������� ��������
		//
		VOID SetSettingsInteger(wstring wsName, INT iValue = 0)
		{
			ATLASSERT(!wsName.empty());

			vector<WCHAR> wcValue(16);
			_itow_s(iValue, &wcValue.front(), wcValue.size() - 1, 10);

			m_mapSettings[wsName] = &wcValue.front();
		}


		//////////////////////////////////////////////////////////////////////////
		// �������� ������������ ��������
		//
		FLOAT GetSettingsFloat(wstring wsName, FLOAT fDefault = 0.0f)
		{
			ATLASSERT(!wsName.empty());

			vector<WCHAR> wcDefault(16);
			swprintf(&wcDefault.front(), 15, L"%f", fDefault);

			return (FLOAT)_wtof(GetSettingsString(wsName, &wcDefault.front()).c_str());
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ������������ ��������
		//
		VOID SetSettingsFloat(wstring wsName, FLOAT fValue = 0.0f)
		{
			ATLASSERT(!wsName.empty());

			vector<WCHAR> wcValue(16);
			swprintf(&wcValue.front(), 15, L"%.10f", fValue);

			m_mapSettings[wsName] = &wcValue.front();
		}


		//////////////////////////////////////////////////////////////////////////
		// �������� ���������� ��������
		//
		BOOL GetSettingsBoolean(wstring wsName, BOOL bDefault = TRUE)
		{
			ATLASSERT(!wsName.empty());

			if (L"TRUE" == GetSettingsString(wsName, bDefault ? L"TRUE" : L"FALSE"))
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ���������� ��������
		//
		VOID SetSettingsBoolean(wstring wsName, BOOL bValue = TRUE)
		{
			ATLASSERT(!wsName.empty());

			if (TRUE == bValue)
			{
				m_mapSettings[wsName] = L"TRUE";
			}
			else
			{
				m_mapSettings[wsName] = L"FALSE";
			}
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ���������� �������� �� �������
		//
		wstring GetRegSettingsString(wstring wsName, wstring wsDefault = L"")
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return wsDefault;
			}

			wstring wsReturn    = wsDefault;
			HKEY    hKeySettings = NULL;

			while (TRUE, TRUE)
			{
				// ��������� ������
				if (ERROR_SUCCESS != RegOpenKeyEx(m_hkMain, m_wsKeyPath.c_str(), 0, KEY_QUERY_VALUE, &hKeySettings))
				{
					break;
				}

				vector<WCHAR> pwcKeyValue(MAX_KEY_VALUE);
				DWORD         dwLengthValue;
				DWORD         dwType;

				if (ERROR_SUCCESS != RegQueryValueEx(hKeySettings, wsName.c_str(), 0, &dwType, (LPBYTE)&pwcKeyValue.front(), &dwLengthValue))
				{
					break;
				}

				if (REG_SZ != dwType)
				{
					break;
				}

				wsReturn = &pwcKeyValue.front();

				break;
			}

			// ��������� ������
			if (NULL != hKeySettings)
			{
				ATLVERIFY(ERROR_SUCCESS == RegCloseKey(hKeySettings));
			}

			return wsReturn;
		}


		//////////////////////////////////////////////////////////////////////////
		// ������ ���������� �������� � ������
		//
		BOOL SetRegSettingsString(wstring wsName, wstring wsValue)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return FALSE;
			}

			HKEY hKeySettings = NULL;

			// ��������� ������
			if (ERROR_SUCCESS != RegOpenKeyEx(m_hkMain, m_wsKeyPath.c_str(), 0, KEY_SET_VALUE, &hKeySettings))
			{
				return FALSE;
			}

			if (ERROR_SUCCESS != RegSetValueEx(hKeySettings, wsName.c_str(), 0, REG_SZ, (LPBYTE)(wsValue.c_str()), (DWORD)(wsValue.size() * sizeof(wstring::value_type))))
			{
				ATLVERIFY(ERROR_SUCCESS == RegCloseKey(hKeySettings));

				return FALSE;
			}

			ATLVERIFY(ERROR_SUCCESS == RegCloseKey(hKeySettings));

			return TRUE;
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� �������������� �������� �� �������
		//
		INT GetRegSettingsInteger(wstring wsName, INT iDefault = 0)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return iDefault;
			}

			vector<WCHAR> wcDefault(16);
			_itow_s(iDefault, &wcDefault.front(), wcDefault.size() - 1,10);

			return _wtoi(GetRegSettingsString(wsName, &wcDefault.front()).c_str());
		}


		//////////////////////////////////////////////////////////////////////////
		// ������ � ������ �������������� ��������
		//
		BOOL SetRegSettingsInteger(wstring wsName, INT iValue)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return FALSE;
			}

			vector<WCHAR> wcValue(16);
			_itow_s(iValue, &wcValue.front(), 15, 10);

			return SetRegSettingsString(wsName, &wcValue.front());
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� ������������� �������� �� �������
		//
		FLOAT GetRegSettingsFloat(wstring wsName, FLOAT fDefault = 0)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return fDefault;
			}

			vector<WCHAR> wcDefault(16);
			swprintf(&wcDefault.front(), 15, L"%.10f", fDefault);

			return (FLOAT)_wtof(GetRegSettingsString(wsName, &wcDefault.front()).c_str());
		}


		//////////////////////////////////////////////////////////////////////////
		// ������ � ������ ������������� ��������
		//
		BOOL SetRegSettingsInteger(wstring wsName, FLOAT fValue)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return FALSE;
			}

			vector<WCHAR> wcValue(16);
			swprintf_s(&wcValue.front(), 15, L"%.10f", fValue);

			return SetRegSettingsString(wsName, &wcValue.front());
		}


		//////////////////////////////////////////////////////////////////////////
		// ��������� �� ������� �������� ��������
		//
		BOOL GetRegSettingsBoolean(wstring wsName, BOOL bDefault = TRUE)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return bDefault;
			}

			if (L"TRUE" == GetRegSettingsString(wsName, bDefault ? L"TRUE" : L"FALSE"))
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}


		//////////////////////////////////////////////////////////////////////////
		// ������ � ������ �������� ��������
		//
		BOOL SetRegSettingsBoolean(wstring wsName, BOOL bValue)
		{
			ATLASSERT(!wsName.empty() && !m_wsKeyPath.empty());
			if (wsName.empty() || m_wsKeyPath.empty())
			{
				return FALSE;
			}

			return SetRegSettingsString(wsName, bValue ? L"TRUE" : L"FALSE");
		}
};

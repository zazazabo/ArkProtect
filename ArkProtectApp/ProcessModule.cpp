#include "stdafx.h"
#include "ProcessModule.h"

namespace ArkProtect
{
	CProcessModule *CProcessModule::m_ProcessModule;

	CProcessModule::CProcessModule(CGlobal *GlobalObject, PPROCESS_ENTRY_INFORMATION ProcessEntry)
		: m_Global(GlobalObject)
		, m_ProcessEntry(ProcessEntry)
	{
		m_ProcessModule = this;
	}


	CProcessModule::~CProcessModule()
	{
	}



	/************************************************************************
	*  Name : InitializeProcessModuleList
	*  Param: ProcessInfoList        ProcessModule�Ի����ListControl�ؼ�
	*  Ret  : void
	*  ��ʼ��ListControl����Ϣ
	************************************************************************/
	void CProcessModule::InitializeProcessModuleList(CListCtrl *ListCtrl)
	{
		while (ListCtrl->DeleteColumn(0));
		ListCtrl->DeleteAllItems();

		ListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);

		for (int i = 0; i < m_iColumnCount; i++)
		{
			ListCtrl->InsertColumn(i, m_ColumnStruct[i].wzTitle, LVCFMT_LEFT, (int)(m_ColumnStruct[i].nWidth*(m_Global->iDpiy / 96.0)));
		}
	}


	/************************************************************************
	*  Name : PerfectProcessModuleInfo
	*  Param: ModuleEntry			     ������Ϣ�ṹ
	*  Ret  : void
	*  ���ƽ�����Ϣ�ṹ
	************************************************************************/
	void CProcessModule::PerfectProcessModuleInfo(PPROCESS_MODULE_ENTRY_INFORMATION ModuleEntry)
	{
		// �޼�ģ���ļ�·��
		CString strFullPath = m_Global->TrimPath(ModuleEntry->wzFilePath);
		StringCchCopyW(ModuleEntry->wzFilePath, strFullPath.GetLength() + 1, strFullPath.GetBuffer());
		//wcsncpy_s(ModuleEntry.wzFullPath, MAX_PATH, strFullPath.GetBuffer(), strFullPath.GetLength());

		CString strCompanyName = m_Global->GetFileCompanyName(ModuleEntry->wzFilePath);
		if (strCompanyName.GetLength() == 0)
		{
			strCompanyName = L" ";
		}

		StringCchCopyW(ModuleEntry->wzCompanyName, strCompanyName.GetLength() + 1, strCompanyName.GetBuffer());
	}
	

	/************************************************************************
	*  Name : EnumProcessModule
	*  Param: void
	*  Ret  : BOOL
	*  ��������ͨ�ţ�ö�ٽ���ģ����Ϣ
	************************************************************************/
	BOOL CProcessModule::EnumProcessModule()
	{
		BOOL bOk = FALSE;

		m_ProcessModuleEntryVector.clear();

		UINT32   Count = 0x100;
		DWORD	 dwReturnLength = 0;
		PPROCESS_MODULE_INFORMATION pmi = NULL;

		do
		{
			UINT32 OutputLength = 0;

			if (pmi)
			{
				free(pmi);
				pmi = NULL;
			}

			OutputLength = sizeof(PROCESS_MODULE_INFORMATION) + Count * sizeof(PROCESS_MODULE_ENTRY_INFORMATION);

			pmi = (PPROCESS_MODULE_INFORMATION)malloc(OutputLength);
			if (!pmi)
			{
				break;
			}

			RtlZeroMemory(pmi, OutputLength);

			bOk = DeviceIoControl(m_Global->m_DeviceHandle,
				IOCTL_ARKPROTECT_ENUMPROCESSMODULE,
				&m_ProcessEntry->ProcessId,		// InputBuffer
				sizeof(UINT32),
				pmi,
				OutputLength,
				&dwReturnLength,
				NULL);

			Count = (UINT32)pmi->NumberOfModules + 1000;

		} while (bOk == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

		if (bOk && pmi)
		{
			for (UINT32 i = 0; i < pmi->NumberOfModules; i++)
			{
				// ���ƽ�����Ϣ�ṹ
				PerfectProcessModuleInfo(&pmi->ModuleEntry[i]);
				m_ProcessModuleEntryVector.push_back(pmi->ModuleEntry[i]);
			}
			bOk = TRUE;
		}

		if (pmi)
		{
			free(pmi);
			pmi = NULL;
		}

		if (m_ProcessModuleEntryVector.empty())
		{
			return FALSE;
		}

		return bOk;
	}


	/************************************************************************
	*  Name : InsertProcessInfoList
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ListControl����������Ϣ
	************************************************************************/
	void CProcessModule::InsertProcessModuleInfoList(CListCtrl *ListCtrl)
	{
		UINT32 ProcessModuleNum = 0;
		size_t Size = m_ProcessModuleEntryVector.size();
		for (size_t i = 0; i < Size; i++)
		{
			PROCESS_MODULE_ENTRY_INFORMATION ModuleEntry = m_ProcessModuleEntryVector[i];

			CString strFilePath, strBaseAddress, strImageSize, strCompanyName;
			
			strFilePath = ModuleEntry.wzFilePath;
			strBaseAddress.Format(L"0x%p", ModuleEntry.BaseAddress);
			strImageSize.Format(L"0x%X", ModuleEntry.SizeOfImage);
			strCompanyName = ModuleEntry.wzCompanyName;

			int iItem = ListCtrl->InsertItem(ListCtrl->GetItemCount(), strFilePath);
			ListCtrl->SetItemText(iItem, pmc_BaseAddress, strBaseAddress);
			ListCtrl->SetItemText(iItem, pmc_SizeOfImage, strImageSize);
			ListCtrl->SetItemText(iItem, pmc_Company, strCompanyName);

			ProcessModuleNum++;

			CString strStatusContext;
			strStatusContext.Format(L"Process Info is loading now, Count:%d", ProcessModuleNum);

			m_Global->UpdateStatusBarDetail(strStatusContext);
		}

		CString strStatusContext;
		strStatusContext.Format(L"Process Info load complete, Count:%d", Size);
		m_Global->UpdateStatusBarDetail(strStatusContext);
	}


	/************************************************************************
	*  Name : QueryProcessModule
	*  Param: ListCtrl
	*  Ret  : void
	*  ��ѯ������Ϣ
	************************************************************************/
	void CProcessModule::QueryProcessModule(CListCtrl *ListCtrl)
	{
		ListCtrl->DeleteAllItems();
		m_ProcessModuleEntryVector.clear();

		if (EnumProcessModule() == FALSE)
		{
			m_Global->UpdateStatusBarDetail(L"Process Module Initialize failed");
			return;
		}

		InsertProcessModuleInfoList(ListCtrl);
	}



	/************************************************************************
	*  Name : QueryProcessModuleCallback
	*  Param: lParam ��ListCtrl��
	*  Ret  : DWORD
	*  ��ѯ����ģ��Ļص�
	************************************************************************/
	DWORD CALLBACK CProcessModule::QueryProcessModuleCallback(LPARAM lParam)
	{
		CListCtrl *ListCtrl = (CListCtrl*)lParam;

		m_ProcessModule->m_Global->m_bIsRequestNow = TRUE;      // ��TRUE����������û�з���ǰ����ֹ����������ͨ�ŵĲ���

		m_ProcessModule->m_Global->UpdateStatusBarTip(L"Process Module");
		m_ProcessModule->m_Global->UpdateStatusBarDetail(L"Process Module is loading now...");

		m_ProcessModule->QueryProcessModule(ListCtrl);

		m_ProcessModule->m_Global->m_bIsRequestNow = FALSE;

		return 0;
	}

}
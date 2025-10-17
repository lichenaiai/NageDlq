#include "pch.h"
#include "framework.h"
#include "NageDlqServer.h"
#include "���öԻ�����.h"
#include "afxdialogex.h"

// ���ODBCͷ�ļ�
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ���öԻ����� �Ի���
IMPLEMENT_DYNAMIC(���öԻ�����, CDialogEx)

���öԻ�����::���öԻ�����(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTINGS_DIALOG, pParent)
	, ���ݿ��û���(_T("sa"))
	, ���ݿ�����(_T("password"))
	, ���ݿ�����(_T("nagelogin"))
{
}

���öԻ�����::~���öԻ�����()
{
}

void ���öԻ�����::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_DB_USER, ���ݿ��û���);
	DDX_Text(pDX, IDC_EDIT_DB_PASSWORD, ���ݿ�����);
	DDX_Text(pDX, IDC_EDIT_DB_NAME, ���ݿ�����);
	DDX_Control(pDX, IDC_EDIT_DB_USER, ���ݿ��û��༭��);
	DDX_Control(pDX, IDC_EDIT_DB_PASSWORD, ���ݿ�����༭��);
	DDX_Control(pDX, IDC_EDIT_DB_NAME, ���ݿ����Ʊ༭��);
	DDX_Control(pDX, IDC_BUTTON_TEST_CONNECTION, �������Ӱ�ť);
}

BEGIN_MESSAGE_MAP(���öԻ�����, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_TEST_CONNECTION, &���öԻ�����::OnBnClickedButtonTestConnection)
END_MESSAGE_MAP()

BOOL ���öԻ�����::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������༭��Ϊ����ģʽ
	���ݿ�����༭��.SetPasswordChar('*');

	return TRUE;
}

// �������Ӱ�ť����¼�
void ���öԻ�����::OnBnClickedButtonTestConnection()
{
	UpdateData(TRUE); // ��ȡ��������

	SQLHENV �������;
	SQLHDBC ���Ӿ��;
	SQLRETURN ���ش���;

	// ���价�����
	���ش��� = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &�������);
	if (���ش��� != SQL_SUCCESS) {
		MessageBox(_T("���价�����ʧ��"), _T("��������"), MB_ICONERROR);
		return;
	}

	// ����ODBC�汾
	SQLSetEnvAttr(�������, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

	// �������Ӿ��
	���ش��� = SQLAllocHandle(SQL_HANDLE_DBC, �������, &���Ӿ��);
	if (���ش��� != SQL_SUCCESS) {
		SQLFreeHandle(SQL_HANDLE_ENV, �������);
		MessageBox(_T("�������Ӿ��ʧ��"), _T("��������"), MB_ICONERROR);
		return;
	}

	// �����ַ���
	CString �����ַ���;
	�����ַ���.Format(_T("DRIVER={SQL Server};SERVER=127.0.0.1;DATABASE=%s;UID=%s;PWD=%s;"),
		���ݿ�����, ���ݿ��û���, ���ݿ�����);

	SQLWCHAR* ���ַ������ַ��� = (SQLWCHAR*)�����ַ���.GetBuffer();
	SQLSMALLINT �����ַ����������;

	// ��������
	���ش��� = SQLDriverConnect(���Ӿ��, NULL, ���ַ������ַ���, SQL_NTS,
		NULL, 0, &�����ַ����������, SQL_DRIVER_NOPROMPT);

	�����ַ���.ReleaseBuffer();

	if (���ش��� == SQL_SUCCESS || ���ش��� == SQL_SUCCESS_WITH_INFO) {
		MessageBox(_T("���ݿ����ӳɹ���"), _T("��������"), MB_ICONINFORMATION);
		SQLDisconnect(���Ӿ��);
	}
	else {
		SQLWCHAR SQL״̬[6], ������Ϣ[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER ԭ��������;
		SQLSMALLINT ��Ϣ����;

		SQLGetDiagRec(SQL_HANDLE_DBC, ���Ӿ��, 1, SQL״̬, &ԭ��������,
			������Ϣ, SQL_MAX_MESSAGE_LENGTH, &��Ϣ����);

		MessageBox(_T("���ݿ�����ʧ�ܣ�") + CString(������Ϣ), _T("��������"), MB_ICONERROR);
	}

	SQLFreeHandle(SQL_HANDLE_DBC, ���Ӿ��);
	SQLFreeHandle(SQL_HANDLE_ENV, �������);
}

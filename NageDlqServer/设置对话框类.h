#pragma once
#include "afxdialogex.h"

// ���öԻ����� �Ի���
class ���öԻ����� : public CDialogEx
{
	DECLARE_DYNAMIC(���öԻ�����)

public:
	���öԻ�����(CWnd* pParent = nullptr);
	virtual ~���öԻ�����();

	enum { IDD = IDD_SETTINGS_DIALOG };

public:
	CString ���ݿ��û���;       // ���ݿ��û���
	CString ���ݿ�����;        // ���ݿ�����
	CString ���ݿ�����;        // ���ݿ�����

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonTestConnection();  // �������Ӱ�ť����¼�

private:
	// �ؼ����� - ʹ������
	CEdit ���ݿ��û��༭��;
	CEdit ���ݿ�����༭��;
	CEdit ���ݿ����Ʊ༭��;
	CButton �������Ӱ�ť;
};

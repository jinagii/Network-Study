#pragma once

/// �����ð� �������� ���ϼ̴� �Ҹ��ڿ� leaveCS �� �����غ���
class CLockPoint
{
public:
	CLockPoint(CRITICAL_SECTION* CS);
	~CLockPoint(); // �̺κп� leaveCS �־���-�������������� �Ҹ�ɶ� �ڵ����� ó��

private:
	CRITICAL_SECTION* m_CS;

public:
	// ���� �ߴ�...
	//void InitiateCS();	//
	//void EnterCS();		// ������������ init�� enter�� �־��ش�
	//void DeleteCS();	// ���α׷� �����

};


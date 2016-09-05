#pragma once

#define  SN_MAX_LEN	20
#define  HD_MAX_COUNT	10

typedef unsigned char byte;

struct HDSNInfo
{
	int count;
	byte sn[HD_MAX_COUNT][SN_MAX_LEN + 1];
};

bool GetSNInfo(HDSNInfo& origin, HDSNInfo& changeTo);
bool SpoofHDSN(const HDSNInfo& originSN, const HDSNInfo& newSN);
bool InstallService();
bool UninstallService();
bool IsServiceInstalled();
bool GenRandomSN(int count, HDSNInfo& info);

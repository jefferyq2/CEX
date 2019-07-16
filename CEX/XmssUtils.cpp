#include "XmssUtils.h"

NAMESPACE_XMSS

bool XmssUtils::IsXMSS(XmssParameters Enumeral)
{
	bool ret;

	switch (Enumeral)
	{
	case XmssParameters::XMSSSHA256H10:
	case XmssParameters::XMSSSHA256H16:
	case XmssParameters::XMSSSHA256H20:
	case XmssParameters::XMSSSHA512H10:
	case XmssParameters::XMSSSHA512H16:
	case XmssParameters::XMSSSHA512H20:
	case XmssParameters::XMSSSHAKE256H10:
	case XmssParameters::XMSSSHAKE256H16:
	case XmssParameters::XMSSSHAKE256H20:
	case XmssParameters::XMSSSHAKE512H10:
	case XmssParameters::XMSSSHAKE512H16:
	case XmssParameters::XMSSSHAKE512H20:
	{
		ret = true;
		break;
	}
	default:
	{
		ret = false;
	}
	}

	return ret;
}

uint XmssUtils::ToOid(XmssParameters Enumeral)
{
	uint oid;

	switch (Enumeral)
	{
		// XMSS
	case XmssParameters::XMSSSHA256H10:
		oid = 0x00000001UL;
		break;
	case XmssParameters::XMSSSHA256H16:
		oid = 0x00000002UL;
		break;
	case XmssParameters::XMSSSHA256H20:
		oid = 0x00000003UL;
		break;
	case XmssParameters::XMSSSHA512H10:
		oid = 0x00000004UL;
		break;
	case XmssParameters::XMSSSHA512H16:
		oid = 0x00000005UL;
		break;
	case XmssParameters::XMSSSHA512H20:
		oid = 0x00000006UL;
		break;
	case XmssParameters::XMSSSHAKE256H10:
		oid = 0x00000007UL;
		break;
	case XmssParameters::XMSSSHAKE256H16:
		oid = 0x00000008UL;
		break;
	case XmssParameters::XMSSSHAKE256H20:
		oid = 0x00000009UL;
		break;
	case XmssParameters::XMSSSHAKE512H10:
		oid = 0x0000000AUL;
		break;
	case XmssParameters::XMSSSHAKE512H16:
		oid = 0x0000000BUL;
		break;
	case XmssParameters::XMSSSHAKE512H20:
		oid = 0x0000000CUL;
		break;
		// XMSS-MT
	case XmssParameters::XMSSMTSHA256H20D2:
		oid = 0x00000001UL;
		break;
	case XmssParameters::XMSSMTSHA256H20D4:
		oid = 0x00000002UL;
		break;
	case XmssParameters::XMSSMTSHA256H40D2:
		oid = 0x00000003UL;
		break;
	case XmssParameters::XMSSMTSHA256H40D4:
		oid = 0x00000004UL;
		break;
	case XmssParameters::XMSSMTSHA256H40D8:
		oid = 0x00000005UL;
		break;
	case XmssParameters::XMSSMTSHA256H60D3:
		oid = 0x00000006UL;
		break;
	case XmssParameters::XMSSMTSHA256H60D6:
		oid = 0x00000007UL;
		break;
	case XmssParameters::XMSSMTSHA256H60D12:
		oid = 0x00000008UL;
		break;
	case XmssParameters::XMSSMTSHA512H20D2:
		oid = 0x00000009UL;
		break;
	case XmssParameters::XMSSMTSHA512H20D4:
		oid = 0x0000000AUL;
		break;
	case XmssParameters::XMSSMTSHA512H40D2:
		oid = 0x0000000BUL;
		break;
	case XmssParameters::XMSSMTSHA512H40D4:
		oid = 0x0000000CUL;
		break;
	case XmssParameters::XMSSMTSHA512H40D8:
		oid = 0x0000000DUL;
		break;
	case XmssParameters::XMSSMTSHA512H60D3:
		oid = 0x0000000EUL;
		break;
	case XmssParameters::XMSSMTSHA512H60D6:
		oid = 0x0000000FUL;
		break;
	case XmssParameters::XMSSMTSHA512H60D12:
		oid = 0x00000010UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H20D2:
		oid = 0x00000011UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H20D4:
		oid = 0x00000012UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H40D2:
		oid = 0x00000013UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H40D4:
		oid = 0x00000014UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H40D8:
		oid = 0x00000015UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H60D3:
		oid = 0x00000016UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H60D6:
		oid = 0x00000017UL;
		break;
	case XmssParameters::XMSSMTSHAKE256H60D12:
		oid = 0x00000018UL;
		break;
	case XmssParameters::XMSSMTSHAKE512H20D2:
		oid = 0x00000019UL;
		break;
	case XmssParameters::XMSSMTSHAKE512H20D4:
		oid = 0x0000001AUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H40D2:
		oid = 0x0000001BUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H40D4:
		oid = 0x0000001CUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H40D8:
		oid = 0x0000001DUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H60D3:
		oid = 0x0000001EUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H60D6:
		oid = 0x0000001FUL;
		break;
	case XmssParameters::XMSSMTSHAKE512H60D12:
		oid = 0x00000020UL;
		break;
	default:
		oid = 0x00000000UL;
		break;
	}

	return oid;
}

NAMESPACE_XMSSEND

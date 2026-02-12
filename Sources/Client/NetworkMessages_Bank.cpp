#include "Game.h"
#include "NetworkMessageManager.h"
#include "Packet/SharedPackets.h"
#include "lan_eng.h"
#include "DialogBoxIDs.h"
#include <cstring>
#include <cstdio>
#include <format>
#include <string>


namespace NetworkMessageHandlers {
	void HandleItemToBank(CGame* pGame, char* pData)
	{
		int cIndex;
		DWORD dwCount, dwAttribute;
		char  cName[hb::shared::limits::ItemNameLen]{}, cItemType, cEquipPos, cGenderLimit, cItemColor;
		bool  bIsEquipped;
		short sSprite, sSpriteFrame, sLevelLimit, sItemEffectValue2, sItemSpecEffectValue2;
		WORD wWeight, wCurLifeSpan;
		std::string cTxt;
		const auto* pkt = hb::net::PacketCast<hb::net::PacketNotifyItemToBank>(
			pData, sizeof(hb::net::PacketNotifyItemToBank));
		if (!pkt) return;

		cIndex = static_cast<int>(pkt->bank_index);
		if (cIndex < 0 || cIndex >= hb::shared::limits::MaxBankItems) return;
		memcpy(cName, pkt->name, sizeof(pkt->name));
		dwCount = pkt->count;
		cItemType = static_cast<char>(pkt->item_type);
		cEquipPos = static_cast<char>(pkt->equip_pos);
		bIsEquipped = (pkt->is_equipped != 0);
		sLevelLimit = static_cast<short>(pkt->level_limit);
		cGenderLimit = static_cast<char>(pkt->gender_limit);
		wCurLifeSpan = pkt->cur_lifespan;
		wWeight = pkt->weight;
		sSprite = static_cast<short>(pkt->sprite);
		sSpriteFrame = static_cast<short>(pkt->sprite_frame);
		cItemColor = static_cast<char>(pkt->item_color);
		sItemEffectValue2 = static_cast<short>(pkt->item_effect_value2);
		dwAttribute = pkt->attribute;
		sItemSpecEffectValue2 = static_cast<short>(pkt->spec_effect_value2);

		std::string cStr1;


		char cStr2[64], cStr3[64];
		cStr1 = cName;
		cStr2[0] = 0;
		cStr3[0] = 0;

		if (pGame->m_pBankList[cIndex] == 0) {
			pGame->m_pBankList[cIndex] = std::make_unique<CItem>();
			pGame->m_pBankList[cIndex]->m_sIDnum = static_cast<short>(pkt->item_id);
			pGame->m_pBankList[cIndex]->m_dwCount = dwCount;
			pGame->m_pBankList[cIndex]->m_wCurLifeSpan = wCurLifeSpan;
			pGame->m_pBankList[cIndex]->m_cItemColor = cItemColor;
			pGame->m_pBankList[cIndex]->m_dwAttribute = dwAttribute;
			pGame->m_pBankList[cIndex]->m_sItemSpecEffectValue2 = sItemSpecEffectValue2;

			if (dwCount == 1) cTxt = std::format(NOTIFYMSG_ITEMTOBANK3, cStr1.c_str());
			else cTxt = std::format(NOTIFYMSG_ITEMTOBANK2, dwCount, cStr1.c_str());

			if (pGame->m_dialogBoxManager.IsEnabled(DialogBoxId::Bank) == true)
				pGame->m_dialogBoxManager.Info(DialogBoxId::Bank).sView = hb::shared::limits::MaxBankItems - 12;
			pGame->AddEventList(cTxt.c_str(), 10);
		}
	}

	void HandleCannotItemToBank(CGame* pGame, char* pData)
	{
		pGame->AddEventList(NOTIFY_MSG_HANDLER63, 10);
	}
} // namespace NetworkMessageHandlers


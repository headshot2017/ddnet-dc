/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "killmessages.h"

static float Scale = 1.5f;

void CKillMessages::OnReset()
{
	m_KillmsgCurrent = 0;
	for(int i = 0; i < MAX_KILLMSGS; i++)
		m_aKillmsgs[i].m_Tick = -100000;
}

void CKillMessages::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;

		// unpack messages
		CKillMsg Kill;
		Kill.m_VictimID = pMsg->m_Victim;
		Kill.m_VictimTeam = m_pClient->m_aClients[Kill.m_VictimID].m_Team;
		Kill.m_VictimDDTeam = m_pClient->m_Teams.Team(Kill.m_VictimID);
		str_copy(Kill.m_aVictimName, m_pClient->m_aClients[Kill.m_VictimID].m_aName, sizeof(Kill.m_aVictimName));
		Kill.m_VictimRenderInfo = m_pClient->m_aClients[Kill.m_VictimID].m_RenderInfo;
		Kill.m_KillerID = pMsg->m_Killer;
		Kill.m_KillerTeam = m_pClient->m_aClients[Kill.m_KillerID].m_Team;
		str_copy(Kill.m_aKillerName, m_pClient->m_aClients[Kill.m_KillerID].m_aName, sizeof(Kill.m_aKillerName));
		Kill.m_KillerRenderInfo = m_pClient->m_aClients[Kill.m_KillerID].m_RenderInfo;
		Kill.m_Weapon = pMsg->m_Weapon;
		Kill.m_ModeSpecial = pMsg->m_ModeSpecial;
		Kill.m_Tick = Client()->GameTick();

		Kill.m_VictimRenderInfo.m_Size *= Scale;
		Kill.m_KillerRenderInfo.m_Size *= Scale;

		// add the message
		m_KillmsgCurrent = (m_KillmsgCurrent+1)%MAX_KILLMSGS;
		m_aKillmsgs[m_KillmsgCurrent] = Kill;
	}
}

void CKillMessages::OnRender()
{
	if (!g_Config.m_ClShowKillMessages)
		return;

	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width*1.5f, Height*1.5f);
	float StartX = Width*1.5f-10.0f;
	float y = 20.0f;
	if(g_Config.m_ClShowfps)
		y = 150.0f;

	for(int i = 1; i <= MAX_KILLMSGS; i++)
	{
		int r = (m_KillmsgCurrent+i)%MAX_KILLMSGS;
		if(Client()->GameTick() > m_aKillmsgs[r].m_Tick+50*10)
			continue;

		float FontSize = 36.0f*Scale;
		float KillerNameW = TextRender()->TextWidth(0, FontSize, m_aKillmsgs[r].m_aKillerName, -1);
		float VictimNameW = TextRender()->TextWidth(0, FontSize, m_aKillmsgs[r].m_aVictimName, -1);

		float x = StartX;

		// render victim name
		x -= VictimNameW;
		if(m_aKillmsgs[r].m_VictimID >= 0 && g_Config.m_ClChatTeamColors && m_aKillmsgs[r].m_VictimDDTeam)
		{
			vec3 rgb = HslToRgb(vec3(m_aKillmsgs[r].m_VictimDDTeam / 64.0f, 1.0f, 0.75f));
			TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0);
		}
		TextRender()->Text(0, x, y, FontSize, m_aKillmsgs[r].m_aVictimName, -1);
		TextRender()->TextColor(1.0, 1.0, 1.0, 1.0);

		// render victim tee
		x -= 24.0f*Scale;

		if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS)
		{
			if(m_aKillmsgs[r].m_ModeSpecial&1)
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				if(m_aKillmsgs[r].m_VictimTeam == TEAM_RED)
					RenderTools()->SelectSprite(SPRITE_FLAG_BLUE);
				else
					RenderTools()->SelectSprite(SPRITE_FLAG_RED);

				float Size = 56.0f*Scale;
				IGraphics::CQuadItem QuadItem(x, y-16, Size/2, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}

		RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aKillmsgs[r].m_VictimRenderInfo, EMOTE_PAIN, vec2(-1,0), vec2(x, y+28*Scale));
		x -= 32.0f*Scale;

		// render weapon
		x -= 44.0f*Scale;
		if (m_aKillmsgs[r].m_Weapon >= 0)
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();
			RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_aKillmsgs[r].m_Weapon].m_pSpriteBody);
			RenderTools()->DrawSprite(x, y+28*Scale, 96*Scale);
			Graphics()->QuadsEnd();
		}
		x -= 52.0f*Scale;

		if(m_aKillmsgs[r].m_VictimID != m_aKillmsgs[r].m_KillerID)
		{
			if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS)
			{
				if(m_aKillmsgs[r].m_ModeSpecial&2)
				{
					Graphics()->BlendNormal();
					Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
					Graphics()->QuadsBegin();

					if(m_aKillmsgs[r].m_KillerTeam == TEAM_RED)
						RenderTools()->SelectSprite(SPRITE_FLAG_BLUE, SPRITE_FLAG_FLIP_X);
					else
						RenderTools()->SelectSprite(SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

					float Size = 56.0f*Scale;
					IGraphics::CQuadItem QuadItem(x-56*Scale, y-16*Scale, Size/2, Size);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}

			// render killer tee
			x -= 24.0f*Scale;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &m_aKillmsgs[r].m_KillerRenderInfo, EMOTE_ANGRY, vec2(1,0), vec2(x, y+28*Scale));
			x -= 32.0f*Scale;

			// render killer name
			x -= KillerNameW;
			TextRender()->Text(0, x, y, FontSize, m_aKillmsgs[r].m_aKillerName, -1);
		}

		y += 46.0f*Scale;
	}
}

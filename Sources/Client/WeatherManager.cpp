// MapData.h must come before WeatherManager.h — it pulls in GlobalDef.h which
// sets GLOBALDEF_H_RESOLUTION_FUNCTIONS, preventing RenderConstants.h from
// redefining LOGICAL_WIDTH/HEIGHT when IRenderer.h is included below.
#include "MapData.h"
#include "WeatherManager.h"
#include "IRenderer.h"
#include "SpriteCollection.h"
#include "Camera.h"
#include "AudioManager.h"
#include "Misc.h"
#include "CommonTypes.h"

#include <cstdlib>

WeatherManager& WeatherManager::Get()
{
	static WeatherManager instance;
	return instance;
}

void WeatherManager::Initialize()
{
	m_is_active = false;
	m_effect_type = 0;
	m_weather_status = 0;
	m_last_update_time = 0;
	m_is_xmas = false;
	m_ambient_light_level = 1;
	ResetParticles();
}

void WeatherManager::Shutdown()
{
	m_renderer = nullptr;
	m_effect_sprites = nullptr;
	m_camera = nullptr;
	m_map_data = nullptr;
}

void WeatherManager::SetDependencies(hb::shared::render::IRenderer& renderer,
                                     hb::shared::sprite::SpriteCollection& effect_sprites,
                                     CCamera& camera)
{
	m_renderer = &renderer;
	m_effect_sprites = &effect_sprites;
	m_camera = &camera;
}

void WeatherManager::SetMapData(CMapData* map_data)
{
	m_map_data = map_data;
}

void WeatherManager::ResetParticles()
{
	for (auto& p : m_particles)
	{
		p.sX = 0;
		p.sBX = 0;
		p.sY = 0;
		p.cStep = 0;
	}
}

void WeatherManager::Draw()
{
	if (!m_effect_sprites || !m_camera) return;

	constexpr int MaxSnowAccum = 1000;
	static int ix1[MaxSnowAccum];
	static int iy2[MaxSnowAccum];
	static int iFrame[MaxSnowAccum];
	static int iNum = 0;
	int i;
	short dX, dY, sCnt;
	char cTempFrame;

	switch (m_effect_type) {
	case 1:
	case 2:
	case 3: // rain
		switch (m_effect_type) {
		case 1: sCnt = game_limits::max_weather_objects / 5; break;
		case 2:	sCnt = game_limits::max_weather_objects / 2; break;
		case 3:	sCnt = game_limits::max_weather_objects;     break;
		}

		for (i = 0; i < sCnt; i++)
		{
			if ((m_particles[i].cStep >= 0) && (m_particles[i].cStep < 20) && (m_particles[i].sX != 0))
			{
				dX = m_particles[i].sX - m_camera->GetX();
				dY = m_particles[i].sY - m_camera->GetY();
				cTempFrame = 16 + (m_particles[i].cStep / 6);
				(*m_effect_sprites)[11]->Draw(dX, dY, cTempFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));
			}
			else if ((m_particles[i].cStep >= 20) && (m_particles[i].cStep < 25) && (m_particles[i].sX != 0))
			{
				dX = m_particles[i].sX - m_camera->GetX();
				dY = m_particles[i].sY - m_camera->GetY();
				(*m_effect_sprites)[11]->Draw(dX, dY, m_particles[i].cStep, hb::shared::sprite::DrawParams::Alpha(0.5f));
			}
		}
		break;

	case 4:
	case 5:
	case 6: // Snow
		switch (m_effect_type) {
		case 4: sCnt = game_limits::max_weather_objects / 5; break;
		case 5:	sCnt = game_limits::max_weather_objects / 2; break;
		case 6:	sCnt = game_limits::max_weather_objects;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			if ((m_particles[i].cStep >= 0) && (m_particles[i].cStep < 80))
			{
				dX = m_particles[i].sX - m_camera->GetX();
				dY = m_particles[i].sY - m_camera->GetY();

				// Snoopy: Snow on lower bar
				if (dY >= 460)
				{
					cTempFrame = 39 + (m_particles[i].cStep / 20) * 3;
					dX = m_particles[i].sBX;
					dY = 426;
				}
				else cTempFrame = 39 + (m_particles[i].cStep / 20) * 3 + (rand() % 3);

				(*m_effect_sprites)[11]->Draw(dX, dY, cTempFrame, hb::shared::sprite::DrawParams::Alpha(0.5f));

				if (m_is_xmas == true)
				{
					if (dY == 478 - 53)
					{
						ix1[iNum] = dX;
						iy2[iNum] = dY + (rand() % 5);
						iFrame[iNum] = cTempFrame;
						iNum++;
					}
					if (iNum >= MaxSnowAccum) iNum = 0;
				}
			}
		}
		if (m_is_xmas == true)
		{
			for (i = 0; i <= MaxSnowAccum; i++)
			{
				if (iy2[i] > 10) (*m_effect_sprites)[11]->Draw(ix1[i], iy2[i], iFrame[i], hb::shared::sprite::DrawParams::Alpha(0.5f));
			}
		}
		break;
	}
}

void WeatherManager::Update(uint32_t current_time)
{
	if (!m_map_data || !m_camera) return;

	int i;
	short sCnt;
	char  cAdd;

	if ((current_time - m_last_update_time) < 30) return;
	m_last_update_time = current_time;

	switch (m_effect_type) {
	case 1:
	case 2:
	case 3: // Rain
		switch (m_effect_type) {
		case 1: sCnt = game_limits::max_weather_objects / 5; break;
		case 2:	sCnt = game_limits::max_weather_objects / 2; break;
		case 3:	sCnt = game_limits::max_weather_objects;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			m_particles[i].cStep++;
			if ((m_particles[i].cStep >= 0) && (m_particles[i].cStep < 20))
			{
				cAdd = (40 - m_particles[i].cStep);
				if (cAdd < 0) cAdd = 0;
				m_particles[i].sY = m_particles[i].sY + cAdd;
				if (cAdd != 0)
					m_particles[i].sX = m_particles[i].sX - 1;
			}
			else if (m_particles[i].cStep >= 25)
			{
				if (m_is_active == false)
				{
					m_particles[i].sX = 0;
					m_particles[i].sY = 0;
					m_particles[i].cStep = 30;
				}
				else
				{
					m_particles[i].sX = (m_map_data->m_sPivotX * 32) + ((rand() % 940) - 200) + 300;
					m_particles[i].sY = (m_map_data->m_sPivotY * 32) + ((rand() % LOGICAL_WIDTH()) - LOGICAL_HEIGHT()) + 240;
					m_particles[i].cStep = -1 * (rand() % 10);
				}
			}
		}
		break;

	case 4:
	case 5:
	case 6:
		switch (m_effect_type) {
		case 4: sCnt = game_limits::max_weather_objects / 5; break;
		case 5:	sCnt = game_limits::max_weather_objects / 2; break;
		case 6:	sCnt = game_limits::max_weather_objects;     break;
		}
		for (i = 0; i < sCnt; i++)
		{
			m_particles[i].cStep++;
			if ((m_particles[i].cStep >= 0) && (m_particles[i].cStep < 80))
			{
				cAdd = (80 - m_particles[i].cStep) / 10;
				if (cAdd < 0) cAdd = 0;
				m_particles[i].sY = m_particles[i].sY + cAdd;

				//Snoopy: Snow on lower bar
				if (m_particles[i].sY > (426 + m_camera->GetY()))
				{
					m_particles[i].sY = 470 + m_camera->GetY();
					if ((rand() % 10) != 2) m_particles[i].cStep--;
					if (m_particles[i].sBX == 0) m_particles[i].sBX = m_particles[i].sX - m_camera->GetX();

				}
				else m_particles[i].sX += 1 - (rand() % 3);
			}
			else if (m_particles[i].cStep >= 80)
			{
				if (m_is_active == false)
				{
					m_particles[i].sX = 0;
					m_particles[i].sY = 0;
					m_particles[i].sBX = 0;
					m_particles[i].cStep = 80;
				}
				else
				{
					m_particles[i].sX = (m_map_data->m_sPivotX * 32) + ((rand() % 940) - 200) + 300;
					m_particles[i].sY = (m_map_data->m_sPivotY * 32) + ((rand() % LOGICAL_WIDTH()) - LOGICAL_HEIGHT()) + LOGICAL_HEIGHT();
					m_particles[i].cStep = -1 * (rand() % 10);
					m_particles[i].sBX = 0;
				}
			}
		}
		break;
	}
}

void WeatherManager::SetAmbientLight(char level)
{
	m_ambient_light_level = level;
	if (m_renderer)
		m_renderer->SetAmbientLightLevel(level);
}

void WeatherManager::SetWeather(bool start, char effect_type)
{
	// Always stop weather sounds first when changing weather
	AudioManager::Get().StopSound(SoundType::Effect, 38);

	if (start == true)
	{
		m_is_active = true;
		m_effect_type = effect_type;

		// Rain sound (types 1-3)
		if (AudioManager::Get().IsSoundEnabled() && (effect_type >= 1) && (effect_type <= 3))
			AudioManager::Get().PlaySoundLoop(SoundType::Effect, 38);

		for (auto& p : m_particles)
		{
			p.sX = 1;
			p.sBX = 1;
			p.sY = 1;
			p.cStep = -1 * (rand() % 40);
		}
	}
	else
	{
		m_is_active = false;
		m_effect_type = 0;
	}
}

void WeatherManager::DrawThunderEffect(int sX, int sY, int dX, int dY, int rX, int rY, char cType)
{
	if (!m_renderer || !m_effect_sprites) return;

	int j, iErr, pX1, pY1, iX1, iY1, tX, tY;
	char cDir;
	pX1 = iX1 = tX = sX;
	pY1 = iY1 = tY = sY;

	for (j = 0; j < 100; j++)
	{
		switch (cType) {
		case 1:
			m_renderer->DrawLine(pX1, pY1, iX1, iY1, hb::shared::render::Color(15, 15, 20), hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 - 1, pY1, iX1 - 1, iY1, GameColors::NightBlueMid, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 + 1, pY1, iX1 + 1, iY1, GameColors::NightBlueMid, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1, pY1 - 1, iX1, iY1 - 1, GameColors::NightBlueMid, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1, pY1 + 1, iX1, iY1 + 1, GameColors::NightBlueMid, hb::shared::render::BlendMode::Additive);

			m_renderer->DrawLine(pX1 - 2, pY1, iX1 - 2, iY1, GameColors::NightBlueDark, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 + 2, pY1, iX1 + 2, iY1, GameColors::NightBlueDark, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1, pY1 - 2, iX1, iY1 - 2, GameColors::NightBlueDark, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1, pY1 + 2, iX1, iY1 + 2, GameColors::NightBlueDark, hb::shared::render::BlendMode::Additive);

			m_renderer->DrawLine(pX1 - 1, pY1 - 1, iX1 - 1, iY1 - 1, GameColors::NightBlueDeep, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 + 1, pY1 - 1, iX1 + 1, iY1 - 1, GameColors::NightBlueDeep, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 + 1, pY1 - 1, iX1 + 1, iY1 - 1, GameColors::NightBlueDeep, hb::shared::render::BlendMode::Additive);
			m_renderer->DrawLine(pX1 - 1, pY1 + 1, iX1 - 1, iY1 + 1, GameColors::NightBlueDeep, hb::shared::render::BlendMode::Additive);
			break;

		case 2:
			m_renderer->DrawLine(pX1, pY1, iX1, iY1, hb::shared::render::Color(GameColors::NightBlueBright.r, GameColors::NightBlueBright.g, GameColors::NightBlueBright.b, 128), hb::shared::render::BlendMode::Additive);
			break;
		}
		iErr = 0;
		CMisc::GetPoint(sX, sY, dX, dY, &tX, &tY, &iErr, j * 10);
		pX1 = iX1;
		pY1 = iY1;
		cDir = CMisc::cGetNextMoveDir(iX1, iY1, tX, tY);
		switch (cDir) {
		case 1:	rY -= 5; break;
		case 2: rY -= 5; rX += 5; break;
		case 3:	rX += 5; break;
		case 4: rX += 5; rY += 5; break;
		case 5: rY += 5; break;
		case 6: rX -= 5; rY += 5; break;
		case 7: rX -= 5; break;
		case 8: rX -= 5; rY -= 5; break;
		}
		if (rX < -20) rX = -20;
		if (rX > 20) rX = 20;
		if (rY < -20) rY = -20;
		if (rY > 20) rY = 20;
		iX1 = iX1 + rX;
		iY1 = iY1 + rY;
		if ((abs(tX - dX) < 5) && (abs(tY - dY) < 5)) break;
	}
	switch (cType) {
	case 1:
		(*m_effect_sprites)[6]->Draw(iX1, iY1, (rand() % 2), hb::shared::sprite::DrawParams::Alpha(0.75f));
		break;
	}
}

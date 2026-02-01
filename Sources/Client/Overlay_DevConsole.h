#pragma once

#include "IGameScreen.h"

class Overlay_DevConsole : public IGameScreen
{
public:
	SCREEN_TYPE(Overlay_DevConsole)

	explicit Overlay_DevConsole(CGame* pGame);
	~Overlay_DevConsole() override = default;

	bool wants_background_dim() const override { return false; }

	void on_initialize() override;
	void on_uninitialize() override;
	void on_update() override;
	void on_render() override;
};

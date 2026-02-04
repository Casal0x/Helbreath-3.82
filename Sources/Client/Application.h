#pragma once
#include <memory>
#include "NativeTypes.h"

class CGame;
class GameWindowHandler;

class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool initialize(NativeInstance instance, int icon_resource_id, const char* cmd_line);
    int run();

private:
    void _shutdown();
    void _event_loop();

    std::unique_ptr<CGame> game_;
    std::unique_ptr<GameWindowHandler> window_handler_;
};

#include "application.hpp"

int main(int argc, char* argv[])
{
    Application app;
    app.SetupWindow();
    app.SetupGraphics();
    app.Run();
    return 0;
}
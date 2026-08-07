#include "teleprompter-window.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QPointer>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "A Markdown teleprompter window for OBS Studio";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "OBS Teleprompter";
}

static QPointer<TeleprompterWindow> teleprompter;

bool obs_module_load(void)
{
	auto *action = static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Teleprompter"));
	QObject::connect(action, &QAction::triggered, [] {
		if (!teleprompter) {
			auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
			teleprompter = new TeleprompterWindow(mainWindow);
		}
		teleprompter->show();
		teleprompter->raise();
		teleprompter->activateWindow();
	});

	blog(LOG_INFO, "OBS Teleprompter %s loaded", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	if (teleprompter)
		delete teleprompter;
	teleprompter = nullptr;
}

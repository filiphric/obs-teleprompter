#include "teleprompter-window.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QAction>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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
static QPointer<QWidget> teleprompterDock;

static void openTeleprompter()
{
	if (!teleprompter) {
		auto *mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
		teleprompter = new TeleprompterWindow(mainWindow);
	}
	teleprompter->show();
	teleprompter->raise();
	teleprompter->activateWindow();
}

bool obs_module_load(void)
{
	auto *action = static_cast<QAction *>(
		obs_frontend_add_tools_menu_qaction(obs_module_text("Teleprompter")));
	QObject::connect(action, &QAction::triggered, openTeleprompter);

	teleprompterDock = new QWidget;
	auto *dockLayout = new QVBoxLayout(teleprompterDock);
	dockLayout->setContentsMargins(8, 8, 8, 8);
	auto *openButton = new QPushButton(obs_module_text("OpenTeleprompter"), teleprompterDock);
	dockLayout->addWidget(openButton);
	dockLayout->addStretch();
	QObject::connect(openButton, &QPushButton::clicked, openTeleprompter);

	if (!obs_frontend_add_dock_by_id("obs-teleprompter", obs_module_text("Teleprompter"),
		teleprompterDock)) {
		blog(LOG_WARNING, "Could not add the Teleprompter dock");
		delete teleprompterDock;
	}

	blog(LOG_INFO, "OBS Teleprompter %s loaded", PLUGIN_VERSION);
	return true;
}

void obs_module_unload(void)
{
	if (teleprompterDock)
		obs_frontend_remove_dock("obs-teleprompter");
	teleprompterDock = nullptr;

	if (teleprompter)
		delete teleprompter;
	teleprompter = nullptr;
}

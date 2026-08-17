#include "teleprompter-window.hpp"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSettings>
#include <QTemporaryDir>

namespace {
bool writeScript(const QString &path, const QByteArray &contents)
{
	QFile file(path);
	return file.open(QIODevice::WriteOnly | QIODevice::Text) && file.write(contents) == contents.size();
}
}

int main(int argc, char *argv[])
{
	QApplication application(argc, argv);
	application.setOrganizationName(QStringLiteral("OBS Teleprompter Screenshot"));
	application.setApplicationName(QStringLiteral("README Preview"));
	if (argc != 2)
		return 2;

	QTemporaryDir scripts;
	if (!scripts.isValid())
		return 3;

	if (!writeScript(scripts.filePath(QStringLiteral("01 - Welcome.md")),
			"# Welcome to the show\n\nToday we'll learn how to speak naturally on camera—without losing our place.\n\n"
			"## Three things to remember\n\n- Look into the lens\n- Take your time\n- Make the story yours\n\n"
			"> You're ready. Take a breath, and let's begin.\n") ||
		!writeScript(scripts.filePath(QStringLiteral("02 - Product update.md")),
			"# Product update\n\nA short script for the weekly video.\n") ||
		!writeScript(scripts.filePath(QStringLiteral("03 - Closing.md")),
			"# Thanks for watching\n\nSee you next time.\n"))
		return 4;

	QSettings settings;
	settings.clear();
	settings.setValue(QStringLiteral("teleprompter/folder"), scripts.path());
	settings.setValue(QStringLiteral("teleprompter/speed"), 60);
	settings.setValue(QStringLiteral("teleprompter/margins"), 80);
	settings.setValue(QStringLiteral("teleprompter/fontSize"), 32);
	settings.sync();

	TeleprompterWindow window;
	window.show();
	application.processEvents();
	for (QLabel *label : window.findChildren<QLabel *>()) {
		if (label->text() == scripts.path())
			label->setText(QStringLiteral("Demo scripts"));
	}
	application.processEvents();
	const bool saved = window.grab().save(QDir::cleanPath(QString::fromLocal8Bit(argv[1])), "PNG");
	window.close();
	settings.clear();
	return saved ? 0 : 5;
}

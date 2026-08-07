#pragma once

#include <QDialog>
#include <QElapsedTimer>

class QLabel;
class QListWidget;
class QComboBox;
class QSlider;
class QPlainTextEdit;
class QStackedWidget;
class QShortcut;
class QSplitter;
class QVBoxLayout;
class QResizeEvent;
class QPushButton;
class QTextBrowser;
class QTimer;
class QWidget;

class TeleprompterWindow final : public QDialog {
	Q_OBJECT

public:
	explicit TeleprompterWindow(QWidget *parent = nullptr);

protected:
	void closeEvent(QCloseEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private slots:
	void chooseFolder();
	void loadSelectedScript();
	void toggleScrolling();
	void scrollFrame();
	void increaseSpeed();
	void decreaseSpeed();
	void jumpUp();
	void jumpDown();
	void updateMargins(int pixels);
	void changeSortOrder();
	void increaseMargins();
	void decreaseMargins();
	void exitFullscreen();
	void saveCurrentScript();
	void startFullscreen();
	void updateFontSize(int pixels);
	void increaseFontSize();
	void decreaseFontSize();

private:
	void loadFolder(const QString &path);
	void setScrolling(bool active);
	void updateSpeedLabel();
	void applyDocumentFormatting();
	void updateWindowMode();
	void renderEditorMarkdown();
	void positionFullscreenOverlays();
	void saveSettings() const;

	QListWidget *scriptList = nullptr;
	QTextBrowser *scriptView = nullptr;
	QPlainTextEdit *markdownEditor = nullptr;
	QStackedWidget *contentStack = nullptr;
	QPushButton *folderButton = nullptr;
	QPushButton *playButton = nullptr;
	QPushButton *saveButton = nullptr;
	QLabel *folderLabel = nullptr;
	QLabel *speedLabel = nullptr;
	QSlider *marginInput = nullptr;
	QLabel *marginValueLabel = nullptr;
	QSlider *fontSizeInput = nullptr;
	QLabel *fontSizeValueLabel = nullptr;
	QComboBox *sortInput = nullptr;
	QTimer *scrollTimer = nullptr;
	QWidget *controlsWidget = nullptr;
	QWidget *sidebarWidget = nullptr;
	QWidget *hintWidget = nullptr;
	QWidget *marginControlsWidget = nullptr;
	QLabel *hintLabel = nullptr;
	QShortcut *spaceShortcut = nullptr;
	QSplitter *mainSplitter = nullptr;
	QVBoxLayout *rootLayout = nullptr;
	QElapsedTimer elapsed;
	QString folderPath;
	QString currentFilePath;
	double speed = 60.0;
	double scrollRemainder = 0.0;
	bool scrolling = false;
	bool wasMaximizedBeforeRun = false;
	bool editorDirty = false;
	bool overlaysDetached = false;
};

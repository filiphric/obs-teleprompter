#include "teleprompter-window.hpp"

#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QSyntaxHighlighter>
#include <QTextBrowser>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTimer>
#include <QVBoxLayout>

namespace {
constexpr double MinSpeed = 10.0;
constexpr double MaxSpeed = 300.0;
constexpr double SpeedStep = 10.0;
constexpr int TimerIntervalMs = 16;
constexpr int MarginShortcutStep = 20;
constexpr int FontSizeShortcutStep = 2;

class MarkdownHighlighter final : public QSyntaxHighlighter {
public:
	explicit MarkdownHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) {}

protected:
	void highlightBlock(const QString &text) override
	{
		QTextCharFormat heading;
		heading.setFontWeight(QFont::Bold);
		heading.setForeground(QColor(QStringLiteral("#f1f1ef")));
		const QRegularExpression headingExpression(QStringLiteral("^(#{1,6})\\s+.*$"));
		const auto headingMatch = headingExpression.match(text);
		if (headingMatch.hasMatch()) {
			const int level = headingMatch.captured(1).size();
			heading.setFontPointSize(qMax(17, 28 - (level - 1) * 2));
			setFormat(headingMatch.capturedStart(), headingMatch.capturedLength(), heading);
			QTextCharFormat marker;
			marker.setForeground(QColor(QStringLiteral("#777570")));
			setFormat(headingMatch.capturedStart(1), headingMatch.capturedLength(1), marker);
		}

		applyExpression(text, QRegularExpression(QStringLiteral("\\*\\*([^*]+)\\*\\*")), boldFormat());
		applyExpression(text, QRegularExpression(QStringLiteral("(?<!\\*)\\*([^*]+)\\*(?!\\*)")), italicFormat());
		applyExpression(text, QRegularExpression(QStringLiteral("`([^`]+)`")), codeFormat());
		applyExpression(text, QRegularExpression(QStringLiteral("\\[[^\\]]+\\]\\([^)]+\\)")), linkFormat());

		const QRegularExpression quoteExpression(QStringLiteral("^\\s*>\\s+.*$"));
		const auto quoteMatch = quoteExpression.match(text);
		if (quoteMatch.hasMatch()) {
			QTextCharFormat quote;
			quote.setForeground(QColor(QStringLiteral("#b4b4b0")));
			quote.setFontItalic(true);
			setFormat(quoteMatch.capturedStart(), quoteMatch.capturedLength(), quote);
		}

		const QRegularExpression listExpression(QStringLiteral("^(\\s*(?:[-+*]|\\d+\\.))\\s+"));
		const auto listMatch = listExpression.match(text);
		if (listMatch.hasMatch()) {
			QTextCharFormat marker;
			marker.setForeground(QColor(QStringLiteral("#9b9a97")));
			marker.setFontWeight(QFont::DemiBold);
			setFormat(listMatch.capturedStart(1), listMatch.capturedLength(1), marker);
		}
	}

private:
	void applyExpression(const QString &text, const QRegularExpression &expression,
		const QTextCharFormat &format)
	{
		auto matches = expression.globalMatch(text);
		while (matches.hasNext()) {
			const auto match = matches.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	QTextCharFormat boldFormat() const
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(QColor(QStringLiteral("#f1f1ef")));
		return format;
	}

	QTextCharFormat italicFormat() const
	{
		QTextCharFormat format;
		format.setFontItalic(true);
		return format;
	}

	QTextCharFormat codeFormat() const
	{
		QTextCharFormat format;
		format.setFontFamilies({QStringLiteral("SF Mono"), QStringLiteral("Menlo")});
		format.setBackground(QColor(QStringLiteral("#2c2c2a")));
		format.setForeground(QColor(QStringLiteral("#eb5757")));
		return format;
	}

	QTextCharFormat linkFormat() const
	{
		QTextCharFormat format;
		format.setForeground(QColor(QStringLiteral("#529cca")));
		format.setFontUnderline(true);
		return format;
	}
};
}

TeleprompterWindow::TeleprompterWindow(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(tr("OBS Teleprompter"));
	setWindowFlag(Qt::Window, true);
	setAttribute(Qt::WA_DeleteOnClose, false);
	resize(1100, 760);

	folderButton = new QPushButton(tr("Choose scripts folder…"), this);
	playButton = new QPushButton(tr("▶ Start"), this);
	saveButton = new QPushButton(tr("Save Markdown"), this);
	saveButton->setEnabled(false);
	folderLabel = new QLabel(tr("No folder selected"), this);
	folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	speedLabel = new QLabel(this);
	marginInput = new QSlider(Qt::Horizontal, this);
	marginInput->setRange(0, 1000);
	marginInput->setSingleStep(10);
	marginInput->setPageStep(50);
	marginInput->setMinimumWidth(160);
	marginInput->setToolTip(tr("Horizontal margin on each side"));
	marginValueLabel = new QLabel(this);
	marginValueLabel->setMinimumWidth(55);
	fontSizeInput = new QSlider(Qt::Horizontal, this);
	fontSizeInput->setRange(20, 96);
	fontSizeInput->setSingleStep(FontSizeShortcutStep);
	fontSizeInput->setPageStep(8);
	fontSizeInput->setMinimumWidth(130);
	fontSizeInput->setToolTip(tr("Fullscreen teleprompter font size"));
	fontSizeValueLabel = new QLabel(this);
	fontSizeValueLabel->setMinimumWidth(55);
	sortInput = new QComboBox(this);
	sortInput->addItem(tr("Name"), QStringLiteral("name"));
	sortInput->addItem(tr("Last modified"), QStringLiteral("date"));
	sortInput->setToolTip(tr("Sort scripts in the selected folder"));

	controlsWidget = new QWidget(this);
	auto *controls = new QHBoxLayout(controlsWidget);
	controls->setContentsMargins(0, 0, 0, 0);
	controls->addStretch(1);
	controls->addWidget(saveButton);
	controls->addWidget(playButton);
	controls->addWidget(speedLabel);
	marginControlsWidget = new QWidget(controlsWidget);
	auto *marginControls = new QHBoxLayout(marginControlsWidget);
	marginControls->setContentsMargins(0, 0, 0, 0);
	marginControls->addWidget(new QLabel(tr("Margins:"), marginControlsWidget));
	marginControls->addWidget(marginInput);
	marginControls->addWidget(marginValueLabel);
	marginControls->addSpacing(12);
	marginControls->addWidget(new QLabel(tr("Text:"), marginControlsWidget));
	marginControls->addWidget(fontSizeInput);
	marginControls->addWidget(fontSizeValueLabel);
	controls->addWidget(marginControlsWidget);
	controls->addStretch(1);

	scriptList = new QListWidget(this);
	scriptList->setMinimumWidth(220);
	scriptList->setAlternatingRowColors(true);

	sidebarWidget = new QWidget(this);
	auto *sidebar = new QVBoxLayout(sidebarWidget);
	sidebar->setContentsMargins(0, 0, 0, 0);
	auto *sortRow = new QHBoxLayout;
	sortRow->addWidget(new QLabel(tr("Sort:"), sidebarWidget));
	sortRow->addWidget(sortInput, 1);
	sidebar->addLayout(sortRow);
	sidebar->addWidget(scriptList, 1);
	folderLabel->setWordWrap(true);
	folderLabel->setToolTip(tr("Current scripts folder"));
	sidebar->addWidget(folderLabel);
	sidebar->addWidget(folderButton);

	scriptView = new QTextBrowser(this);
	scriptView->setOpenExternalLinks(true);
	scriptView->setFocusPolicy(Qt::StrongFocus);
	scriptView->setStyleSheet(
		"QTextBrowser { background: #111; color: #f5f5f5; border: 0; "
		"font-size: 32px; line-height: 1.45; selection-background-color: #555; }");
	scriptView->setPlaceholderText(
		tr("Choose a folder containing Markdown (.md) scripts, then select a script."));
	markdownEditor = new QPlainTextEdit(this);
	markdownEditor->setPlaceholderText(tr("Select a Markdown file to edit it."));
	markdownEditor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
	markdownEditor->setFrameShape(QFrame::NoFrame);
	QFont editorFont(QStringLiteral("SF Pro Text"));
	editorFont.setPointSize(17);
	markdownEditor->setFont(editorFont);
	markdownEditor->setTabStopDistance(QFontMetricsF(editorFont).horizontalAdvance(QLatin1Char(' ')) * 4);
	markdownEditor->setStyleSheet(
		"QPlainTextEdit { background: #191919; color: #e3e3e1; border: 0; "
		"font-family: 'SF Pro Text'; font-size: 17px; padding: 42px 72px; "
		"selection-background-color: #2f5d7c; selection-color: #ffffff; }");
	new MarkdownHighlighter(markdownEditor->document());
	contentStack = new QStackedWidget(this);
	contentStack->addWidget(markdownEditor);
	contentStack->addWidget(scriptView);

	mainSplitter = new QSplitter(this);
	mainSplitter->addWidget(sidebarWidget);
	mainSplitter->addWidget(contentStack);
	mainSplitter->setStretchFactor(1, 1);
	mainSplitter->setSizes({240, 860});

	hintLabel = new QLabel(this);
	hintLabel->setAlignment(Qt::AlignCenter);
	hintWidget = hintLabel;

	rootLayout = new QVBoxLayout(this);
	rootLayout->addWidget(controlsWidget);
	rootLayout->addWidget(mainSplitter, 1);
	rootLayout->addWidget(hintLabel);

	scrollTimer = new QTimer(this);
	scrollTimer->setTimerType(Qt::PreciseTimer);
	scrollTimer->setInterval(TimerIntervalMs);

	connect(folderButton, &QPushButton::clicked, this, &TeleprompterWindow::chooseFolder);
	connect(playButton, &QPushButton::clicked, this, &TeleprompterWindow::toggleScrolling);
	connect(saveButton, &QPushButton::clicked, this, &TeleprompterWindow::saveCurrentScript);
	connect(scriptList, &QListWidget::currentItemChanged, this,
		&TeleprompterWindow::loadSelectedScript);
	connect(marginInput, &QSlider::valueChanged, this,
		&TeleprompterWindow::updateMargins);
	connect(fontSizeInput, &QSlider::valueChanged, this,
		&TeleprompterWindow::updateFontSize);
	connect(sortInput, qOverload<int>(&QComboBox::currentIndexChanged), this,
		&TeleprompterWindow::changeSortOrder);
	connect(scrollTimer, &QTimer::timeout, this, &TeleprompterWindow::scrollFrame);
	connect(markdownEditor, &QPlainTextEdit::textChanged, this, [this] {
		editorDirty = true;
		saveButton->setEnabled(!currentFilePath.isEmpty());
	});

	auto addShortcut = [this](const QKeySequence &key, auto slot) {
		auto *shortcut = new QShortcut(key, this);
		shortcut->setContext(Qt::WindowShortcut);
		connect(shortcut, &QShortcut::activated, this, slot);
	};
	spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
	spaceShortcut->setContext(Qt::WindowShortcut);
	connect(spaceShortcut, &QShortcut::activated, this, &TeleprompterWindow::toggleScrolling);
	addShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Return), &TeleprompterWindow::startFullscreen);
	addShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_Enter), &TeleprompterWindow::startFullscreen);
	addShortcut(QKeySequence(Qt::Key_Left), &TeleprompterWindow::decreaseSpeed);
	addShortcut(QKeySequence(Qt::Key_Right), &TeleprompterWindow::increaseSpeed);
	addShortcut(QKeySequence(Qt::Key_Up), &TeleprompterWindow::jumpUp);
	addShortcut(QKeySequence(Qt::Key_Down), &TeleprompterWindow::jumpDown);
	addShortcut(QKeySequence(Qt::Key_Escape), &TeleprompterWindow::exitFullscreen);
	addShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Left), &TeleprompterWindow::decreaseMargins);
	addShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Right), &TeleprompterWindow::increaseMargins);
	addShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Up), &TeleprompterWindow::increaseFontSize);
	addShortcut(QKeySequence(Qt::AltModifier | Qt::Key_Down), &TeleprompterWindow::decreaseFontSize);
	addShortcut(QKeySequence::Save, &TeleprompterWindow::saveCurrentScript);

	QSettings settings;
	const QString savedFolder = settings.value(QStringLiteral("teleprompter/folder")).toString();
	speed = settings.value(QStringLiteral("teleprompter/speed"), 60.0).toDouble();
	{
		const QSignalBlocker blocker(sortInput);
		const QString savedSort = settings.value(
			QStringLiteral("teleprompter/sort"), QStringLiteral("name")).toString();
		const int sortIndex = sortInput->findData(savedSort);
		sortInput->setCurrentIndex(sortIndex >= 0 ? sortIndex : 0);
	}
	{
		const QSignalBlocker blocker(marginInput);
		marginInput->setValue(settings.value(QStringLiteral("teleprompter/margins"), 80).toInt());
	}
	{
		const QSignalBlocker blocker(fontSizeInput);
		fontSizeInput->setValue(settings.value(QStringLiteral("teleprompter/fontSize"), 32).toInt());
	}
	updateMargins(marginInput->value());
	updateFontSize(fontSizeInput->value());
	updateSpeedLabel();
	loadFolder(savedFolder);
	setScrolling(false);
}

void TeleprompterWindow::chooseFolder()
{
	const QString selected = QFileDialog::getExistingDirectory(
		this, tr("Choose scripts folder"), folderPath);
	if (!selected.isEmpty())
		loadFolder(selected);
}

void TeleprompterWindow::loadFolder(const QString &path)
{
	if (path.isEmpty() || !QDir(path).exists())
		return;

	setScrolling(false);
	folderPath = path;
	folderLabel->setText(QDir::toNativeSeparators(path));
	scriptList->clear();

	QDir directory(path);
	const bool sortByDate = sortInput->currentData().toString() == QStringLiteral("date");
	const QDir::SortFlags sortFlags = sortByDate ? QDir::Time : (QDir::Name | QDir::IgnoreCase);
	const QFileInfoList files = directory.entryInfoList(
		{QStringLiteral("*.md"), QStringLiteral("*.markdown")}, QDir::Files | QDir::Readable,
		sortFlags);
	for (const QFileInfo &file : files) {
		auto *item = new QListWidgetItem(file.completeBaseName(), scriptList);
		item->setData(Qt::UserRole, file.absoluteFilePath());
		item->setToolTip(file.fileName());
	}

	if (scriptList->count() > 0)
		scriptList->setCurrentRow(0);
	else
		scriptView->setMarkdown(tr("# No Markdown scripts found\n\nAdd `.md` files to this folder."));
	saveSettings();
}

void TeleprompterWindow::loadSelectedScript()
{
	if (editorDirty)
		saveCurrentScript();

	QListWidgetItem *item = scriptList->currentItem();
	if (!item)
		return;

	setScrolling(false);
	const QString filePath = item->data(Qt::UserRole).toString();
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		currentFilePath.clear();
		scriptView->setPlainText(tr("Could not open this script."));
		markdownEditor->setPlainText(tr("Could not open this script."));
		return;
	}

	currentFilePath = filePath;
	const QSignalBlocker blocker(markdownEditor);
	markdownEditor->setPlainText(QString::fromUtf8(file.readAll()));
	editorDirty = false;
	saveButton->setEnabled(false);
	renderEditorMarkdown();
	scriptView->verticalScrollBar()->setValue(0);
}

void TeleprompterWindow::toggleScrolling()
{
	setScrolling(!scrolling);
}

void TeleprompterWindow::setScrolling(bool active)
{
	if (active && !isFullScreen()) {
		saveCurrentScript();
		renderEditorMarkdown();
	}
	scrolling = active;
	playButton->setText(scrolling ? tr("⏸ Pause") : tr("▶ Start"));
	playButton->setStyleSheet(scrolling
		? QStringLiteral("QPushButton { background: #e1ad24; color: #17120a; font-weight: 700; padding: 6px 16px; }")
		: QStringLiteral("QPushButton { background: #258c4b; color: white; font-weight: 700; padding: 6px 16px; }"));
	if (scrolling) {
		if (!isFullScreen()) {
			wasMaximizedBeforeRun = isMaximized();
			showFullScreen();
		}
		updateWindowMode();
		scrollRemainder = 0.0;
		elapsed.restart();
		scrollTimer->start();
	} else {
		scrollTimer->stop();
		updateWindowMode();
	}
}

void TeleprompterWindow::updateWindowMode()
{
	const bool fullscreen = isFullScreen();
	const bool showFullscreenPanels = fullscreen && !scrolling;
	if (fullscreen && !overlaysDetached) {
		rootLayout->removeWidget(controlsWidget);
		rootLayout->removeWidget(hintWidget);
		overlaysDetached = true;
		controlsWidget->setStyleSheet(
			QStringLiteral("background: #1b1b1b; color: #f5f5f5;"));
		controlsWidget->layout()->setContentsMargins(10, 8, 10, 8);
		hintWidget->setStyleSheet(QStringLiteral("background: #1b1b1b; color: #f5f5f5; padding: 8px;"));
		positionFullscreenOverlays();
	} else if (!fullscreen && overlaysDetached) {
		controlsWidget->setStyleSheet(QString());
		controlsWidget->layout()->setContentsMargins(0, 0, 0, 0);
		hintWidget->setStyleSheet(QString());
		rootLayout->insertWidget(0, controlsWidget);
		rootLayout->addWidget(hintWidget);
		overlaysDetached = false;
	}
	spaceShortcut->setEnabled(fullscreen);
	hintLabel->setText(fullscreen
		? tr("Space: start/pause   ←/→: speed   ↑/↓: jump   Option+←/→: margins   Option+↑/↓: text size")
		: tr("Cmd+Enter: start fullscreen   ←/→: slower/faster   ↑/↓: jump up/down"));
	controlsWidget->setVisible(!fullscreen || showFullscreenPanels);
	marginControlsWidget->setVisible(showFullscreenPanels);
	saveButton->setVisible(!fullscreen);
	sidebarWidget->setVisible(!fullscreen);
	hintWidget->setVisible(!fullscreen || showFullscreenPanels);
	scriptView->setVerticalScrollBarPolicy(fullscreen ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	rootLayout->setContentsMargins(fullscreen ? QMargins(0, 0, 0, 0) : QMargins(11, 11, 11, 11));
	contentStack->setCurrentWidget(fullscreen ? static_cast<QWidget *>(scriptView)
		: static_cast<QWidget *>(markdownEditor));
	updateMargins(marginInput->value());
	if (showFullscreenPanels) {
		positionFullscreenOverlays();
		controlsWidget->raise();
		hintWidget->raise();
	}
}

void TeleprompterWindow::positionFullscreenOverlays()
{
	if (!overlaysDetached)
		return;
	const int topHeight = controlsWidget->sizeHint().height();
	const int bottomHeight = hintWidget->sizeHint().height();
	controlsWidget->setGeometry(0, 0, width(), topHeight);
	hintWidget->setGeometry(0, qMax(0, height() - bottomHeight), width(), bottomHeight);
}

void TeleprompterWindow::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);
	positionFullscreenOverlays();
	if (isFullScreen())
		updateMargins(marginInput->value());
}

void TeleprompterWindow::startFullscreen()
{
	if (!isFullScreen())
		setScrolling(true);
}

void TeleprompterWindow::renderEditorMarkdown()
{
	if (currentFilePath.isEmpty())
		return;
	scriptView->document()->setBaseUrl(
		QUrl::fromLocalFile(QFileInfo(currentFilePath).absolutePath() + QLatin1Char('/')));
	scriptView->setMarkdown(markdownEditor->toPlainText());
	applyDocumentFormatting();
	updateMargins(marginInput->value());
}

void TeleprompterWindow::saveCurrentScript()
{
	if (!editorDirty || currentFilePath.isEmpty())
		return;

	QFile file(currentFilePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		saveButton->setText(tr("Save failed"));
		return;
	}
	file.write(markdownEditor->toPlainText().toUtf8());
	file.close();
	editorDirty = false;
	saveButton->setEnabled(false);
	saveButton->setText(tr("Save Markdown"));
}

void TeleprompterWindow::applyDocumentFormatting()
{
	for (QTextBlock block = scriptView->document()->begin(); block.isValid(); block = block.next()) {
		QTextBlockFormat format = block.blockFormat();
		format.setLineHeight(145, QTextBlockFormat::ProportionalHeight);
		format.setBottomMargin(block.textList() ? 7 : 32);
		if (format.headingLevel() > 0) {
			format.setTopMargin(24);
			format.setBottomMargin(20);
		}
		QTextCursor cursor(block);
		cursor.setBlockFormat(format);
	}
}

void TeleprompterWindow::scrollFrame()
{
	QScrollBar *bar = scriptView->verticalScrollBar();
	const double distance = speed * (elapsed.restart() / 1000.0) + scrollRemainder;
	const int pixels = static_cast<int>(distance);
	scrollRemainder = distance - pixels;
	bar->setValue(bar->value() + pixels);
	if (bar->value() >= bar->maximum())
		setScrolling(false);
}

void TeleprompterWindow::increaseSpeed()
{
	speed = qMin(MaxSpeed, speed + SpeedStep);
	updateSpeedLabel();
	saveSettings();
}

void TeleprompterWindow::decreaseSpeed()
{
	speed = qMax(MinSpeed, speed - SpeedStep);
	updateSpeedLabel();
	saveSettings();
}

void TeleprompterWindow::updateSpeedLabel()
{
	speedLabel->setText(tr("Speed: %1 px/s").arg(static_cast<int>(speed)));
}

void TeleprompterWindow::jumpUp()
{
	QScrollBar *bar = scriptView->verticalScrollBar();
	bar->setValue(bar->value() - qMax(80, scriptView->viewport()->height() / 3));
}

void TeleprompterWindow::jumpDown()
{
	QScrollBar *bar = scriptView->verticalScrollBar();
	bar->setValue(bar->value() + qMax(80, scriptView->viewport()->height() / 3));
}

void TeleprompterWindow::updateMargins(int pixels)
{
	marginValueLabel->setText(tr("%1 px").arg(pixels));
	const int appliedPixels = isFullScreen() ? pixels : 0;
	QTextFrameFormat format = scriptView->document()->rootFrame()->frameFormat();
	format.setLeftMargin(appliedPixels);
	format.setRightMargin(appliedPixels);
	format.setTopMargin(isFullScreen() ? scriptView->viewport()->height() / 2 : 0);
	format.setBottomMargin(isFullScreen() ? scriptView->viewport()->height() / 2 : 0);
	scriptView->document()->rootFrame()->setFrameFormat(format);
	saveSettings();
}

void TeleprompterWindow::updateFontSize(int pixels)
{
	fontSizeValueLabel->setText(tr("%1 px").arg(pixels));
	scriptView->setStyleSheet(QStringLiteral(
		"QTextBrowser { background: #111; color: #f5f5f5; border: 0; "
		"font-size: %1px; line-height: 1.45; selection-background-color: #555; }").arg(pixels));
	saveSettings();
}

void TeleprompterWindow::increaseFontSize()
{
	fontSizeInput->setValue(fontSizeInput->value() + FontSizeShortcutStep);
}

void TeleprompterWindow::decreaseFontSize()
{
	fontSizeInput->setValue(fontSizeInput->value() - FontSizeShortcutStep);
}

void TeleprompterWindow::increaseMargins()
{
	marginInput->setValue(marginInput->value() + MarginShortcutStep);
}

void TeleprompterWindow::decreaseMargins()
{
	marginInput->setValue(marginInput->value() - MarginShortcutStep);
}

void TeleprompterWindow::exitFullscreen()
{
	if (!isFullScreen())
		return;

	scrolling = false;
	scrollTimer->stop();
	playButton->setText(tr("▶ Start"));
	playButton->setStyleSheet(
		QStringLiteral("QPushButton { background: #258c4b; color: white; font-weight: 700; padding: 6px 16px; }"));
	if (wasMaximizedBeforeRun)
		showMaximized();
	else
		showNormal();
	updateWindowMode();
}

void TeleprompterWindow::changeSortOrder()
{
	const QString selectedPath = scriptList->currentItem()
		? scriptList->currentItem()->data(Qt::UserRole).toString()
		: QString();
	loadFolder(folderPath);
	for (int row = 0; row < scriptList->count(); ++row) {
		if (scriptList->item(row)->data(Qt::UserRole).toString() == selectedPath) {
			scriptList->setCurrentRow(row);
			break;
		}
	}
}

void TeleprompterWindow::saveSettings() const
{
	QSettings settings;
	settings.setValue(QStringLiteral("teleprompter/folder"), folderPath);
	settings.setValue(QStringLiteral("teleprompter/speed"), speed);
	settings.setValue(QStringLiteral("teleprompter/margins"), marginInput->value());
	settings.setValue(QStringLiteral("teleprompter/fontSize"), fontSizeInput->value());
	settings.setValue(QStringLiteral("teleprompter/sort"), sortInput->currentData());
}

void TeleprompterWindow::closeEvent(QCloseEvent *event)
{
	saveCurrentScript();
	setScrolling(false);
	saveSettings();
	QDialog::closeEvent(event);
}

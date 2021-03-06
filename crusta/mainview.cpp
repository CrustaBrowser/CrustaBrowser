/* ============================================================
* Crusta - Qt5 webengine browser
* Copyright (C) 2017-2018 Anmol Gautam <anmol@crustabrowser.com>
*
* THIS FILE IS A PART OF CRUSTA
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distrib   uted in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */

#include "mainview.h"
#include "privatemainview.h"
#include "lib/sidepanel/sidepane.h"

#include "lib/utils/themeeditor.h"
#include "lib/utils/siteinfo.h"
#include "lib/speeddial/speeddial.h"

#include <QFileDialog>
#include <QPrinter>
#include <QPageSetupDialog>
#include <QClipboard>
#include <QProcess>
#include <QSound>
#include <QSettings>

#include <iostream>
#include <memory>

QWebEngineView *MainView::getWebView() const {
    int index = tabWindow->currentIndex();
    QWidget *widget = tabWindow->widget(index);
    QLayout *layout = widget->layout();
    auto *webview = (QWebEngineView *)layout->itemAt(1)->widget();
    return webview;
}

QWebEngineView *MainView::getWebView(int index) const {
    QWidget* widget = tabWindow->widget(index);
    QLayout* layout = widget->layout();
    auto * webview = dynamic_cast<QWebEngineView *>(layout->itemAt(1)->widget());
    return webview;
}

void MainView::closeTab(int index)
{
    QWebEngineView *webview = getWebView(index);
    auto *hist = new QAction;
    hist->setText(webview->page()->title());
    hist->setIcon(webview->page()->icon());
    QUrl u = webview->url();
    QFile file(QDir::homePath() + "/.crusta_db/session.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream in(&file);
    in << u.toString().toUtf8() + "\n";
    file.close();
    webview->load(QUrl("http://"));

    if (tabWindow->count() == 1) {
        MainView::quit();
    }

    webview->page()->deleteLater();
    tabWindow->removeTab(index);
    MainView::addNewTabButton();
    recently_closed->addAction(hist);
    connect(hist, &QAction::triggered, this, [this, u] {restoreTab(u);});
}

void MainView::zoomIn()
{
    QWebEngineView *webview = getWebView();
    webview->setZoomFactor(webview->zoomFactor() + .20);
}

void MainView::zoomOut()
{
    QWebEngineView *webview = getWebView();
    webview->setZoomFactor(webview->zoomFactor() - .20);
}

void MainView::resetZoom()
{
    QWebEngineView *webview = getWebView();
    webview->setZoomFactor(1);
}

void MainView::fullScreen()
{
    if (this->window->isFullScreen()) {
        this->window->showMaximized();
        this->fullscreen_action->setText(tr("&Show Full Screen"));
    } else {
        this->window->showFullScreen();
        this->fullscreen_action->setText(tr("&Exit Full Screen"));
    }
}


void MainView::tabBarContext(QPoint point)
{
    if (this->tabWindow->tabBar()->tabAt(point) != -1) {
        int index = this->tabWindow->tabBar()->tabAt(point);
        QWidget *widget = this->tabWindow->widget(index);
        QLayout *layout = widget->layout();
        auto *webview = (QWebEngineView *)layout->itemAt(1)->widget();

        std::unique_ptr<QMenu>contextMenu(new QMenu());
        QAction *rld_tab = contextMenu->addAction(tr("&Reload Tab"));
        connect(rld_tab, &QAction::triggered, webview, &QWebEngineView::reload);
        QAction *pn_tab = contextMenu->addAction(tr("Pin Tab"));
        connect(pn_tab, &QAction::triggered, this, [this, index, pn_tab] {
            if (pn_tab->text() == tr("Pin Tab"))
            {
                pn_tab->setText("Unpin Tab");
                tabWindow->tabBar()->tabButton(index, QTabBar::RightSide)->resize(0, 0);
            } else
            {
                pn_tab->setText("Pin Tab");
                tabWindow->tabBar()->tabButton(index, QTabBar::RightSide)->resize(20, 20);
            }
        });
        QAction *mute_tab = contextMenu->addAction(tr("&Toggle Audio"));
        connect(mute_tab, &QAction::triggered, this, [this, webview] {webview->page()->setAudioMuted(!webview->page()->isAudioMuted());});
        QAction *duplicate = contextMenu->addAction(tr("&Duplicate Tab"));
        connect(duplicate, &QAction::triggered, this, [this, webview] {duplicateTab(webview);});
        contextMenu->addAction(this->bookmark_tab);
        contextMenu->addAction(this->bookmark_all_tabs);
        QAction *closeoth = contextMenu->addAction(tr("&Close Other Tabs"));
        connect(closeoth, &QAction::triggered, this, [this, index] {closeOtherTabs(index);});
        contextMenu->exec(this->tabWindow->tabBar()->mapToGlobal(point));
    } else {
        std::unique_ptr<QMenu>barContext(new QMenu());
        QAction *ntab_bar = barContext->addAction(tr("&New Tab"));
        connect(ntab_bar, &QAction::triggered, this, &MainView::addNormalTab);
        QAction *rlod = barContext->addAction(tr("&Reload All Tabs"));
        connect(rlod, &QAction::triggered, this, &MainView::reloadAllTabs);
        barContext->addAction(this->bookmark_all_tabs);
        barContext->exec(this->tabWindow->tabBar()->mapToGlobal(point));
    }
}

void MainView::FindText()
{
    try {
        QWebEngineView *webview = getWebView();
        this->hbox = new QHBoxLayout();
        this->findwidget->setParent(webview);

        if (findwidget->layout() == nullptr) {
            this->findwidget->setLayout(this->hbox);
            this->hbox->addWidget(this->close_findwidget);
            this->hbox->addWidget(this->label);
            this->hbox->addWidget(this->text);
            this->hbox->addWidget(this->match_case_btn);
            this->hbox->addWidget(new QLabel());
            this->close_findwidget->setFlat(true);
            this->close_findwidget->setIcon(QIcon(":/res/drawables/close.svg"));
            this->close_findwidget->setFixedWidth(50);
            connect(this->close_findwidget, &QPushButton::clicked, this, &MainView::hideFindWidget);
            this->label->setText(QString(tr("Search")));
            this->label->setFixedWidth(75);
            this->text->setFixedWidth(380);
            connect(this->text, &QLineEdit::returnPressed, this, &MainView::findFindWidget);
            connect(this->match_case_btn, &QCheckBox::toggled, this, &MainView::findFindWidget);
            this->match_case_btn->setText(tr("&Match &Case"));
            this->hbox->setAlignment(Qt::AlignLeft);
            this->findwidget->setFixedHeight(50);
            this->findwidget->setFixedWidth(webview->geometry().width());
            this->findwidget->setObjectName("findwidget");
        }

        this->findwidget->show();
        QPropertyAnimation *animation = new QPropertyAnimation(this->findwidget, "pos");
        animation->setDuration(600);
        this->start_findwidget = webview->geometry().height();
        animation->setStartValue(QPoint(0, this->start_findwidget));
        animation->setEndValue(QPoint(0, this->start_findwidget - 50));
        animation->start();
        QTimer::singleShot(600, [animation](){ delete animation;});
        this->text->setFocus();
    } catch (...) {
        return;
    }
}

void MainView::findFindWidget()
{
    QWebEngineView *webview = getWebView();
    QString data = this->text->text();

    if (this->match_case_btn->isChecked()) {
        webview->findText("");
        QWebEnginePage::FindFlags options = {QWebEnginePage::FindCaseSensitively};
        webview->findText(data, options);
    } else {
        webview->findText("");
        webview->findText(data);
    }
}

void MainView::hideFindWidget()
{
    QPropertyAnimation *animation = new QPropertyAnimation(this->findwidget, "pos");
    animation->setDuration(800);
    animation->setStartValue(QPoint(0, this->start_findwidget - 50));
    animation->setEndValue(QPoint(0, this->start_findwidget));
    animation->start();
    QTimer::singleShot(800, [animation](){ delete animation;});
    int index = this->tabWindow->currentIndex();
    QWidget *widget = this->tabWindow->widget(index);
    QLayout *layout = widget->layout();
    auto *webview = dynamic_cast<QWebEngineView *>(layout->itemAt(1)->widget());
    if (webview == nullptr)
        return;
    webview->findText("");
    this->findwidget->setParent(nullptr);
    this->text->setText("");
}

void MainView::selectAllText()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::SelectAll);
}

void MainView::enterPresentationMode()
{
    QWebEngineView *webview = getWebView();
    QString qurl = webview->url().toString();
    auto *newwebview = new QWebEngineView();
    newwebview->load(QUrl(qurl));
    newwebview->showFullScreen();
    auto *newExitAction = new QAction();
    newExitAction->setShortcut(Qt::Key_Escape);
    newwebview->addAction(newExitAction);
    this->p_notifier->setViewParent(newwebview);
    this->p_notifier->showNotifier();
    connect(newExitAction, &QAction::triggered, newwebview,[newwebview, newExitAction]{
        newwebview->close();
        delete newExitAction;
    });
}

void MainView::undoPageAction()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::Undo);
}

void MainView::redoPageAction()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::Redo);
}

void MainView::cutPageAction()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::Cut);
}

void MainView::copyPageAction()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::Copy);
}

void MainView::pastePageAction()
{
    QWebEngineView *webview = getWebView();
    webview->triggerPageAction(QWebEnginePage::Paste);
}

MainView::MainView()
{
    defaultTheme = "QTabWidget::tab-bar{left:0px;height:32} QTabBar{background-color:#00b0e3;} QTabBar::tab:selected{background-color:white;color:black;max-width:175px;min-width:175px;height:32px} QTabBar::tab:!selected{max-width:173px;min-width:173px;color:black;background-color:#dbdbdb;top:2px;border:0.5px solid #00b0e3;height:30px} QPushButton{border: none;background-color:#dbdbdb;} QPushButton:hover{background-color:white;}";
    window->parentView = this;

    limitCompleterFile();
    limitHistoryFile();

    QString new_version;
    QFile newVersion(QDir::homePath() + "/.crusta_db/new_version.txt");

    if (newVersion.open(QIODevice::ReadOnly)) {
        QTextStream in(&newVersion);
        in.setCodec("UTF-8");

        while (!in.atEnd()) {
            new_version = in.readLine();
        }

        newVersion.close();
    }

    QString current_version;
    QFile currentVersion(QDir::homePath() + "/.crusta_db/current.txt");

    if (currentVersion.open(QIODevice::ReadOnly)) {
        QTextStream in(&currentVersion);
        in.setCodec("UTF-8");

        while (!in.atEnd()) {
            current_version = in.readLine();
        }

        currentVersion.close();
    }

    if (current_version == new_version) {
        QDir().rmdir(QDir::homePath() + "/.crusta_db/setup.exe");
    }

    QFile f(QDir::homePath() + "/.crusta_db/settings.txt");

    if (!f.exists()) {
        QWebEngineProfile p;
        p.setHttpUserAgent("");
        QStringList ua = p.httpUserAgent().split(" ");
        QString new_string = "";
        int len = ua.length();

        for (int i = 0; i < len - 1; i++) {
            new_string += ua[i] + " ";
        }

        new_string += "Crusta/1.4.3 " + ua[len - 1];
        f.open(QIODevice::WriteOnly);
        QTextStream in(&f);
        in.setCodec("UTF-8");

        if (QLocale().languageToString(QLocale().system().language()) == "Russian") {
            in << "engine>>>>>http://www.yandex.ru/?clid=2308389&q=\nIncognito engine>>>>>http://www.yandex.ru/?clid=2308389&q=\nUA String>>>>>" + new_string + "\nHome page>>>>>\nIncognito Home page>>>>>\ntheme>>>>>" + defaultTheme + "\n";
        } else {
            in << "engine>>>>>https://www.ecosia.org/search?tt=crusta&q=\nIncognito engine>>>>>https://www.ecosia.org/search?tt=crusta&q=\nUA String>>>>>" + new_string + "\nHome page>>>>>\nIncognito Home page>>>>>\ntheme>>>>>" + defaultTheme + "\n";
        }

        f.close();
    }

    QFile f_(QDir::homePath() + "/.crusta_db/permissions.txt");

    if (!f_.exists()) {
        f_.open(QIODevice::WriteOnly);
        QTextStream in(&f_);
        in.setCodec("UTF-8");
        in << "1\n1\n0\n1\n1\n1\n1\n1\n";
        f_.close();
    }

    QFile vf_(QDir::homePath() + "/.crusta_db/current.txt");
    vf_.open(QIODevice::WriteOnly);
    QTextStream in(&vf_);
    in.setCodec("UTF-8");
    in << "1.4.3"; // current local version of crusta
    vf_.close();

    QFile fi(QDir::homePath() + "/.crusta_db/startpage.txt");

    if (!fi.exists()) {
        fi.open(QIODevice::WriteOnly);
        QTextStream in(&fi);
        in.setCodec("UTF-8");
        in << "whatsapp>>>>>https://web.whatsapp.com/\n"
           "twitter>>>>>https://twitter.com\n"
           "tumblr>>>>>https://tumblr.com\nfacebook>>>>>https://facebook.com\n"
           "yandex>>>>>http://www.yandex.ru/?clid=2308388\nlinkedin>>>>>https://linkedin.com\nyoutube>>>>>https://youtube.com\n";
        fi.close();
        std::unique_ptr<SpeedDial>sd(new SpeedDial());

        if (QLocale().languageToString(QLocale().system().language()) == "Russian") {
            QSettings("Tarptaeya", "Crusta").setValue("speeddial_srch_engine", "Yandex");
        } else {
            QSettings("Tarptaeya", "Crusta").setValue("speeddial_srch_engine", "Ecosia");
        }

        sd->save(QSettings("Tarptaeya", "Crusta").value("speeddial_bgimage").toString());
    }

    std::unique_ptr<SpeedDial>sd(new SpeedDial());
    sd->configure();
    sd->save(QSettings("Tarptaeya", "Crusta").value("speeddial_bgimage").toString());

    if (!QDir(QDir::homePath() + "/.crusta_db/sidepanel").exists()) {
        QDir().mkdir(QDir::homePath() + "/.crusta_db/sidepanel");
        QDir().mkdir(QDir::homePath() + "/.crusta_db/sidepanel/ico");
        QString a = QCoreApplication::applicationDirPath() + "/sidepanel/ico/";
        QString b = QDir::homePath() + "/.crusta_db/sidepanel/ico/";
        QDir d(QCoreApplication::applicationDirPath() + "/sidepanel/ico/");
        QStringList filesList = d.entryList(QStringList("*"));

        for (QString file : filesList) {
            QFile::copy(a + file, b + file);
        }
    }

    box->setContentsMargins(0, 0, 0, 0);
    tabWindow->tabBar()->setDocumentMode(true);
    tabWindow->setElideMode(Qt::ElideRight);
    tabWindow->tabBar()->setExpanding(false);
    tabWindow->tabBar()->setShape(QTabBar::RoundedNorth);
    tabWindow->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabWindow->tabBar(), &QTabBar::tabCloseRequested, this, &MainView::closeTab);
    connect(tabWindow->tabBar(), &QTabBar::customContextMenuRequested, this, &MainView::tabBarContext);
    createView();
    createMenuBar();
    createTabWindow();
    addNormalTab();

    addNewTabButton();
    tabWindow->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    connect(tabWindow->tabBar(), &QTabBar::tabBarDoubleClicked, this, &MainView::tabAreaDoubleClicked);
    connect(tabWindow->tabBar(), &QTabBar::tabMoved, this, &MainView::addNewTabButton);
    connect(tabWindow, &QTabWidget::currentChanged, this, &MainView::addNewTabButton);
    connect(newtabbtn, &QPushButton::clicked, this, &MainView::addNormalTab);
    connect(tabWindow, &QTabWidget::currentChanged, this, &MainView::changeSpinner);

    loadTheme();

    QString new_version_file = QDir::homePath() + "/.crusta_db/new_version.txt";
    QProcess::startDetached(QString("powershell -Command \"(New-Object Net.WebClient).DownloadFile('http://crustabrowser.com/version/current.txt', '" + new_version_file + "')\""));

    QString platform = QSysInfo().productType();

    if (platform == QString("windows")) {
        if (new_version != current_version && new_version.size() != 0) {
            if (!QFile(QDir::homePath() = "/.crusta_db/setup.exe").exists()) {
                QProcess::startDetached("powershell -Command \"Import-Module BitsTransfer; Start-BitsTransfer http://crustabrowser.com/version/setup.exe " + QDir::homePath() + "/.crusta_db/setup.exe\"");
            }

            updateOn = true;
        } else {
            QDir().remove(QDir::homePath() + "/.crusta_db/setup.exe");
        }
    } else if (platform == QString("ubuntu")) {
        QFile uf(QDir::homePath() + "/.crusta_db/updater.sh");

        if (!uf.exists()) {
            // TODO
        }

        QProcess::startDetached("./" + QDir::homePath() + "/.crusta_db/updater.sh");
    } else if (platform == QString("osx")) {
        // TODO
    } else {
        // TODO
    }
}

void MainView::createView()
{
    auto *pane = new SidePane(this);
    this->window->setWindowTitle("Crusta");
    this->window->setLayout(box);
    box->addLayout(side_pane);
    box->setSpacing(0);
    side_pane->setSpacing(0);
    side_pane->setContentsMargins(0, 0, 0, 0);
    side_pane->addWidget(pane);
    this->toggle_spane_action->setText(tr("&Hide Side Panel"));

    if (QSettings("Tarptaeya", "Crusta").value("sidepanel_visibility").isNull()) {
        QSettings("Tarptaeya", "Crusta").setValue("sidepanel_visibility", 1);
    }

    if (QSettings("Tarptaeya", "Crusta").value("sidepanel_visibility") == 0) {
        this->toggle_spane_action->setText(tr("&Show Side Panel"));
        pane->hide();
    }

    connect(this->toggle_spane_action, &QAction::triggered, this, [this, pane] {
        if (pane->isVisible())
        {
            this->toggle_spane_action->setText(tr("&Show Side Panel"));
            QSettings("Tarptaeya", "Crusta").setValue("sidepanel_visibility", 0);
            pane->hide();
        } else
        {
            this->toggle_spane_action->setText(tr("&Hide Side Panel"));
            QSettings("Tarptaeya", "Crusta").setValue("sidepanel_visibility", 1);
            pane->show();
        }
    });
    pane->download_manager = this->window->d_manager;
}

void MainView::showView()
{
    QSettings appSettings("Tarptaeya", "Crusta");
    this->window->restoreGeometry(appSettings.value("geometry").toByteArray());
    this->window->show();
}

void MainView::newWindow()
{
    auto *newView = new MainView();
    newView->showView();
}

void MainView::createMenuBar()
{
    this->file_menu = this->menu->addMenu(tr("&File"));
    this->new_tab_action = this->file_menu->addAction(tr("&New Tab"));
    connect(this->new_tab_action, &QAction::triggered, this, &MainView::addNormalTab);
    this->split_mode_action = this->file_menu->addAction(tr("&Split Mode"));
    connect(this->split_mode_action, &QAction::triggered, this, &MainView::spiltModefx);
    this->new_window_action = this->file_menu->addAction(tr("&New Window"));
    connect(this->new_window_action, &QAction::triggered, this, &MainView::newWindow);
    this->incognito = this->file_menu->addAction(tr("&New Private Window"));
    connect(this->incognito, &QAction::triggered, this, &MainView::openIncognito);
    this->file_menu->addSeparator();
    this->closeCurrentTab = this->file_menu->addAction(tr("&Close Current Tab"));
    this->closeCurrentTab->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    this->file_menu->addSeparator();
    connect(this->closeCurrentTab, &QAction::triggered, [this] {closeTab(tabWindow->currentIndex());});
    this->open_file = this->file_menu->addAction(tr("&Open File"));
    connect(this->open_file, &QAction::triggered, this, &MainView::openLocalFile);
    this->save_as_pdf = this->file_menu->addAction(tr("&Save Page As PDF"));
    connect(this->save_as_pdf, &QAction::triggered, this, &MainView::saveAsPdf);
    this->save_page = this->file_menu->addAction(tr("&Save Page"));
    connect(this->save_page, &QAction::triggered, this, &MainView::savePage);
    this->capture_screenshot = this->file_menu->addAction(tr("&Capture ScreenShot"));
    this->capture_screenshot->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
    connect(this->capture_screenshot, &QAction::triggered, this, &MainView::screenShot);
    this->exit_action = this->file_menu->addAction(tr("&Exit"));
    connect(this->exit_action, &QAction::triggered, this, &MainView::closeWindow);
    this->edit_menu = this->menu->addMenu(tr("&Edit"));
    this->undo_action = this->edit_menu->addAction(tr("&Undo"));
    connect(this->undo_action, &QAction::triggered, this, &MainView::undoPageAction);
    this->redo_action = this->edit_menu->addAction(tr("&Redo"));
    connect(this->redo_action, &QAction::triggered, this, &MainView::redoPageAction);
    this->edit_menu->addSeparator();
    this->cut_action = this->edit_menu->addAction(tr("&Cut"));
    connect(this->cut_action, &QAction::triggered, this, &MainView::cutPageAction);
    this->copy_action = this->edit_menu->addAction(tr("&Copy"));
    connect(this->copy_action, &QAction::triggered, this, &MainView::copyPageAction);
    this->paste_action = this->edit_menu->addAction(tr("&Paste"));
    connect(this->paste_action, &QAction::triggered, this, &MainView::pastePageAction);
    this->edit_menu->addSeparator();
    this->selectall_action = this->edit_menu->addAction(tr("&Select All"));
    connect(this->selectall_action, &QAction::triggered, this, &MainView::selectAllText);
    this->find_action = this->edit_menu->addAction(tr("&Find"));
    this->find_action->setShortcut(QKeySequence(QKeySequence::Find));
    connect(this->find_action, &QAction::triggered, this, &MainView::FindText);
    this->edit_menu->addSeparator();
    this->preference = this->edit_menu->addAction(tr("&Edit Theme"));
    connect(this->preference, &QAction::triggered, this, &MainView::editPreference);
    this->edit_permissions = this->edit_menu->addAction(tr("&Edit Permissions"));
    connect(this->edit_permissions, &QAction::triggered, this, &MainView::editPermissions);
    this->view_menu = this->menu->addMenu(tr("&View"));
    this->view_menu->addAction(this->toggle_spane_action);
    this->view_page_source_action = this->view_menu->addAction(tr("&Page Source"));
    connect(this->view_page_source_action, &QAction::triggered, this, &MainView::viewPageSource);
    this->view_menu->addSeparator();
    this->zoom_in_action = this->view_menu->addAction(tr("&Zoom In"));
    connect(this->zoom_in_action, &QAction::triggered, this, &MainView::zoomIn);
    this->zoom_out_action = this->view_menu->addAction(tr("&Zoom Out"));
    connect(this->zoom_out_action, &QAction::triggered, this, &MainView::zoomOut);
    this->reset_zoom_action = this->view_menu->addAction(tr("&Reset Zoom"));
    connect(this->reset_zoom_action, &QAction::triggered, this, &MainView::resetZoom);
    this->view_menu->addSeparator();
    this->presentation_action = this->view_menu->addAction(tr("&Presentation Mode"));
    connect(this->presentation_action, &QAction::triggered, this, &MainView::enterPresentationMode);
    this->fullscreen_action = this->view_menu->addAction(tr("&Show Full Screen"));
    connect(this->fullscreen_action, &QAction::triggered, this, &MainView::fullScreen);
    this->history_menu = this->menu->addMenu(tr("&History"));
    this->clearAllHist = this->history_menu->addAction(tr("&Clear All History"));
    connect(this->clearAllHist, &QAction::triggered, this, &MainView::clearHistory);
    this->history_menu->addSeparator();
    this->restore_session = this->history_menu->addAction(tr("Restore Previous Session"));
    connect(this->restore_session, &QAction::triggered, this, &MainView::restoreSession);
    this->recently_closed = this->history_menu->addMenu(tr("&Recently Closed"));
    this->bookmark_menu = this->menu->addMenu(tr("&Bookmarks"));
    this->bookmark_tab = this->bookmark_menu->addAction(tr("&Bookmark This Page"));
    connect(this->bookmark_tab, &QAction::triggered, this, &MainView::bookmarkTab);
    this->bookmark_all_tabs = this->bookmark_menu->addAction(tr("&Bookmark All Tabs"));
    connect(this->bookmark_all_tabs, &QAction::triggered, this, &MainView::bookmarkAllTabs);
    this->tool_menu = this->menu->addMenu(tr("&Tools"));
    this->sitei = this->tool_menu->addAction(tr("&Site Info"));
    connect(this->sitei, &QAction::triggered, this, &MainView::showPageInfo);
    this->tool_menu->addSeparator();
    this->web_inspector_action = this->tool_menu->addAction(tr("&Web Inspector"));
    connect(this->web_inspector_action, &QAction::triggered, this, &MainView::openDebugger);
    this->tool_menu->addSeparator();
    this->runJsCode = this->tool_menu->addAction(tr("&Run Javascript Code"));
    this->viewSource = this->tool_menu->addAction(tr("View Page Source"));
    this->changeUA = this->tool_menu->addAction(tr("Edit User Agent"));
    this->pick_color = this->tool_menu->addAction(tr("Pick Screen Color"));
    connect(this->viewSource, &QAction::triggered, this, &MainView::viewPageSource);
    connect(this->runJsCode, &QAction::triggered, this, &MainView::showJsCodeEditor);
    connect(this->changeUA, &QAction::triggered, this, &MainView::changeUAfx);
    connect(this->pick_color, &QAction::triggered, this, &MainView::pickColor);
    QMenu *help_menu = this->menu->addMenu(tr("Help"));
    QAction *help_ = help_menu->addAction(tr("User manual"));
    connect(help_, &QAction::triggered, this, &MainView::help);
    QAction* about_qt = help_menu->addAction(tr("About Qt"));
    connect(about_qt, &QAction::triggered, this, []{QApplication::aboutQt();});
    this->new_tab_action->setShortcut(QKeySequence(QKeySequence::AddTab));
    this->new_window_action->setShortcut(QKeySequence(QKeySequence::New));
    this->incognito->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    this->open_file->setShortcut(QKeySequence(QKeySequence::Open));
    this->save_as_pdf->setShortcut(QKeySequence(QKeySequence::Print));
    this->save_page->setShortcut(QKeySequence(QKeySequence::Save));
    this->exit_action->setShortcut(QKeySequence(QKeySequence::Quit));
    this->undo_action->setShortcut(QKeySequence(QKeySequence::Undo));
    this->redo_action->setShortcut(QKeySequence(QKeySequence::Redo));
    this->cut_action->setShortcut(QKeySequence(QKeySequence::Cut));
    this->copy_action->setShortcut(QKeySequence(QKeySequence::Copy));
    this->paste_action->setShortcut(QKeySequence(QKeySequence::Paste));
    this->selectall_action->setShortcut(QKeySequence(QKeySequence::SelectAll));
    this->zoom_in_action->setShortcut(QKeySequence(QKeySequence::ZoomIn));
    this->zoom_out_action->setShortcut(QKeySequence(QKeySequence::ZoomOut));
    this->fullscreen_action->setShortcut(QKeySequence(QKeySequence::FullScreen));
    this->window->menu = this->menu;
}

void MainView::createTabWindow()
{
    this->tabWindow->setMovable(true);
    this->tabWindow->setTabsClosable(true);
    this->tabWindow->setContentsMargins(0, 0, 0, 0);
    this->box->addWidget(this->tabWindow);
}

void MainView::addNormalTab()
{
    auto *tab = new TabWindow();
    tab->menu_btn->setMenu(menu);
    tab->menu_btn->setStyleSheet("QPushButton::menu-indicator { image: none; }");
    this->tabWindow->addTab(tab->returnTab(), tr("new Tab"));
    this->tabWindow->setCurrentIndex(this->tabWindow->count() - 1);
    int cnt = this->tabWindow->count();

    if (cnt == 1) {
        this->tabWindow->setTabText(0, tr("Connecting..."));
        QWidget *widget = this->tabWindow->widget(0);
        QLayout *layout = widget->layout();
        auto *webview = (WebView *)layout->itemAt(1)->widget();
        QString home;
        QFile inputFile(QDir::homePath() + "/.crusta_db/settings.txt");

        if (inputFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&inputFile);
            in.setCodec("UTF-8");

            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList data = line.split(">>>>>");

                if (data[0] == "Home page") {
                    home = data[1];
                    break;
                }
            }

            inputFile.close();
        }

        if (home.isEmpty()) {
            std::unique_ptr<QDir>exec_dir(new QDir(QDir::homePath() + "/.crusta_db"));
            exec_dir->cd("speeddial");
            QString forbidden;

            if (exec_dir->absolutePath().startsWith("/")) {
                forbidden = "file://" + exec_dir->absolutePath() + "/index.html";
            } else {
                forbidden = "file:///" + exec_dir->absolutePath() + "/index.html";
            }

            home = forbidden;
            QFile f(QDir::homePath() + "/.crusta_db/settings.txt");

            if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QString s;
                QTextStream t(&f);
                t.setCodec("UTF-8");

                while (!t.atEnd()) {
                    QString line = t.readLine();
                    QStringList data = line.split(">>>>>");

                    if (data[0] == "Home page") {
                        s.append(data[0] + ">>>>>" + home + "\n");
                    } else {
                        s.append(line + "\n");
                    }
                }

                f.resize(0);
                t << s;
                f.close();
            }
        }

        webview->home_page = home;

        if (QCoreApplication::arguments().length() > 1) {
            QString given_url = QCoreApplication::arguments().at(1);

            if (!given_url.startsWith("--")) {
                webview->load(given_url.split("\\").join("/"));
            } else {
                webview->load(home);
            }
        } else {
            webview->load(home);
        }
    } else {
        WebView *webview = dynamic_cast<WebView *>(getWebView(cnt - 1));

        QFile inputFile(QDir::homePath() + "/.crusta_db/settings.txt");

        if (inputFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&inputFile);
            in.setCodec("UTF-8");

            while (!in.atEnd()) {
                QString line = in.readLine();
                QStringList data = line.split(">>>>>");

                if (data[0] == "Home page") {
                    webview->home_page = data[1];
                    break;
                }
            }

            inputFile.close();
        }

        std::unique_ptr<QDir>exec_dir(new QDir(QDir::homePath() + "/.crusta_db"));
        exec_dir->cd("speeddial");

        if (exec_dir->absolutePath().startsWith("/")) {
            webview->load(QUrl("file://" + exec_dir->absolutePath() + "/index.html"));
        } else {
            webview->load(QUrl("file:///" + exec_dir->absolutePath() + "/index.html"));
        }
    }

    tab->addr_bar->setFocus();
    MainView::addNewTabButton();
}

void MainView::viewPageSource()
{
    int index = this->tabWindow->currentIndex();
    QWebEngineView *webview = getWebView();
    QString qurl = webview->url().toString();
    qurl = QString("view-source:") + qurl;
    auto *tab = new TabWindow();
    tab->menu_btn->setMenu(menu);
    index++;
    this->tabWindow->insertTab(index, tab->returnTab(), tr("new Tab"));
    this->tabWindow->setCurrentIndex(index);
    QWidget *wid = this->tabWindow->widget(index);
    QLayout *lay = wid->layout();
    auto *webv = (QWebEngineView *)lay->itemAt(1)->widget();
    webv->load(QUrl(qurl));
    MainView::addNewTabButton();
}

void MainView::saveAsPdf()
{
    QPrinter printer;
    printer.setPageLayout(currentPageLayout);
    QPageSetupDialog dlg(&printer);

    if (dlg.exec() != QPageSetupDialog::Accepted) {
        return;
    }

    currentPageLayout.setPageSize(printer.pageLayout().pageSize());
    currentPageLayout.setOrientation(printer.pageLayout().orientation());
    QFileDialog f;
    QString file_name = f.getSaveFileName(this->window, tr("Crusta : Save File"), QDir::homePath(), "Pdf File(*.pdf)", nullptr, f.options());
    int index = this->tabWindow->currentIndex();
    QWebEngineView *webview = getWebView(index);

    if (file_name != "") {
        if (!file_name.endsWith(".pdf")) {
            file_name += QString(".pdf");
        }

        webview->page()->printToPdf(file_name, QPageLayout(printer.pageLayout().pageSize(), printer.pageLayout().orientation(), printer.pageLayout().margins()));
    }
}

void MainView::savePage()
{
    QFileDialog f;
    QString file_name = f.getSaveFileName(this->window, tr("Crusta : Save File"), QDir::homePath(), "WebPage, Complete", nullptr, f.options());
    QWebEngineView *webview = getWebView();

    if (file_name != QString("")) {
        webview->page()->save(file_name, QWebEngineDownloadItem::CompleteHtmlSaveFormat);
    }
}

void MainView::showJsCodeEditor()
{
    QWebEngineView *webview = getWebView();
    jsEditor->setView(webview);
    jsEditor->show();
}

void MainView::openLocalFile()
{
    QFileDialog f;
    QString filename = f.getOpenFileName(this->window, tr("Crusta : Open File"), QDir::homePath(), QString(), nullptr, f.options());
    QWebEngineView *webview = getWebView();

    if (filename == "") {
        return;
    }

    if (!filename.startsWith("/")) {
        webview->load(QUrl(QString("file:///") + filename));
    } else {
        webview->load(QUrl(QString("file://") + filename));
    }
}

void MainView::screenShot()
{
    QSound::play(":/res/audio/shutter.wav");
    QWebEngineView *webview = getWebView();
    QPixmap pmap = webview->grab();
    QFileDialog f;
    QString filename = f.getSaveFileName(this->window, tr("Crusta : Open File"), QDir::homePath(), QString("Images (*.png *.xpm *.jpg *.bmp)"), nullptr, f.options());

    if (filename != "") {
        if (!(filename.endsWith(".png") || filename.endsWith(".jpg") || filename.endsWith(".bmp") || filename.endsWith(".xpm"))) {
            filename += QString(".png");
        }

        pmap.save(filename);
    }
}

void MainView::tabAreaDoubleClicked(int index)
{
    if (index == -1) {
        MainView::addNormalTab();
    }
}

void MainView::addNewTabButton()
{
    int cnt = this->tabWindow->count();
    int x = cnt * 175 + 5; //size of a tab;

    if (newtabbtn->parent() == nullptr) {
        newtabbtn->setParent(this->tabWindow->tabBar());
    }

    this->newtabbtn->move(x, 5);
    this->newtabbtn->setFlat(false);
    this->newtabbtn->setFixedHeight(22);
    this->newtabbtn->setFixedWidth(25);
}

void MainView::editPreference()
{
    auto *th = new ThemeEditor();
    th->_parent = this;
    th->exec();
}

void MainView::duplicateTab(QWebEngineView *view)
{
    TabWindow *tab = new TabWindow();
    tab->menu_btn->setMenu(menu);
    WebView *wview = new WebView();
    wview->load(view->url());
    this->tabWindow->addTab(tab->returnTab(wview), tr("new Tab"));
    this->tabWindow->setCurrentIndex(this->tabWindow->count() - 1);
    MainView::addNewTabButton();
}

void MainView::reloadAllTabs()
{
    int cnt = this->tabWindow->count();

    for (int i = 0; i < cnt; i++) {
        QWebEngineView *webview = getWebView(i);
        webview->reload();
    }
}

void MainView::closeOtherTabs(int index)
{
    int cnt = this->tabWindow->count();
    int i = 0;

    while (i < cnt) {
        if (i != index) {
            QWebEngineView *webview = getWebView(i);
            webview->page()->windowCloseRequested();

            if (index > 0) {
                index--;
            }

            cnt--;
            i = 0;
            MainView::addNewTabButton();
            continue;
        }

        i++;
    }
}

void MainView::restoreTab(QUrl u)
{
    MainView::addNormalTab();
    int index = this->tabWindow->count() - 1;
    QWebEngineView *webview = getWebView(index);
    webview->load(u);
}

void MainView::help()
{
    MainView::addNormalTab();
    int index = this->tabWindow->count() - 1;
    QWebEngineView *webview = getWebView(index);
    webview->load(QUrl("http://www.crustabrowser.com/help"));
}

void MainView::clearHistory()
{
    QFile file(QDir::homePath() + "/.crusta_db/history.txt");
    file.open(QIODevice::WriteOnly);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "";
    file.close();
}

void MainView::bookmarkTab()
{
    QWebEngineView *webview = getWebView();

    QFile file(QDir::homePath() + "/.crusta_db/bookmarks.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << webview->title().toUtf8() + ">>>>>" + webview->url().toString().toUtf8() + ">>>>>" + "\n";
    file.close();
}

void MainView::bookmarkAllTabs()
{
    int cnt = this->tabWindow->count();

    for (int index = 0; index < cnt; index++) {
        QWebEngineView *webview = getWebView(index);

        QFile file(QDir::homePath() + "/.crusta_db/bookmarks.txt");
        file.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << webview->title().toUtf8() + ">>>>>" + webview->url().toString().toUtf8() + ">>>>>" + "\n";
        file.close();
    }
}

void MainView::quit()
{
    this->window->deleteLater();
    QString platform = QSysInfo().productType();

    if (platform == QString("windows")) {
        if (updateOn && QFile(QDir::homePath() + "/.crusta_db/setup.exe").exists()) {
            QDialog* ud = new QDialog();
            ud->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CoverWindow);
            ud->setWindowTitle(tr("Crusta Updater"));
            QLabel* lbl = new QLabel(tr("A new version of Crusta is available.\nDo you want to launch the setup now?"));
            auto * vbox_ud = new QVBoxLayout();
            ud->setLayout(vbox_ud);
            vbox_ud->addWidget(lbl);
            QPushButton* ok = new QPushButton(tr("Yes"));
            QPushButton* later = new QPushButton(tr("Remind Later"));
            auto * hbox_ud = new QHBoxLayout();
            hbox_ud->addWidget(ok);
            hbox_ud->addWidget(later);
            vbox_ud->addLayout(hbox_ud);
            connect(ok, &QPushButton::clicked, ud, &QDialog::accept);
            connect(later, &QPushButton::clicked, ud, &QDialog::reject);
            ud->setFixedWidth(300);

            if (ud->exec() == QDialog::Accepted) {
                QProcess::startDetached(QDir::homePath() + "/.crusta_db/setup.exe");
            }
        }
    } else if (platform == QString("ubuntu")) {
        if (updateOn) {
            // TODO
        }
    } else if (platform == QString("osx")) {
        if (updateOn) {
            // TODO
        }
    } else {
        if (updateOn) {
            // TODO
        }
    }
}

void MainView::showPageInfo()
{
    QWebEngineView *webview = getWebView();
    std::unique_ptr<SiteInfoWidget>sw(new SiteInfoWidget(webview));
    sw->exec();
}

void MainView::changeSpinner(int index)
{
    int cnt = this->tabWindow->count();

    for (int i = 0; i < cnt; i++) {
        QWebEngineView *webview = getWebView(i);

        if (((WebView *)webview)->favLoaded) {
            continue;
        }

        QLabel *lbl = new QLabel();

        if (i != index) {
            QMovie *mov = new QMovie(":/res/videos/passive_loader.gif");
            lbl->setMovie(mov);
            mov->start();
        } else {
            QMovie *mov = new QMovie(":/res/videos/loader.gif");
            lbl->setMovie(mov);
            mov->start();
        }

        this->tabWindow->tabBar()->setTabButton(i, QTabBar::LeftSide, lbl);
    }
}

void MainView::openIncognito()
{
    auto *pmainview = new PrivateMainView();
    pmainview->showView();
}

void MainView::changeUAfx()
{
    auto *ad = new AddressLineEdit();
    ad->setUAString();
    ad->deleteLater();
}

void MainView::spiltModefx()
{
    if (this->split_mode_action->text() == QString(tr("&Exit Split Mode"))) {
        if (this->box->count() == 3) {
            this->box->removeWidget(this->splitWindow->window);
            this->splitWindow->closeWindow();
            this->split_mode_action->setText(tr("&Split Mode"));
            return;
        }

        this->split_mode_action->setText(tr("&Split Mode"));
        return;
    }

    box->setContentsMargins(0, 0, 0, 0);
    box->setSpacing(0);
    this->splitWindow = new MainView();
    this->splitWindow->box->removeItem(this->splitWindow->side_pane);
    box->addWidget(this->splitWindow->window);
    this->splitWindow->split_mode_action->setVisible(false);
    this->split_mode_action->setText(tr("&Exit Split Mode"));
}

void MainView::closeWindow()
{
    QFile file(QDir::homePath() + "/.crusta_db/session.txt");
    file.open(QIODevice::WriteOnly);
    QTextStream in(&file);
    in.setCodec("UTF-8");
    in << "";
    file.close();

    if (this->split_mode_action->text() == QString(tr("&Exit Split Mode"))) {
        if (this->box->count() == 3) {
            this->splitWindow->closeWindow();
            this->split_mode_action->setText(tr("&Split Mode"));
        }
    }

    int side_cnt = this->side_pane->itemAt(0)->widget()->layout()->itemAt(0)->widget()->layout()->count();

    while (side_cnt != 6) {
        auto *side_btn = (SidePaneButton *)this->side_pane->itemAt(0)->widget()->layout()->itemAt(0)->widget()->layout()->itemAt(4)->widget();
        side_btn->sidewebview->load(QUrl("http://"));
        this->side_pane->itemAt(0)->widget()->layout()->itemAt(0)->widget()->layout()->removeWidget(side_btn);
        side_btn->sidewebview->page()->deleteLater();
        side_cnt = this->side_pane->itemAt(0)->widget()->layout()->itemAt(0)->widget()->layout()->count();
    }

    int cnt = this->tabWindow->count();

    for (int i = 0; i < cnt; i++) {
        this->closeTab(0);
    }
}

void Window::closeEvent(QCloseEvent *event)
{
    this->parentView->closeWindow();
    QSettings appSettings("Tarptaeya", "Crusta");
    appSettings.setValue("geometry", saveGeometry());
    event->accept();
}

void MainView::restoreSession()
{
    QFile inputFile(QDir::homePath() + "/.crusta_db/session.txt");

    if (inputFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&inputFile);
        in.setCodec("UTF-8");

        while (!in.atEnd()) {
            QString line = in.readLine();
            this->openUrl(line);
        }

        inputFile.close();
    }
}

void MainView::openUrl(QString url)
{
    this->addNormalTab();
    int index = this->tabWindow->count() - 1;
    QWebEngineView *webview = getWebView(index);
    webview->load(QUrl(url));
}

void MainView::openDebugger()
{
    QString a = QCoreApplication::arguments().last();

    if (!a.contains("--remote-debugging-port=")) {
        auto *notify = new QMessageBox(this->window);
        notify->setWindowTitle("Crusta : Debugger");
        notify->setText("Enable Debugging Mode By Launching Crusta With Argument '--remote-debugging-port=<port>' ");
        notify->exec();
        return;
    }

    QDialog *w = new QDialog();
    QLabel *lbl = new QLabel(tr("REMOTE DEBUGGING PORT :"));
    auto *port = new QLineEdit();
    auto *hbox = new QHBoxLayout();
    hbox->addWidget(lbl);
    hbox->addWidget(port);
    auto *h1box = new QHBoxLayout();
    QPushButton *ok = new QPushButton(tr("OK"));
    h1box->addWidget(new QLabel());
    h1box->addWidget(ok);
    ok->setFixedWidth(100);
    auto *vbox = new QVBoxLayout();
    vbox->addLayout(hbox);
    vbox->addLayout(h1box);
    w->setLayout(vbox);
    w->setFixedWidth(500);
    w->setWindowTitle("Crusta : Debugger");
    connect(ok, &QPushButton::clicked, w, &QDialog::accept);

    if (w->exec() != QDialog::Accepted) {
        return;
    }

    if (port->text() == "") {
        return;
    }

    QString _port = port->text();
    auto *debugger = new QWebEngineView();
    debugger->load(QUrl("http://localhost:" + _port));
    debugger->show();
}

void MainView::loadTheme()
{
    QFile inputFile(QDir::homePath() + "/.crusta_db/settings.txt");

    if (inputFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&inputFile);
        in.setCodec("UTF-8");

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.split(">>>>>").length() < 2) {
                continue;
            }

            if (line.split(">>>>>")[0] == "theme") {
                theme = line.split(">>>>>")[1];
            }
        }

        inputFile.close();
    }

    QString bgcolor = QString(QString(theme.split(" ")[1]).split("{")[1]).split("}")[0];
    ((SidePane *)side_pane->itemAt(0)->widget())->top->setStyleSheet(bgcolor + "; border: 0px solid");
    this->tabWindow->setStyleSheet(theme);
}

void MainView::limitCompleterFile()
{
    QFile inputFile(QDir::homePath() + "/.crusta_db/completer.txt");

    while (inputFile.size() > 75000) { //limit file to 75kb
        QString s = "";

        if (inputFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&inputFile);
            in.setCodec("UTF-8");
            int cnt = 0;

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (cnt != 0) {
                    s.append(line + "\n");
                }

                cnt++;
            }

            inputFile.close();
        }

        QFile file(QDir::homePath() + "/.crusta_db/completer.txt");
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << s.toUtf8();
        file.close();
    }
}

void MainView::limitHistoryFile()
{
    QFile inputFile(QDir::homePath() + "/.crusta_db/history.txt");

    while (inputFile.size() > 20000000) { // limit file to 2Mb
        QString s = "";

        if (inputFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&inputFile);
            in.setCodec("UTF-8");
            int cnt = 0;

            while (!in.atEnd()) {
                QString line = in.readLine();

                if (cnt != 0) {
                    s.append(line + "\n");
                }

                cnt++;
            }

            inputFile.close();
        }

        QFile file(QDir::homePath() + "/.crusta_db/history.txt");
        file.open(QIODevice::WriteOnly);
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out << s.toUtf8();
        file.close();
    }
}

void MainView::editPermissions()
{
    auto * pdg= new PermissionDialog;
    pdg->show();
}

void MainView::pickColor()
{
    QColorDialog cd;
    QString color = cd.getColor(Qt::white, this, QString(tr("Pick Color"))).name();
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(color);
}

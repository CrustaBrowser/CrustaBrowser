#include "tab.h"
#include "tabwidget.h"
#include "webpage.h"
#include "webview.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QTabBar>

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget (parent)
{
    m_newTabButton = new QToolButton;

    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setObjectName(QStringLiteral("NewTabButton"));
    m_newTabButton->setToolTip(QStringLiteral("Click to add tab"));
    m_newTabButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

    QTabBar *tabBar = this->tabBar();
    tabBar->setDocumentMode(true);
    tabBar->setExpanding(true);
    tabBar->setMovable(true);
    tabBar->setTabsClosable(true);
    tabBar->setUsesScrollButtons(true);
    tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    setContentsMargins(0, 0, 0, 0);

    setCornerWidget(m_newTabButton);

    addTab(new Tab, QStringLiteral("New tab"));

    connect(m_newTabButton, &QToolButton::clicked, this, [this] { addTab(new Tab, QStringLiteral("New Tab")); });
    connect(this, &TabWidget::currentChanged, this, [this] (int index) {
        Tab *tab = dynamic_cast<Tab *>(widget(index));
        if (!tab) {
            return ;
        }

        emit urlChanged(tab->webView()->url());
        emit historyChanged(tab->webView()->history());

        if (tab->webView()->isLoading()) {
            emit loadStarted();
        } else {
            emit loadFinished();
        }
    });
    connect(tabBar, &QTabBar::customContextMenuRequested, this, &TabWidget::createContextMenu);
    connect(tabBar, &QTabBar::tabCloseRequested, this, [this] (int index) {
        Tab *tab = dynamic_cast<Tab *>(widget(index));
        if (!tab) {
            return;
        }

        removeTab(index);
        tab->deleteLater();

        if (!count()) {
            // TODO: add option to not close window when
            //       last tab is closed
            emit windowCloseRequested();
        }
    });
}

int TabWidget::addTab(Tab *tab, const QString &label)
{
    WebView *webView = tab->webView();

    connect(webView, &WebView::titleChanged, this, [this, tab] (const QString &title) {
        int index = indexOf(tab);
        setTabText(index, title);
        setTabToolTip(index, title);
    });

    connect(webView, &WebView::iconChanged, this, [this, tab] (const QIcon &icon) {
        int index = indexOf(tab);

        if (tab->webView()->page()->recentlyAudible()) {
            return ;
        }

        setTabIcon(index, icon);
    });

    connect(webView, &WebView::urlChanged, this, [this, tab] (const QUrl &url) {
        int index = indexOf(tab);
        if (index != currentIndex()) {
            return ;
        }
        emit urlChanged(url);
    });

    connect(webView, &WebView::historyChanged, this, [this, tab] (QWebEngineHistory *history) {
        int index = indexOf(tab);
        if (index != currentIndex()) {
            return ;
        }
        emit historyChanged(history);
    });

    connect(webView, &WebView::loadStarted, this, [this, tab] {
        int index = indexOf(tab);
        if (index != currentIndex()) {
            return ;
        }

        emit loadStarted();
    });

    connect(webView, &WebView::loadFinished, this, [this, tab] {
        int index = indexOf(tab);
        if (index != currentIndex()) {
            return ;
        }

        emit loadFinished();
    });

    connect(webView->page(), &WebPage::recentlyAudibleChanged, this, [this, tab] (bool recentlyAudible) {
        int index = indexOf(tab);
        setTabIcon(index, tab->webView()->page()->isAudioMuted() ? QIcon::fromTheme(QStringLiteral("audio-volume-muted")) :
                                                                   recentlyAudible ? QIcon::fromTheme(QStringLiteral("audio-volume-full")) :
                                                                                     tab->webView()->icon());
    });

    return QTabWidget::addTab(tab, label);
}

void TabWidget::back()
{
    Tab *tab = dynamic_cast<Tab *>(widget(currentIndex()));
    if (!tab) {
        return;
    }

    tab->webView()->back();
}

void TabWidget::forward()
{
    Tab *tab = dynamic_cast<Tab *>(widget(currentIndex()));
    if (!tab) {
        return;
    }

    tab->webView()->forward();
}

void TabWidget::navigateToItem(const QWebEngineHistoryItem &item)
{
    Tab *tab = dynamic_cast<Tab *>(widget(currentIndex()));
    if (!tab) {
        return;
    }

    tab->webView()->history()->goToItem(item);
}

void TabWidget::changeLoadingState()
{
    Tab *tab = dynamic_cast<Tab *>(widget(currentIndex()));
    if (!tab) {
        return;
    }

    if (tab->webView()->isLoading()) {
        tab->webView()->stop();
    } else {
        tab->webView()->reload();
    }
}

void TabWidget::createContextMenu(const QPoint &pos)
{
    int index = tabBar()->tabAt(pos);
    Tab *tab = dynamic_cast<Tab *>(widget(index));
    if (!tab) {
        return;
    }

    QMenu menu;
    QAction *duplicate = menu.addAction(QStringLiteral("Duplicate tab"));
    QAction *mute = menu.addAction(tab->webView()->page()->isAudioMuted() ? QStringLiteral("Unmute tab") : QStringLiteral("Mute tab"));
    menu.addSeparator();
    QAction *copy = menu.addAction(QStringLiteral("Copy page title"));
    menu.addSeparator();
    QAction *configure = menu.addAction(QStringLiteral("Configure"));

    connect(mute, &QAction::triggered, this, [tab] { tab->webView()->page()->setAudioMuted(!tab->webView()->page()->isAudioMuted()); });
    connect(copy, &QAction::triggered, this, [tab] { qApp->clipboard()->setText(tab->webView()->title()); });

    menu.exec(mapToGlobal(pos));
}

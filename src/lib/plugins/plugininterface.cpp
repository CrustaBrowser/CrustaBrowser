#include "api/qmldownloaditem.h"

#include "plugininterface.h"

#include <QQmlEngine>

PluginInterface::PluginInterface(QObject *parent)
    : QObject (parent)
{
}

void PluginInterface::callLoad()
{
    load.call();
}

void PluginInterface::callUnload()
{
    unload.call();
}

bool PluginInterface::callAcceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    if (!acceptNavigationRequest.isCallable()) {
        return true;
    }

    QJSValueList args;
    args << QString::fromUtf8(url.toEncoded());
    args << type;
    args << isMainFrame;

    return acceptNavigationRequest.call(args).toBool();
}

void PluginInterface::callAcceptDownloadRequest(QWebEngineDownloadItem *download)
{
    QmlDownloadItem *item = new QmlDownloadItem(download);
\
    QJSValueList args;
    args << qmlEngine(this)->newQObject(item);

    acceptDownloadRequest.call(args);
}

QJSValue PluginInterface::readLoad() const
{
    return load;
}

void PluginInterface::writeLoad(const QJSValue &jsValue)
{
    load = jsValue;
}

QJSValue PluginInterface::readUnLoad() const
{
    return unload;
}

void PluginInterface::writeUnLoad(const QJSValue &jsValue)
{
    unload = jsValue;
}

QJSValue PluginInterface::readAcceptNavigationRequest() const
{
    return acceptNavigationRequest;
}

void PluginInterface::writeAcceptNavigationRequest(const QJSValue &jsValue)
{
    acceptNavigationRequest = jsValue;
}

QJSValue PluginInterface::readAcceptDownloadRequest() const
{
    return acceptDownloadRequest;
}

void PluginInterface::writeAcceptDownloadRequest(const QJSValue &jsValue)
{
    acceptDownloadRequest = jsValue;
}

QQmlListProperty<QObject> PluginInterface::readChildItems()
{
    return QQmlListProperty<QObject>(this, childItems);
}

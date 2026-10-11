#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <qqmlregistration.h>

class BrandingCatalog : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList assets READ assets NOTIFY assetsChanged)
    Q_PROPERTY(int count READ count NOTIFY assetsChanged)

public:
    explicit BrandingCatalog(QObject *parent = nullptr);

    QVariantList assets() const { return m_assets; }
    int count() const { return m_assets.size(); }

    Q_INVOKABLE QVariantMap get(int index) const;

signals:
    void assetsChanged();

private:
    void loadAssets();
    QString readFile(const QString &resourcePath, const QString &diskPath) const;
    QString resolveAssetSource(const QString &fileName) const;
    void parseCells(const QString &content);
    void parseMascot(const QString &content);

    QVariantList m_assets;
};

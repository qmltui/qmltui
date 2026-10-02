#pragma once

#include "Item.h"
#include <QVariant>
#include <QVariantList>
#include <vector>

namespace qtui {

class BrailleShape;

class BrailleCanvas : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    BrailleCanvas(QObject* parent = nullptr);
    ~BrailleCanvas() override = default;

    QString color() const { return m_color; }
    void setColor(const QString& color);
    uint32_t resolvedColor() const { return m_resolvedColor; }

    void setPixel(int x, int y, bool value = true);
    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) override;

signals:
    void colorChanged();

private:
    void resizeBuffer();
    std::vector<uint8_t> m_cells;
    QString m_color = "white";
    uint32_t m_resolvedColor = 0xFFFFFFFF;
};

class BrailleShape : public Item {
    Q_OBJECT
public:
    BrailleShape(QObject* parent = nullptr) : Item(parent) {}
    virtual void draw(BrailleCanvas* canvas) = 0;
};

class BraillePoint : public BrailleShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)

public:
    BraillePoint(QObject* parent = nullptr);
    int x() const { return m_x; }
    void setX(int x);
    int y() const { return m_y; }
    void setY(int y);

signals:
    void xChanged();
    void yChanged();

public:
    void draw(BrailleCanvas* canvas) override;

private:
    int m_x = 0;
    int m_y = 0;
};

class BrailleLine : public BrailleShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(int y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(int x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(int y2 READ y2 WRITE setY2 NOTIFY y2Changed)

public:
    BrailleLine(QObject* parent = nullptr);
    int x1() const { return m_x1; }
    void setX1(int x1);
    int y1() const { return m_y1; }
    void setY1(int y1);
    int x2() const { return m_x2; }
    void setX2(int x2);
    int y2() const { return m_y2; }
    void setY2(int y2);

signals:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();

public:
    void draw(BrailleCanvas* canvas) override;

private:
    int m_x1 = 0;
    int m_y1 = 0;
    int m_x2 = 0;
    int m_y2 = 0;
};

class BraillePolygon : public BrailleShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(bool closed READ closed WRITE setClosed NOTIFY closedChanged)

public:
    BraillePolygon(QObject* parent = nullptr);
    QVariantList points() const { return m_points; }
    void setPoints(const QVariantList& points);
    bool closed() const { return m_closed; }
    void setClosed(bool closed);

signals:
    void pointsChanged();
    void closedChanged();

public:
    void draw(BrailleCanvas* canvas) override;

private:
    QVariantList m_points;
    bool m_closed = true;
};

} // namespace qtui

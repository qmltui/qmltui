#pragma once

#include "LogProvider.h"

#include <QTimer>

// Emits a fixed startup backlog (asserted by scenario tests) and then keeps
// producing rotating synthetic entries.
class MockLogProvider : public LogProvider {
    Q_OBJECT

public:
    explicit MockLogProvider(QObject *parent = nullptr);

private:
    void emitNext();

    QTimer m_timer;
    int m_tick = 0;
};

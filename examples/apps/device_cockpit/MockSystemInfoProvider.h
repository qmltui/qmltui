#pragma once

#include "SystemInfoProvider.h"

// Deterministic simulated device. Identity, storage, and interface values are
// fixed so scenario tests can assert on them; the fast-changing stats wobble
// on a sine wave so the UI visibly lives.
class MockSystemInfoProvider : public SystemInfoProvider {
    Q_OBJECT

public:
    explicit MockSystemInfoProvider(QObject *parent = nullptr);

protected slots:
    void refresh() override;

private:
    int m_tick = 0;
};

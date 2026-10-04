#pragma once

#include "ServiceProvider.h"

// Deterministic set of units typical for an embedded device. Actions apply
// after a short delay so the UI's pending state is visible and testable.
class MockServiceProvider : public ServiceProvider {
    Q_OBJECT

public:
    explicit MockServiceProvider(QObject *parent = nullptr);

    void startUnit(const QString &unit) override;
    void stopUnit(const QString &unit) override;
    void restartUnit(const QString &unit) override;

protected slots:
    void refresh() override {}

private:
    void applyAction(const QString &unit, const QString &action);
};

pragma Singleton
import QtQml

QtObject {
    id: root

    enum MachineState {
        Idle,
        Processing,
        Error,
        Maintenance
    }

    enum Tab {
        Dashboard,
        Alarms,
        Diagnostics,
        Settings
    }

    enum PendingAction {
        NoAction,
        Reset,
        ToggleMaintenance
    }
}

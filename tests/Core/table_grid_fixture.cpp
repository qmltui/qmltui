#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <array>

// Fictional feature comparison of two made-up terminal toolkits. Long cells
// exercise clipping and alignment; the first column carries the row labels
// that table_grid_scenario.js clicks and searches for.
class ComparisonModel final : public QAbstractTableModel
{
public:
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : int(rows.size()); }
    int columnCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : 3; }
    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && role == Qt::DisplayRole ? QVariant(rows[index.row()][index.column()]) : QVariant();
    }
private:
    const std::array<QStringList, 11> rows{{
        {"Approach", "Unified paint loop over a cell grid, widgets drawn via a custom paint device", "Retained item tree with a scene graph of terminal cells"},
        {"API users write", "Declarative markup plus a scripting layer", "Plain C++ with a small builder API"},
        {"Started", "2019-03-04", "2021-11-17"},
        {"Commits", "1204, monthly releases", "358, tagged on demand"},
        {"Platforms", "Linux and BSD terminals", "Linux, macOS, Windows Terminal, and a browser playground"},
        {"Build", "Meson, shared library, pkg-config file", "CMake package with Core, Widgets, and Image components"},
        {"Images", "Sixel and half-block fallback", "Kitty, sixel, and half-block"},
        {"Testing", "Golden-frame snapshot tests", "Scripted scenarios with committed frames"},
        {"Licence", "MIT", "Apache-2.0"},
        {"Status", "Stable 2.x line, long-term branch maintained", "Beta, API frozen for 1.0"},
        {"Public traction", "Packaged by three distributions", "Used in two internal dashboards"}
    }};
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ComparisonModel model;
    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"tableModel", QVariant::fromValue(static_cast<QObject *>(&model))}});
    engine.loadData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import Qamui
        Terminal {
            id: root
            required property var tableModel
            TableView {
                id: table
                model: root.tableModel
                width: parent.width
                height: 25
                focus: true
                showColumnGrid: true
                showRowGrid: true
                showOuterBorder: true
                selectionColor: "#00ff00"
                selectionBackgroundColor: "#102030"
                columns: [
                    { title: "", width: 17, padding: 1 },
                    { title: "Lantern", width: 81, padding: 1, headerAlign: "center" },
                    { title: "Beacon", width: 62, padding: 1, headerAlign: "center" }
                ]
            }
            Keys.onPressed: event => {
                if (event.key === Qt.Key_R) table.height = 9
                else if (event.key === Qt.Key_W) { table.height = 25; table.selectedRow = -1 }
                else if (event.key === Qt.Key_G) table.showRowGrid = !table.showRowGrid
                else if (event.key === Qt.Key_C) table.showColumnGrid = !table.showColumnGrid
                else if (event.key === Qt.Key_B) table.showOuterBorder = !table.showOuterBorder
                else if (event.key === Qt.Key_D) table.gridStyle = "double"
                else if (event.key === Qt.Key_A) table.gridStyle = "ascii"
                else if (event.key === Qt.Key_S) table.gridStyle = "single"
                else if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) Qt.quit()
                else return
                event.accepted = true
            }
        }
    )");
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}

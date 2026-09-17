#ifndef VIRTUAL_MACHINE_GUI_HPP
#define VIRTUAL_MACHINE_GUI_HPP
#include "mips/controller.hpp"
#include <QWidget>
#include <deque>
class QPlainTextEdit;
class QTableView;
class QStandardItemModel;
class QLineEdit;
class QPushButton;
class QTimer;
// All Qt objects stay on the GUI thread. The worker only returns value snapshots.
class VirtualMachineGUI : public QWidget {
    Q_OBJECT
public:
    explicit VirtualMachineGUI(QWidget* parent = nullptr);
    ~VirtualMachineGUI() override;
    void load(QString filename);
signals:
    void commandCompleted();
    void stepped();
private:
    enum class Request { Step, Run, Pause, Poll };
    struct Pending {
        Request kind;
        std::future<mips::Snapshot> result;
        Pending(Request operation, std::future<mips::Snapshot> future);
    };
    void request(Request operation);
    void collect();
    void render();
    void updateButtons();
    void highlightCurrent();
    QPlainTextEdit* text_;
    QTableView* registers_;
    QTableView* memory_;
    QLineEdit* status_;
    QPushButton* step_;
    QPushButton* run_;
    QPushButton* pause_;
    QStandardItemModel* registersModel_;
    QStandardItemModel* memoryModel_;
    QTimer* timer_;
    mips::ExecutionController controller_;
    mips::Machine displayed_;
    std::deque<Pending> pending_;
    bool running_ = false;
    unsigned userRequests_ = 0;
    QString diagnostic_;
};
#endif

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
class QLabel;
class QTimer;
namespace mips { class MemoryModel; }
// GUI state is read only outside the GUI thread; worker replies are bounded values.
class VirtualMachineGUI : public QWidget {
    Q_OBJECT
public:
    explicit VirtualMachineGUI(QWidget* parent = nullptr, mips::ControllerOptions options = mips::ControllerOptions());
    ~VirtualMachineGUI() override;
    void load(QString filename);
    bool isReady() const;
signals:
    void commandCompleted();
    void stepped();
private:
    struct Pending {
        mips::CommandKind kind;
        std::future<mips::Reply> result;
        Pending(mips::CommandKind operation, std::future<mips::Reply> future);
    };
    void request(mips::CommandKind operation);
    void poll();
    void collect();
    void render();
    void updateButtons();
    void highlightCurrent();
    void failed(const std::exception& error);
    void memoryScrolled();
    void jumpMemory();
    mips::MemoryWindow window() const;
    QPlainTextEdit* text_;
    QTableView* registers_;
    QTableView* memory_;
    QLineEdit* status_;
    QLabel* executionStatus_;
    QLineEdit* memoryAddress_;
    QLineEdit* target_;
    QPushButton* step_;
    QPushButton* run_;
    QPushButton* pause_;
    QPushButton* reset_;
    QPushButton* reload_;
    QPushButton* until_;
    QStandardItemModel* registersModel_;
    mips::MemoryModel* memoryModel_;
    QTimer* completionTimer_;
    QTimer* refreshTimer_;
    mips::ExecutionController controller_;
    mips::DebugState displayed_;
    std::array<uint32_t,35> previousRegisters_{};
    bool hasPrevious_ = false;
    uint64_t previousSteps_ = 0;
    std::deque<Pending> pending_;
    bool running_ = false, controllerFailed_ = false, rendering_ = false, windowDirty_ = false;
    unsigned userRequests_ = 0;
    uint64_t windowBase_ = 0;
    mips::StopReason stopReason_ = mips::StopReason::Paused;
    QString diagnostic_, filename_;
};
#endif

#include "virtual_machine_gui.hpp"
#include "gui/source_editor.hpp"
#include "gui/memory_model.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "mips/numbers.hpp"
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QColor>
#include <QBrush>
#include <QTextCursor>
#include <QTextEdit>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBlock>
#include <QScrollBar>
#include <QTimer>
#include <QShortcut>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <utility>
namespace {
QString hex(uint32_t value) { return QString::fromStdString(mips::hexValue(value)); }
void setCell(QStandardItemModel* model, int row, int column, const QString& value) {
    const auto index = model->index(row,column);
    if (model->data(index).toString() != value) model->setData(index,value);
}
}
VirtualMachineGUI::Pending::Pending(mips::CommandKind operation, std::future<mips::Reply> future)
    : kind(operation), result(std::move(future)) {}
VirtualMachineGUI::VirtualMachineGUI(QWidget* parent, mips::ControllerOptions options)
    : QWidget(parent), controller_(mips::Machine(),std::move(options)) {
    setWindowTitle("MIPS Simulator — Instruction-level debugger");
    text_ = new mips::SourceEditor(this); text_->setObjectName("text"); text_->setReadOnly(true);
    registers_ = new QTableView(this); registers_->setObjectName("registers");
    memory_ = new QTableView(this); memory_->setObjectName("memory");
    status_ = new QLineEdit(this); status_->setObjectName("status"); status_->setReadOnly(true);
    executionStatus_ = new QLabel(this); executionStatus_->setObjectName("executionState");
    auto button = [this](const char* name,const char* caption) {
        auto item = new QPushButton(QString::fromLatin1(caption),this); item->setObjectName(QString::fromLatin1(name)); return item;
    };
    step_ = button("step","Step"); run_ = button("run","Run"); pause_ = button("break","Break");
    reset_ = button("reset","Reset"); reload_ = button("reload","Reload"); until_ = button("runTo","Run to");
    auto open = button("open","Open..."); auto go = button("memoryGo","Go");
    target_ = new QLineEdit(this); target_->setObjectName("runTarget"); target_->setPlaceholderText("Label or instruction index");
    memoryAddress_ = new QLineEdit("0x0",this); memoryAddress_->setObjectName("memoryAddress");
    memoryAddress_->setToolTip("Decimal or hexadecimal byte address");
    registersModel_ = new QStandardItemModel(35,3,this);
    registersModel_->setHorizontalHeaderLabels({"Number","Alias","Value (Hex)"});
    memoryModel_ = new mips::MemoryModel(this);
    registers_->setModel(registersModel_); memory_->setModel(memoryModel_);
    const QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    text_->setFont(fixed); registers_->setFont(fixed); memory_->setFont(fixed);
    for (auto view : {registers_,memory_}) {
        view->setEditTriggers(QAbstractItemView::NoEditTriggers); view->setAlternatingRowColors(true);
        view->horizontalHeader()->setStretchLastSection(true); view->verticalHeader()->hide();
    }
    registers_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    registers_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    memory_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    setCell(registersModel_,0,1,"$pc"); setCell(registersModel_,1,1,"$hi"); setCell(registersModel_,2,1,"$lo");
    for (unsigned i=0;i<32;++i) {
        setCell(registersModel_,static_cast<int>(i)+3,0,"$"+QString::number(i));
        setCell(registersModel_,static_cast<int>(i)+3,1,"$"+QString::fromLatin1(mips::registerAlias(i)));
    }
    auto layout = new QGridLayout(this);
    layout->addWidget(open,0,0); layout->addWidget(reload_,0,1); layout->addWidget(reset_,0,2);
    layout->addWidget(executionStatus_,0,3,1,3);
    layout->addWidget(new QLabel("Assembly",this),1,0,1,2);
    layout->addWidget(new QLabel("Registers",this),1,2,1,2);
    layout->addWidget(new QLabel("Memory",this),1,4,1,2);
    layout->addWidget(text_,2,0,1,2); layout->addWidget(registers_,2,2,1,2); layout->addWidget(memory_,2,4,1,2);
    layout->addWidget(step_,3,0); layout->addWidget(run_,3,1); layout->addWidget(pause_,3,2);
    layout->addWidget(memoryAddress_,3,4); layout->addWidget(go,3,5);
    layout->addWidget(target_,4,0,1,2); layout->addWidget(until_,4,2);
    layout->addWidget(new QLabel("F10 Step  |  F5 Run  |  Shift+F5 Break",this),4,3,1,3);
    layout->addWidget(status_,5,0,1,6);
    layout->setColumnStretch(0,3); layout->setColumnStretch(1,2);
    layout->setColumnStretch(2,2); layout->setColumnStretch(3,2);
    layout->setColumnStretch(4,2); layout->setColumnStretch(5,1);
    layout->setRowStretch(2,1);
    connect(step_,&QPushButton::clicked,this,[this] { request(mips::CommandKind::Step); });
    connect(run_,&QPushButton::clicked,this,[this] { request(mips::CommandKind::Run); });
    connect(pause_,&QPushButton::clicked,this,[this] { request(mips::CommandKind::Pause); });
    connect(reset_,&QPushButton::clicked,this,[this] { request(mips::CommandKind::Reset); });
    connect(until_,&QPushButton::clicked,this,[this] { request(mips::CommandKind::RunUntil); });
    connect(reload_,&QPushButton::clicked,this,[this] { load(filename_); });
    connect(open,&QPushButton::clicked,this,[this] {
        const auto path = QFileDialog::getOpenFileName(this,"Open MIPS assembly",filename_,"Assembly (*.asm);;All files (*)");
        if (!path.isEmpty()) load(path);
    });
    connect(go,&QPushButton::clicked,this,&VirtualMachineGUI::jumpMemory);
    connect(memoryAddress_,&QLineEdit::returnPressed,this,&VirtualMachineGUI::jumpMemory);
    connect(memory_->verticalScrollBar(),&QScrollBar::valueChanged,this,[this] { memoryScrolled(); });
    for (const auto& binding : {std::make_pair(QString("F10"),step_),std::make_pair(QString("F5"),run_),
         std::make_pair(QString("Shift+F5"),pause_),std::make_pair(QString("Ctrl+R"),reset_),
         std::make_pair(QString("Ctrl+O"),open)}) {
        auto shortcut = new QShortcut(QKeySequence(binding.first),this);
        connect(shortcut,&QShortcut::activated,binding.second,&QPushButton::click);
    }
    completionTimer_ = new QTimer(this); completionTimer_->setObjectName("completionTimer"); completionTimer_->setInterval(5);
    connect(completionTimer_,&QTimer::timeout,this,&VirtualMachineGUI::collect);
    refreshTimer_ = new QTimer(this); refreshTimer_->setObjectName("refreshTimer"); refreshTimer_->setInterval(33);
    connect(refreshTimer_,&QTimer::timeout,this,&VirtualMachineGUI::poll);
    render();
}
VirtualMachineGUI::~VirtualMachineGUI() {
    completionTimer_->stop(); refreshTimer_->stop(); controller_.shutdown();
}
bool VirtualMachineGUI::isReady() const {
    return !controllerFailed_ && displayed_.status != mips::Status::Error && diagnostic_.isEmpty();
}
mips::MemoryWindow VirtualMachineGUI::window() const {
    const std::size_t size = displayed_.memorySize;
    const uint64_t base = size == 0 ? 0 : std::min<uint64_t>(windowBase_,size-1);
    return mips::MemoryWindow(base,std::min<std::size_t>(1024,size-static_cast<std::size_t>(base)));
}
void VirtualMachineGUI::failed(const std::exception& error) {
    diagnostic_ = "Error:1: " + QString::fromUtf8(error.what());
    running_ = false; controllerFailed_ = true; userRequests_ = 0; pending_.clear();
    completionTimer_->stop(); refreshTimer_->stop(); render();
}
void VirtualMachineGUI::load(QString filename) {
    filename_ = filename;
    // All submission and completion errors, including pause/reset, are contained.
    try {
        controller_.request(mips::CommandKind::Pause).get();
        // FIFO Pause acknowledges all earlier commands before stale GUI replies
        // are discarded. A consumer future does not own the worker's promise;
        // releasing it cannot cause broken_promise in promise::set_value().
        pending_.clear(); userRequests_ = 0; running_ = false; windowDirty_ = false;
        completionTimer_->stop(); refreshTimer_->stop();
        displayed_ = controller_.replace(mips::Machine()).get().state;
    } catch (const std::exception& error) { failed(error); return; }
    text_->clear(); diagnostic_.clear(); windowBase_ = 0; hasPrevious_ = false;
    stopReason_ = mips::StopReason::Paused;
    try {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("cannot open assembly file");
        const auto content = file.read(static_cast<qint64>(mips::MaxSourceBytes)+1);
        if (file.error() != QFile::NoError) throw std::runtime_error("failed to read assembly file");
        if (static_cast<std::size_t>(content.size()) > mips::MaxSourceBytes) throw std::runtime_error("assembly file exceeds 4 MiB");
        text_->setPlainText(QString::fromLatin1(content));
        std::istringstream source(std::string(content.constData(),static_cast<std::size_t>(content.size())));
        Parse parser;
        if (!parser.parse(tokenize(source))) throw std::runtime_error(parser.error());
        auto state = parser.getVM();
        const auto bytes = state.memSize();
        try { displayed_ = controller_.replace(std::move(state),mips::MemoryWindow(0,std::min<std::size_t>(1024,bytes))).get().state; }
        catch (const std::exception& error) { failed(error); return; }
    } catch (const std::exception& error) {
        diagnostic_ = QString::fromUtf8(error.what());
        if (!diagnostic_.startsWith("Error:")) diagnostic_.prepend("Error:1: ");
    }
    render();
}
void VirtualMachineGUI::request(mips::CommandKind operation) {
    if (controllerFailed_ || userRequests_ != 0) return;
    if ((operation == mips::CommandKind::Step || operation == mips::CommandKind::Run || operation == mips::CommandKind::RunUntil) && !isReady()) return;
    if ((operation == mips::CommandKind::Step || operation == mips::CommandKind::Run || operation == mips::CommandKind::RunUntil) && running_) return;
    uint32_t target = 0;
    if (operation == mips::CommandKind::RunUntil) {
        const auto name = target_->text().trimmed().toStdString();
        if (displayed_.program && displayed_.program->textLabels.count(name)) target = static_cast<uint32_t>(displayed_.program->textLabels.at(name));
        else if (!mips::parseAddress(name,target)) { status_->setText("Error: unknown run-to label"); return; }
    }
    try {
        pending_.emplace_back(operation,controller_.request(operation,window(),target));
        ++userRequests_; completionTimer_->start(); updateButtons();
    } catch (const std::exception& error) { failed(error); }
}
void VirtualMachineGUI::poll() {
    if (controllerFailed_ || !pending_.empty() || (!running_ && !windowDirty_)) return;
    try {
        pending_.emplace_back(mips::CommandKind::Observe,controller_.request(mips::CommandKind::Observe,window()));
        windowDirty_ = false; completionTimer_->start();
    } catch (const std::exception& error) { failed(error); }
}
void VirtualMachineGUI::collect() {
    while (!pending_.empty() && pending_.front().result.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        const auto kind = pending_.front().kind;
        mips::Reply reply;
        try { reply = pending_.front().result.get(); }
        catch (const std::exception& error) { failed(error); return; }
        pending_.pop_front();
        if (kind != mips::CommandKind::Observe) --userRequests_;
        displayed_ = std::move(reply.state); running_ = reply.running; stopReason_ = reply.reason;
        diagnostic_.clear();
        if (reply.accepted) render();
        else { render(); status_->setText(QString::fromStdString(reply.message)); }
        if (kind != mips::CommandKind::Observe) emit commandCompleted();
        if (kind == mips::CommandKind::Step) emit stepped();
    }
    if (pending_.empty()) {
        completionTimer_->stop();
        if (windowDirty_) poll();
    }
    if (running_) { if (!refreshTimer_->isActive()) refreshTimer_->start(); }
    else refreshTimer_->stop();
}
void VirtualMachineGUI::memoryScrolled() {
    if (rendering_ || displayed_.memorySize == 0) return;
    const int row = memory_->rowAt(0);
    if (row < 0) return;
    const uint64_t base = (static_cast<uint64_t>(row)/512)*512;
    if (base == windowBase_) return;
    windowBase_ = base; windowDirty_ = true;
    poll();
}
void VirtualMachineGUI::jumpMemory() {
    uint32_t address = 0;
    if (!mips::parseAddress(memoryAddress_->text().trimmed().toStdString(),address) || address >= displayed_.memorySize) {
        status_->setText("Error: memory address out of bounds"); return;
    }
    memory_->scrollTo(memoryModel_->index(static_cast<int>(address),0),QAbstractItemView::PositionAtTop);
    memory_->setCurrentIndex(memoryModel_->index(static_cast<int>(address),1));
    windowBase_ = (address/512)*512; windowDirty_ = true; poll();
}
void VirtualMachineGUI::updateButtons() {
    const bool available = !controllerFailed_ && userRequests_ == 0;
    step_->setEnabled(available && isReady() && !running_);
    run_->setEnabled(available && isReady() && !running_);
    until_->setEnabled(available && isReady() && !running_);
    pause_->setEnabled(available && running_);
    reset_->setEnabled(available && static_cast<bool>(displayed_.program));
    reload_->setEnabled(available && !filename_.isEmpty());
}
void VirtualMachineGUI::highlightCurrent() {
    QList<QTextEdit::ExtraSelection> selections;
    if (displayed_.program && displayed_.program->hasEntry && displayed_.pc < displayed_.program->instructions.size()) {
        const auto line = displayed_.program->instructions[displayed_.pc].line;
        if (line > 0 && line <= static_cast<std::size_t>(text_->document()->blockCount())) {
            const auto block = text_->document()->findBlockByNumber(static_cast<int>(line-1));
            if (block.isValid()) {
                QTextEdit::ExtraSelection selection;
                selection.format.setBackground(QColor(Qt::yellow).lighter(160));
                selection.format.setProperty(QTextFormat::FullWidthSelection,true);
                selection.cursor = QTextCursor(block); selections.append(selection);
            }
        }
    }
    text_->setExtraSelections(selections);
}
void VirtualMachineGUI::render() {
    rendering_ = true;
    std::array<uint32_t,35> values{};
    values[0] = displayed_.pc; values[1] = displayed_.hi; values[2] = displayed_.lo;
    std::copy(displayed_.registers.begin(),displayed_.registers.end(),values.begin()+3);
    const bool newBoundary = !hasPrevious_ || previousSteps_ != displayed_.executed;
    for (unsigned i=0;i<35;++i) {
        setCell(registersModel_,static_cast<int>(i),2,hex(values[i]));
        if (newBoundary) {
            const bool changed = hasPrevious_ && previousRegisters_[i] != values[i];
            registersModel_->setData(registersModel_->index(static_cast<int>(i),2),
                changed ? QVariant(QBrush(QColor(220,237,255))) : QVariant(),Qt::BackgroundRole);
        }
    }
    previousRegisters_ = values; previousSteps_ = displayed_.executed; hasPrevious_ = true;
    memoryModel_->update(displayed_);
    status_->setText(!diagnostic_.isEmpty() ? diagnostic_ :
        (displayed_.status == mips::Status::Error ? QString::fromStdString(displayed_.error) : "Ok"));
    const QString mode = running_ ? "Running" : stopReason_ == mips::StopReason::TargetReached ? "Target reached" :
        stopReason_ == mips::StopReason::StepLimit ? "Instruction budget reached" : displayed_.status == mips::Status::Error ? "Fault / not loaded" : "Paused";
    executionStatus_->setText(mode+"  |  Steps: "+QString::number(static_cast<qulonglong>(displayed_.executed)));
    highlightCurrent(); updateButtons(); rendering_ = false;
}

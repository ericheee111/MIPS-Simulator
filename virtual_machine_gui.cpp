#include "virtual_machine_gui.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "mips/numbers.hpp"
#include <QFile>
#include <QFontDatabase>
#include <QColor>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextBlock>
#include <QTimer>
#include <chrono>
#include <sstream>
#include <utility>
namespace {
QString hex(uint32_t value, unsigned digits = 8) {
    return QString::fromStdString(mips::hexValue(value, digits));
}
void setCell(QStandardItemModel* model, int row, int column, const QString& value) {
    const auto index = model->index(row, column);
    if (model->data(index).toString() != value) model->setData(index, value);
}
}
VirtualMachineGUI::Pending::Pending(Request operation, std::future<mips::Snapshot> future)
    : kind(operation), result(std::move(future)) {}
VirtualMachineGUI::VirtualMachineGUI(QWidget* parent) : QWidget(parent) {
    setWindowTitle("MIPS Simulator — Instruction-level debugger");
    text_ = new QPlainTextEdit(this); text_->setObjectName("text"); text_->setReadOnly(true);
    registers_ = new QTableView(this); registers_->setObjectName("registers");
    memory_ = new QTableView(this); memory_->setObjectName("memory");
    status_ = new QLineEdit(this); status_->setObjectName("status"); status_->setReadOnly(true);
    step_ = new QPushButton("Step",this); step_->setObjectName("step");
    run_ = new QPushButton("Run",this); run_->setObjectName("run");
    pause_ = new QPushButton("Break",this); pause_->setObjectName("break");
    registersModel_ = new QStandardItemModel(35,3,this);
    registersModel_->setHorizontalHeaderLabels({"Number", "Alias", "Value (Hex)"});
    memoryModel_ = new QStandardItemModel(0,2,this);
    memoryModel_->setHorizontalHeaderLabels({"Address (Hex)", "Value (Hex)"});
    registers_->setModel(registersModel_); memory_->setModel(memoryModel_);
    const QFont fixed = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    text_->setFont(fixed); registers_->setFont(fixed); memory_->setFont(fixed);
    for (auto view : {registers_, memory_}) {
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->setAlternatingRowColors(true);
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->hide();
    }
    setCell(registersModel_,0,1,"$pc"); setCell(registersModel_,1,1,"$hi"); setCell(registersModel_,2,1,"$lo");
    for (unsigned i=0;i<32;++i) {
        setCell(registersModel_,static_cast<int>(i)+3,0,"$"+QString::number(i));
        setCell(registersModel_,static_cast<int>(i)+3,1,"$"+QString::fromLatin1(mips::registerAlias(i)));
    }
    auto layout = new QGridLayout(this);
    layout->addWidget(new QLabel("Assembly",this),0,0);
    layout->addWidget(new QLabel("Registers",this),0,1);
    layout->addWidget(new QLabel("Memory",this),0,2);
    layout->addWidget(text_,1,0); layout->addWidget(registers_,1,1); layout->addWidget(memory_,1,2);
    layout->addWidget(status_,2,0,1,3);
    layout->addWidget(step_,3,0); layout->addWidget(run_,3,1); layout->addWidget(pause_,3,2);
    layout->setColumnStretch(0,5); layout->setColumnStretch(1,4); layout->setColumnStretch(2,3);
    connect(step_,&QPushButton::clicked,this,[this] { request(Request::Step); });
    connect(run_,&QPushButton::clicked,this,[this] { request(Request::Run); });
    connect(pause_,&QPushButton::clicked,this,[this] { request(Request::Pause); });
    timer_ = new QTimer(this);
    timer_->setInterval(10);
    connect(timer_,&QTimer::timeout,this,&VirtualMachineGUI::collect);
    timer_->start();
    render();
}
VirtualMachineGUI::~VirtualMachineGUI() {
    timer_->stop();
    controller_.shutdown();
}
void VirtualMachineGUI::load(QString filename) {
    // A pause acknowledgement also orders all earlier requests before this load.
    controller_.pause().get();
    pending_.clear(); userRequests_ = 0; running_ = false;
    displayed_ = controller_.load(mips::Machine()).get().machine;
    text_->clear(); diagnostic_.clear();
    try {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Error:1: cannot open assembly file");
        const QByteArray content = file.read(static_cast<qint64>(mips::MaxSourceBytes)+1);
        if (file.error() != QFile::NoError) throw std::runtime_error("Error:1: failed to read assembly file");
        if (static_cast<std::size_t>(content.size()) > mips::MaxSourceBytes)
            throw std::runtime_error("Error:1: assembly file exceeds 4 MiB");
        text_->setPlainText(QString::fromLatin1(content));
        std::istringstream source(std::string(content.constData(), static_cast<std::size_t>(content.size())));
        Parse parser;
        if (!parser.parse(tokenize(source))) throw std::runtime_error(parser.error());
        displayed_ = controller_.load(parser.getVM()).get().machine;
    } catch (const std::exception& error) { diagnostic_ = QString::fromUtf8(error.what()); }
    render();
}
void VirtualMachineGUI::request(Request operation) {
    if (userRequests_ != 0) return;
    if ((operation == Request::Step || operation == Request::Run) && running_) return;
    if (displayed_.getStatus() == mips::Status::Error) return;
    switch (operation) {
    case Request::Step: pending_.emplace_back(operation,controller_.step()); break;
    case Request::Run: pending_.emplace_back(operation,controller_.run()); break;
    case Request::Pause: pending_.emplace_back(operation,controller_.pause()); break;
    case Request::Poll: return;
    }
    ++userRequests_; updateButtons();
}
void VirtualMachineGUI::collect() {
    while (!pending_.empty() && pending_.front().result.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        const Request kind = pending_.front().kind;
        try {
            auto snapshot = pending_.front().result.get();
            displayed_ = std::move(snapshot.machine); running_ = snapshot.running;
            diagnostic_ = QString::fromStdString(snapshot.message);
        } catch (const std::exception& error) {
            diagnostic_ = "Error:1: " + QString::fromUtf8(error.what()); running_ = false;
        }
        pending_.pop_front();
        if (kind != Request::Poll) --userRequests_;
        render();
        if (kind != Request::Poll) emit commandCompleted();
        if (kind == Request::Step) emit stepped();
    }
    if (running_ && pending_.empty()) pending_.emplace_back(Request::Poll,controller_.snapshot());
}
void VirtualMachineGUI::updateButtons() {
    const bool valid = displayed_.getStatus() != mips::Status::Error && diagnostic_.isEmpty();
    step_->setEnabled(valid && !running_ && userRequests_ == 0);
    run_->setEnabled(valid && !running_ && userRequests_ == 0);
    pause_->setEnabled(running_ && userRequests_ == 0);
}
void VirtualMachineGUI::highlightCurrent() {
    QList<QTextEdit::ExtraSelection> selections;
    const auto pc = displayed_.readPC();
    if (displayed_.program() && displayed_.program()->hasEntry && pc < displayed_.getInstrVector().size()) {
        const auto line = displayed_.getInstruction(pc).line;
        if (line > 0 && line <= static_cast<std::size_t>(text_->document()->blockCount())) {
            const QTextBlock block = text_->document()->findBlockByNumber(static_cast<int>(line-1));
            if (block.isValid()) {
                QTextEdit::ExtraSelection selection;
                selection.format.setBackground(QColor(Qt::yellow).lighter(160));
                selection.format.setProperty(QTextFormat::FullWidthSelection,true);
                selection.cursor = QTextCursor(block); // value object, no leaked allocation
                selection.cursor.clearSelection(); selections.append(selection);
            }
        }
    }
    text_->setExtraSelections(selections);
}
void VirtualMachineGUI::render() {
    setCell(registersModel_,0,2,hex(displayed_.readPC()));
    setCell(registersModel_,1,2,hex(displayed_.readHI()));
    setCell(registersModel_,2,2,hex(displayed_.readLO()));
    for (unsigned i=0;i<32;++i) setCell(registersModel_,static_cast<int>(i)+3,2,hex(displayed_.readReg(i)));
    const int size = static_cast<int>(displayed_.memSize());
    if (memoryModel_->rowCount() != size) memoryModel_->setRowCount(size);
    for (int i=0;i<size;++i) {
        setCell(memoryModel_,i,0,hex(static_cast<uint32_t>(i)));
        setCell(memoryModel_,i,1,hex(displayed_.readMEM(static_cast<uint32_t>(i),1),2));
    }
    status_->setText(!diagnostic_.isEmpty() ? diagnostic_ :
        (displayed_.getStatus() == mips::Status::Error ? QString::fromStdString(displayed_.error()) : "Ok"));
    highlightCurrent(); updateButtons();
}

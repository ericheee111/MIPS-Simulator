#include "virtual_machine_gui.hpp"
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QTextBlock>
#include <QPixmap>
#include <QTimer>
#include <QDir>
#include <QLabel>
#include <QFontMetrics>
#include <QFontDatabase>
#include <QBrush>
#include <atomic>
#include "gui/memory_model.hpp"
class GuiRegression : public QObject {
    Q_OBJECT
private:
    QTemporaryDir directory;
    QString write(const QByteArray& content) {
        const QString path=directory.filePath("program.asm");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size()) qFatal("failed to create test assembly");
        return path;
    }
    void capture(VirtualMachineGUI& gui, const QString& name) {
        const auto imageDirectory = QString::fromLocal8Bit(qgetenv("MIPS_DEMO_DIR"));
        if (imageDirectory.isEmpty()) return;
        QVERIFY(QDir().mkpath(imageDirectory));
        gui.resize(1200,700); gui.show(); QApplication::processEvents();
        QVERIFY(gui.grab().save(imageDirectory+"/"+name+".png"));
    }
    bool click(VirtualMachineGUI& gui, const char* name) {
        auto button=gui.findChild<QPushButton*>(name);
        if (!button || !button->isEnabled()) return false;
        QSignalSpy spy(&gui,&VirtualMachineGUI::commandCompleted);
        button->click();
        return !spy.isEmpty() || spy.wait(2000);
    }
    QString value(VirtualMachineGUI& gui,int row) {
        auto model=gui.findChild<QTableView*>("registers")->model();
        return model->data(model->index(row,2)).toString();
    }
private slots:
    void initTestCase() {
        // A passing state assertion is not sufficient if the platform cannot render text.
        QVERIFY2(!QFontDatabase().families().isEmpty(), "No fonts available in the Qt platform backend");
        const QFontMetrics metrics(QApplication::font());
        QVERIFY(metrics.inFont(QChar('M')));
        QVERIFY(metrics.boundingRect("MIPS 0x00000000").width() > 0);
    }
    void loadAndStep() {
        VirtualMachineGUI gui;
        gui.load(write("# comment\n.text\nmain:\nli $t0, 7# adjacent\nend:\nj end\n"));
        QCOMPARE(gui.findChild<QLineEdit*>("status")->text(),QString("Ok"));
        QCOMPARE(value(gui,0),QString("0x00000000")); QCOMPARE(value(gui,11),QString("0x00000000"));
        auto text=gui.findChild<QPlainTextEdit*>("text");
        QCOMPARE(text->extraSelections().size(),1);
        QCOMPARE(text->extraSelections().first().cursor.blockNumber(),3);
        QVERIFY(click(gui,"step"));
        QCOMPARE(value(gui,11),QString("0x00000007"));
        QCOMPARE(text->extraSelections().first().cursor.blockNumber(),5);
        QVERIFY(gui.findChild<QTableView*>("memory")->model()->rowCount() == 1024);
        auto memoryView = gui.findChild<QTableView*>("memory");
        QVERIFY(memoryView->columnWidth(0) >= QFontMetrics(memoryView->font()).boundingRect("0x00000000").width());
        QVERIFY(gui.findChild<QWidget*>("lineNumbers"));
        const QString image = QString::fromLocal8Bit(qgetenv("MIPS_GUI_SCREENSHOT"));
        if (!image.isEmpty()) {
            gui.resize(1200,700); gui.show(); QApplication::processEvents();
            QVERIFY(gui.grab().save(image));
        }
    }
    void missingOrMalformedFiles() {
        VirtualMachineGUI gui;
        for (const QString& path : {directory.filePath("missing"),write(".text\nmain:\nli $t0, +\n")}) {
            gui.load(path);
            QVERIFY(gui.findChild<QLineEdit*>("status")->text().startsWith("Error:"));
            QVERIFY(!gui.findChild<QPushButton*>("step")->isEnabled());
            QVERIFY(gui.findChild<QPlainTextEdit*>("text")->extraSelections().isEmpty());
        }
        gui.load(write(""));
        QVERIFY(gui.findChild<QLineEdit*>("status")->text().startsWith("Error:"));
    }
    void runPauseAndReload() {
        VirtualMachineGUI gui;
        gui.load(write(".text\nmain:\nj main\n"));
        QVERIFY(click(gui,"run"));
        QVERIFY(!gui.findChild<QPushButton*>("step")->isEnabled());
        QVERIFY(!gui.findChild<QPushButton*>("run")->isEnabled());
        QVERIFY(gui.findChild<QPushButton*>("break")->isEnabled());
        QVERIFY(click(gui,"break"));
        QVERIFY(gui.findChild<QPushButton*>("step")->isEnabled());
        QVERIFY(click(gui,"run"));
        gui.load(write(".text\nnop\nmain:\nli $t0, 9\n"));
        QCOMPARE(value(gui,0),QString("0x00000001"));
        QCOMPARE(value(gui,11),QString("0x00000000"));
        QVERIFY(click(gui,"step")); QCOMPARE(value(gui,11),QString("0x00000009"));
    }
    void reloadWithUncollectedObservation() {
        VirtualMachineGUI gui;
        gui.load(write(".text\nmain:\nj main\n"));
        QVERIFY(click(gui,"run"));
        auto refresh = gui.findChild<QTimer*>("refreshTimer");
        auto completion = gui.findChild<QTimer*>("completionTimer");
        QVERIFY(refresh); QVERIFY(completion);
        refresh->stop();
        // Queue one real observation without letting the GUI collect its future.
        // load() synchronously waits for FIFO Pause before dropping stale replies.
        QVERIFY(QMetaObject::invokeMethod(refresh,"timeout",Qt::DirectConnection));
        QVERIFY(completion->isActive());
        gui.load(write(".text\nmain:\nli $t0, 9\nend:\nj end\n"));
        QVERIFY(gui.isReady());
        QCOMPARE(value(gui,0),QString("0x00000000"));
        QCOMPARE(value(gui,11),QString("0x00000000"));
        QVERIFY(click(gui,"step"));
        QCOMPARE(value(gui,11),QString("0x00000009"));
        QVERIFY(!completion->isActive());
        QVERIFY(!refresh->isActive());
    }
    void automaticFaultAndRecovery() {
        VirtualMachineGUI gui;
        gui.load(write(".text\nmain:\nlw $t0, 1024\n"));
        QVERIFY(click(gui,"run"));
        QTRY_VERIFY(gui.findChild<QLineEdit*>("status")->text().startsWith("Error:"));
        QVERIFY(!gui.findChild<QPushButton*>("break")->isEnabled());
        QVERIFY(!gui.findChild<QPushButton*>("step")->isEnabled());
        gui.load(write(".text\nmain:\nnop\n"));
        QCOMPARE(gui.findChild<QLineEdit*>("status")->text(),QString("Ok"));
        QVERIFY(click(gui,"step"));
        QVERIFY(gui.findChild<QPlainTextEdit*>("text")->extraSelections().isEmpty());
        QVERIFY(click(gui,"step")); // next fetch reports end-of-program fault
        QVERIFY(gui.findChild<QLineEdit*>("status")->text().startsWith("Error:"));
    }
    void resetRunToJumpAndIdleTimers() {
        VirtualMachineGUI gui;
        gui.load(write(".text\nmain:\nli $t0, 7\nend:\nj end\n"));
        gui.findChild<QLineEdit*>("runTarget")->setText("end");
        QVERIFY(click(gui,"runTo"));
        QTRY_VERIFY(gui.findChild<QPushButton*>("step")->isEnabled());
        QCOMPARE(value(gui,11),QString("0x00000007"));
        QVERIFY(gui.findChild<QLabel*>("executionState")->text().startsWith("Target reached"));
        QVERIFY(gui.findChild<QTableView*>("registers")->model()->data(
            gui.findChild<QTableView*>("registers")->model()->index(11,2),Qt::BackgroundRole).isValid());
        QVERIFY(click(gui,"reset"));
        QCOMPARE(value(gui,11),QString("0x00000000")); QCOMPARE(value(gui,0),QString("0x00000000"));
        QVERIFY(!gui.findChild<QTimer*>("completionTimer")->isActive());
        QVERIFY(!gui.findChild<QTimer*>("refreshTimer")->isActive());
        gui.findChild<QLineEdit*>("memoryAddress")->setText("0x3ff");
        gui.findChild<QPushButton*>("memoryGo")->click();
        QTRY_VERIFY(!gui.findChild<QTimer*>("completionTimer")->isActive());
        QCOMPARE(gui.findChild<QTableView*>("memory")->currentIndex().row(),1023);
        gui.findChild<QLineEdit*>("memoryAddress")->setText("0xffffffff");
        gui.findChild<QPushButton*>("memoryGo")->click();
        QVERIFY(gui.findChild<QLineEdit*>("status")->text().contains("out of bounds"));
        QVERIFY(click(gui,"step")); // an input error does not poison the machine
    }
    void lazyMemoryModelStorageAndNotifications() {
        mips::MemoryModel model;
        mips::DebugState state; state.memorySize = mips::MaxMemoryBytes;
        state.memoryBase = state.memorySize-1024; state.memory.assign(1024,9);
        model.update(state);
        QCOMPARE(model.rowCount(),static_cast<int>(mips::MaxMemoryBytes));
        QCOMPARE(model.cachedBytes(),std::size_t(1024));
        QCOMPARE(model.data(model.index(model.rowCount()-1,1)).toString(),QString("0x09"));
        QCOMPARE(model.data(model.index(0,1)).toString(),QString("--"));
        QSignalSpy changes(&model,&QAbstractItemModel::dataChanged);
        model.update(state); QCOMPARE(changes.size(),0);
        state.memory[7] = 3; model.update(state); QCOMPARE(changes.size(),1);
        state.memoryBase = 0; model.update(state); QCOMPARE(changes.size(),3);
        QVERIFY_EXCEPTION_THROWN((state.memoryBase = UINT64_MAX,model.update(state)),std::invalid_argument);
    }
#ifdef MIPS_ENABLE_TEST_HOOKS
    void workerAndSubmissionFaultsStayInsideGui() {
        for (auto where : {mips::FaultPoint::BeforeCommand,mips::FaultPoint::BeforeReply}) {
            std::atomic<bool> armed{false}; mips::ControllerOptions options;
            options.faultHook = [&](mips::FaultPoint point) {
                if (point == where && armed.exchange(false)) throw std::runtime_error("controlled GUI worker failure");
            };
            VirtualMachineGUI gui(nullptr,options);
            const auto path = write(".text\nmain:\nj main\n"); gui.load(path);
            QVERIFY(gui.isReady()); armed = true;
            gui.findChild<QPushButton*>("step")->click();
            QTRY_VERIFY(gui.findChild<QLineEdit*>("status")->text().contains("controlled GUI worker failure"));
            QVERIFY(!gui.findChild<QPushButton*>("run")->isEnabled());
            QVERIFY(!gui.findChild<QTimer*>("completionTimer")->isActive());
            QVERIFY(!gui.findChild<QTimer*>("refreshTimer")->isActive());
            gui.load(path); // submit now throws; load must catch it, not unwind a Qt callback
            QVERIFY(gui.findChild<QLineEdit*>("status")->text().contains("controller is stopped"));
        }
    }
#endif
    void deterministicDemo() {
        VirtualMachineGUI gui;
        gui.load(QString::fromUtf8(MIPS_SOURCE_DIR)+"/examples/sum_of_squares.asm");
        QVERIFY(gui.isReady()); capture(gui,"01-loaded");
        QVERIFY(click(gui,"step")); capture(gui,"02-stepped");
        gui.findChild<QLineEdit*>("runTarget")->setText("end");
        QVERIFY(click(gui,"runTo"));
        QTRY_VERIFY(gui.findChild<QPushButton*>("step")->isEnabled());
        auto model = gui.findChild<QTableView*>("memory")->model();
        QCOMPARE(model->data(model->index(4,1)).toString(),QString("0x81"));
        QCOMPARE(model->data(model->index(5,1)).toString(),QString("0x01"));
        capture(gui,"03-result");
    }
    void closeWhileRunning() {
        for (int i=0;i<10;++i) {
            VirtualMachineGUI gui;
            gui.load(write(".text\nmain:\nj main\n"));
            QVERIFY(click(gui,"run"));
        } // destructors must stop/join without needing an event-loop callback
    }
};
QTEST_MAIN(GuiRegression)
#include "test_gui.moc"

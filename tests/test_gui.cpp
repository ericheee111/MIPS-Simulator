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
#include <QFontMetrics>
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
        gui.resize(1200,700); gui.show(); QApplication::processEvents();
        auto memoryView = gui.findChild<QTableView*>("memory");
        QVERIFY(memoryView->columnWidth(0) >= QFontMetrics(memoryView->font()).boundingRect("0x00000000").width());
        const QString image = QString::fromLocal8Bit(qgetenv("MIPS_GUI_SCREENSHOT"));
        if (!image.isEmpty()) {
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

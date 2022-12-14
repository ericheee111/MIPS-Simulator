#include <QTest>
#include <QPlainTextEdit>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QDebug>
#include <QStandardItemModel>

#include "virtual_machine_gui.hpp"

#include "test_config.hpp"

class VirtualMachineGUITest : public QObject {
	Q_OBJECT

private slots:

	void initTestCase();
 
  // TODO: implement additional tests here
	void test01();
	void test02();
	void test03();
	void test04();
	void test05();
	void test06();
	void test07();
	void test08();
	void test09();
	void test10();
	void test11();
	void test12();
	void test13();
	void test14();
	void test15();
	void test16();
	void test17();
	void test18();
	void test19();
	void test20();

	void test21();

private:
	VirtualMachineGUI widget;
  
};

// this section just verifies the object names and API
void VirtualMachineGUITest::initTestCase(){

  widget.load(QString::fromStdString(TEST_FILE_DIR + "/vm/test00.asm"));

  auto textWidget = widget.findChild<QPlainTextEdit *>("text");
  QVERIFY2(textWidget, "Could not find QPLainText widget");

  auto registerViewWidget = widget.findChild<QTableView *>("registers");
  QVERIFY2(registerViewWidget, "Could not find QTableView widget for registers");

  auto memoryViewWidget = widget.findChild<QTableView *>("memory");
  QVERIFY2(memoryViewWidget, "Could not find QTableView widget for memory");

  auto statusViewWidget = widget.findChild<QLineEdit *>("status");
  QVERIFY2(statusViewWidget, "Could not find QLineEdit widget for status");

  auto stepButtonWidget = widget.findChild<QPushButton *>("step");
  QVERIFY2(stepButtonWidget, "Could not find QTableView widget for memory");

  auto runButtonWidget = widget.findChild<QPushButton *>("run");
  QVERIFY2(runButtonWidget, "Could not find QTableView widget for memory");

  auto breakButtonWidget = widget.findChild<QPushButton *>("break");
  QVERIFY2(breakButtonWidget, "Could not find QTableView widget for memory");
}

void VirtualMachineGUITest::test01() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test01.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	QVERIFY2(textWidget, "Could not find QPLainText widget");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	QVERIFY2(registerViewWidget, "Could not find QTableView widget for registers");
	auto memoryViewWidget = widget.findChild<QTableView*>("memory");
	QVERIFY2(memoryViewWidget, "Could not find QTableView widget for memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	QVERIFY2(statusViewWidget, "Could not find QLineEdit widget for status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");
	QVERIFY2(stepButtonWidget, "Could not find QTableView widget for memory");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Wrong status str");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Wrong");

	str = modelM->data(modelM->index(8, 1)).toString();
	QVERIFY2(str == "0x01", "Memory Wrong");
	str = modelM->data(modelM->index(9, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Wrong");
	str = modelM->data(modelM->index(10, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Wrong");
	str = modelM->data(modelM->index(11, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Wrong");
	str = modelM->data(modelM->index(12, 1)).toString();
	QVERIFY2(str == "0xfe", "Memory Wrong");
	str = modelM->data(modelM->index(13, 1)).toString();
	QVERIFY2(str == "0xff", "Memory Wrong");
	str = modelM->data(modelM->index(14, 1)).toString();
	QVERIFY2(str == "0xff", "Memory Wrong");
	str = modelM->data(modelM->index(15, 1)).toString();
	QVERIFY2(str == "0xff", "Memory Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000001", "Program Counter Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000008", "Register Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000002", "Program Counter Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Wrong");
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000004", "Program Counter Wrong");
	str = modelR->data(modelR->index(11 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffe", "Register Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Wrong");
	str = modelR->data(modelR->index(12 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffe", "Register Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000006", "Program Counter Wrong");
	str = modelR->data(modelR->index(13 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffe", "Program Counter Wrong");

}

void VirtualMachineGUITest::test02() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test02.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView*>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	qDebug() << "PC = " << str;
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000001", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000002", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000007", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Value Wrong");
	str = modelM->data(modelM->index(0, 1)).toString();
	QVERIFY2(str == "0x07", "Memory Value Wrong");
	str = modelM->data(modelM->index(1, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(2, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(3, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000004", "Program Counter Value Wrong");
	str = modelM->data(modelM->index(4, 1)).toString();
	QVERIFY2(str == "0x07", "Memory Value Wrong");
	str = modelM->data(modelM->index(5, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(6, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(7, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Value Wrong");
	str = modelM->data(modelM->index(8, 1)).toString();
	QVERIFY2(str == "0x07", "Memory Value Wrong");
	str = modelM->data(modelM->index(9, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(10, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(11, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000006", "Program Counter Value Wrong");
	str = modelM->data(modelM->index(12, 1)).toString();
	QVERIFY2(str == "0x07", "Memory Value Wrong");
	str = modelM->data(modelM->index(13, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(14, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(15, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000007", "Program Counter Value Wrong");
	str = modelM->data(modelM->index(16, 1)).toString();
	QVERIFY2(str == "0x07", "Memory Value Wrong");
	str = modelM->data(modelM->index(17, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(18, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(19, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
}

void VirtualMachineGUITest::test03() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test03.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000001", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000064", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(11 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(12 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Program Counter Value Wrong");

}

void VirtualMachineGUITest::test04() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test04.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");


	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000001", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(14 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(15 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(11 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(12 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(13 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(14 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(15 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffb2e", "Register Value Wrong");

}

void VirtualMachineGUITest::test05() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test05.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");
}

void VirtualMachineGUITest::test06() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test06.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000001f", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000002b", "Register Value Wrong");
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000004a", "Register Value Wrong");
}

void VirtualMachineGUITest::test07() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test07.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000c", "Register Value Wrong");
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000006", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(11 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");
	str = modelR->data(modelR->index(12 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000b", "Register Value Wrong");
	str = modelR->data(modelR->index(13 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffd", "Register Value Wrong");

}

void VirtualMachineGUITest::test08() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test08.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000c", "Register Value Wrong");
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000006", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(11 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");
	str = modelR->data(modelR->index(12 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000b", "Register Value Wrong");
	str = modelR->data(modelR->index(13 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffd", "Register Value Wrong");
}

void VirtualMachineGUITest::test09() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test09.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000008", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000000", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000008", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000a", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffe", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0xfffffffc", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0xffffffff", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffc", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000f", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000000", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000001", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000014", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000000", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffc", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0xffffffff", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");

}

void VirtualMachineGUITest::test10() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test10.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000008", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000000", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000008", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000a", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000000", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000001", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");
}

void VirtualMachineGUITest::test11() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test11.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000002", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000a", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffe", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0xffffffff", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000000", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0xffffffff", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000f", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000000", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x10000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000000", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x10000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000014", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000000", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffc", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0xf0000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000000", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0xf0000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");

}

void VirtualMachineGUITest::test12() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test12.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x00000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000002", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000002", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000a", "Program Counter Value Wrong");
	str = modelR->data(modelR->index(8 + regIndex, 2)).toString();
	QVERIFY2(str == "0x40000001", "Register Value Wrong");
	str = modelR->data(modelR->index(9 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");
	str = modelR->data(modelR->index(2, 2)).toString(); // lo
	QVERIFY2(str == "0x10000000", "Wrong register $lo str");
	str = modelR->data(modelR->index(1, 2)).toString(); // hi
	QVERIFY2(str == "0x00000001", "Wrong register $hi str");
	str = modelR->data(modelR->index(24 + regIndex, 2)).toString();
	QVERIFY2(str == "0x10000000", "Register Value Wrong");
	str = modelR->data(modelR->index(25 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");

	/*stepButtonWidget->click();
	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");*/

}

void VirtualMachineGUITest::test13() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test13.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000008", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
}

void VirtualMachineGUITest::test14() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test14.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffff1", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffff0", "Register Value Wrong");
}

void VirtualMachineGUITest::test15() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test15.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000f", "Register Value Wrong");
}

void VirtualMachineGUITest::test16() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test16.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x00000006", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0x0000000f", "Register Value Wrong");
}

void VirtualMachineGUITest::test17() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test17.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffff3", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffff5", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(10 + regIndex, 2)).toString();
	QVERIFY2(str == "0xfffffffc", "Register Value Wrong");
}

void VirtualMachineGUITest::test18() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test18.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	// int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString(); \
		QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000001", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000003", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000000", "Register Value Wrong");
}

void VirtualMachineGUITest::test19() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test19.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView *>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	// int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString(); \
		QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000004", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000005", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000007", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000008", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000a", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000b", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000d", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x0000000e", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000010", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000011", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000013", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000014", "Register Value Wrong");

	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000016", "Register Value Wrong");

	stepButtonWidget->click();
	stepButtonWidget->click();
	str = modelR->data(modelR->index(0, 2)).toString();
	QVERIFY2(str == "0x00000016", "Register Value Wrong");
}

void VirtualMachineGUITest::test20() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test20.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView*>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	// int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString(); \
		QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	for (int i = 0; i < 54; i++) {
		stepButtonWidget->click();
	}

	str = modelM->data(modelM->index(4, 1)).toString();
	QVERIFY2(str == "0x81", "Memory Value Wrong");
	str = modelM->data(modelM->index(5, 1)).toString();
	QVERIFY2(str == "0x01", "Memory Value Wrong");
	str = modelM->data(modelM->index(6, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(7, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

}

void VirtualMachineGUITest::test21() {
	QString file = QString::fromStdString(TEST_FILE_DIR + "/vm/test20.asm");
	widget.load(file);

	auto textWidget = widget.findChild<QPlainTextEdit*>("text");
	auto registerViewWidget = widget.findChild<QTableView*>("registers");
	auto memoryViewWidget = widget.findChild<QTableView*>("memory");
	auto statusViewWidget = widget.findChild<QLineEdit*>("status");
	auto stepButtonWidget = widget.findChild<QPushButton*>("step");
	auto runButtonWidget = widget.findChild<QPushButton*>("run");
	auto breakButtonWidget = widget.findChild<QPushButton*>("break");

	auto modelR = registerViewWidget->model();
	auto modelM = memoryViewWidget->model();
	QString str;
	// int regIndex = 3;

	QVERIFY2(statusViewWidget->text() == "Ok", "Status Wrong");
	// test program counter value
	str = modelR->data(modelR->index(0, 2)).toString(); 
	QVERIFY2(str == "0x00000000", "Program Counter Value Wrong");

	runButtonWidget->click();
	QTest::qSleep(1000);
	breakButtonWidget->click();

	str = modelM->data(modelM->index(4, 1)).toString();
	QVERIFY2(str == "0x81", "Memory Value Wrong");
	str = modelM->data(modelM->index(5, 1)).toString();
	QVERIFY2(str == "0x01", "Memory Value Wrong");
	str = modelM->data(modelM->index(6, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");
	str = modelM->data(modelM->index(7, 1)).toString();
	QVERIFY2(str == "0x00", "Memory Value Wrong");

	

}

QTEST_MAIN(VirtualMachineGUITest)
#include "virtual_machine_gui_test.moc"


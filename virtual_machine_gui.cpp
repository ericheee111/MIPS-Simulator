#include "virtual_machine_gui.hpp"
#include <QGridLayout>
#include <QLabel>
#include <QDebug>

// TODO implement the GUI class

VirtualMachineGUI::VirtualMachineGUI(QWidget* parent) : QWidget(parent) {
	qDebug() << "Constructor called xxxxxxxxxxxxxxxxxxxxxxxxxx";
	text = new QPlainTextEdit(this);
	text->setObjectName("text");
	registers = new QTableView(this);
	registers->setObjectName("registers");
	memory = new QTableView(this);
	memory->setObjectName("memory");
	status = new QLineEdit("Ok", this);
	status->setObjectName("status");
	qDebug()<< "status = " << status->text();
	step = new QPushButton("step", this);
	step->setObjectName("step");
	run = new QPushButton("run", this);
	run->setObjectName("run");
	break_button = new QPushButton("break", this);
	break_button->setObjectName("break");
	break_button->setEnabled(false);
	modelR = new QStandardItemModel(35, 3, this);
	modelM = new QStandardItemModel(255, 2, this);
	QGridLayout* layout = new QGridLayout(this);

	// text			xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	text->setReadOnly(true);
	layout->addWidget(text, 0, 0, 10, 5);

	// registers		xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	QStringList horizontalHeaderR;
	horizontalHeaderR.append("Number");
	horizontalHeaderR.append("Alias");
	horizontalHeaderR.append("Value (Hex)");
	modelR->setHorizontalHeaderLabels(horizontalHeaderR);

	std::stringstream strR;
	QModelIndex indexpc2 = modelR->index(0, 1, QModelIndex());
	//QModelIndex indexpc3 = modelR->index(0, 2, QModelIndex());
	modelR->setData(indexpc2, QString::fromStdString("$pc"));
	//modelR->setData(indexpc3, QString::fromStdString("0x00000000"));
	QModelIndex indexhi2 = modelR->index(1, 1, QModelIndex());
	//QModelIndex indexhi3 = modelR->index(1, 2, QModelIndex());
	modelR->setData(indexhi2, QString::fromStdString("$hi"));
	//modelR->setData(indexhi3, QString::fromStdString("0x00000000"));
	QModelIndex indexlo2 = modelR->index(2, 1, QModelIndex());
	//QModelIndex indexlo3 = modelR->index(2, 2, QModelIndex());
	modelR->setData(indexlo2, QString::fromStdString("$lo"));
	//modelR->setData(indexlo3, QString::fromStdString("0x00000000"));

	for (int i = 0; i < 32; i++) {
		QModelIndex indexR1 = modelR->index(i + 3, 0, QModelIndex());
		QModelIndex indexR2 = modelR->index(i + 3, 1, QModelIndex());
		QModelIndex indexR3 = modelR->index(i + 3, 2, QModelIndex());
		strR << "$" << i;
		modelR->setData(indexR1, QString::fromStdString(strR.str()));
		strR.str("");
		strR << "$" << numtoReg[i];
		modelR->setData(indexR2, QString::fromStdString(strR.str()));
		modelR->setData(indexR3, QString::fromStdString("0x00000000"));
		strR.str("");		// reset string stream to empty
	}

	registers->setModel(modelR);

	layout->addWidget(registers, 0, 5, 10, 5);

	// memory		xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	QStringList horizontalHeaderM;
	horizontalHeaderM.append("Address (Hex)");
	horizontalHeaderM.append("Value (Hex)");
	modelM->setHorizontalHeaderLabels(horizontalHeaderM);

	std::stringstream strM;
	for (int i = 0; i < 255; i++) {
		QModelIndex indexM1 = modelM->index(i, 0, QModelIndex());
		QModelIndex indexM2 = modelM->index(i, 1, QModelIndex());
		strM << "0x" << std::setw(8) << std::setfill('0') << std::hex << i;
		modelM->setData(indexM1, QString::fromStdString(strM.str()));

		modelM->setData(indexM2, QString::fromStdString("0x00"));
		strM.str("");		// reset string stream to empty
	}
	memory->setModel(modelM);

	layout->addWidget(memory, 0, 10, 10, 5);

	// status		xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	QLabel* stat = new QLabel(tr("Status: "));
	layout->addWidget(stat, 11, 0, 1, 1);
	status->setReadOnly(true);
	layout->addWidget(status, 11, 1, 1, 14);


	// step			xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	layout->addWidget(step, 13, 0, 1, 5);
	layout->addWidget(run, 13, 5, 1, 5);
	layout->addWidget(break_button, 13, 10, 1, 5);

	this->setLayout(layout);
	
	w1.con(&mqueue);
	//w1.writeVM(VM);
	sim_th = std::thread(std::ref(w1));

	connect(step, &QPushButton::clicked, this, &VirtualMachineGUI::stepNext);
	//connect(step, &QPushButton::clicked, this, &VirtualMachineGUI::refresh);
	connect(run, &QPushButton::clicked, this, &VirtualMachineGUI::_run_);
	connect(break_button, &QPushButton::clicked, this, &VirtualMachineGUI::_break_);
	//connect(break_button, &QPushButton::clicked, this, &VirtualMachineGUI::refresh);
}
VirtualMachineGUI::~VirtualMachineGUI() {
	qDebug("closed");
	mqueue.push(Message::quit);
	sim_th.join();
}

void VirtualMachineGUI::load(QString filename) {
	QFile file(filename);
	file.open(QIODevice::ReadOnly);
	QTextStream in(&file);
	QString content = in.readAll();
	text->setPlainText(content);
	file.close();

	std::stringstream source;
	source << content.toStdString();
	TokenList tl = tokenize(source);
	Parse syntax;
	bool parsing = syntax.parse(tl);
	VM = syntax.getVM();
	w1.writeVM(VM);
	startLine = syntax.getMainLine() - 1;
	if (parsing == true) {
		status->setText("Ok");
	}
	else {
		status->setText("Error");
	}

	highlightCurrLine(startLine);

	QModelIndex indexpc3 = modelR->index(0, 2, QModelIndex());
	modelR->setData(indexpc3, QString::fromStdString("0x00000000"));
	QModelIndex indexhi3 = modelR->index(1, 2, QModelIndex());
	modelR->setData(indexhi3, QString::fromStdString("0x00000000"));
	QModelIndex indexlo3 = modelR->index(2, 2, QModelIndex());
	modelR->setData(indexlo3, QString::fromStdString("0x00000000"));
	for (int i = 0; i < 32; i++) {
		QModelIndex indexR3 = modelR->index(i + 3, 2, QModelIndex());
		modelR->setData(indexR3, QString::fromStdString("0x00000000"));
	}

	for (int i = 0; i < 255; i++) {
		QModelIndex indexM2 = modelM->index(i, 1, QModelIndex());
		uint8_t get = VM.readMEM(i, 1);
		std::string mem = uint8ToHex(&get);
		modelM->setData(indexM2, QString::fromStdString(mem));
	}
	mqueue.push(Message::step);
}

void VirtualMachineGUI::writeVM(VirtualMachine v) {
	VM = v;
}


void VirtualMachineGUI::refresh() {
	//VM = w1.getVM();
	qDebug("refreshed");
	qDebug() << "pc = " << VM.readPC();
	if (VM.readPC() < VM.getInstrVector().size()) {
		int currLine = VM.getInstruction(VM.readPC()).readLineNum() - 1;
		highlightCurrLine(currLine);
	}

	QModelIndex indexpc3 = modelR->index(0, 2, QModelIndex());
	modelR->setData(indexpc3, QString::fromStdString(uint32ToHex(VM.readPC())));
	QModelIndex indexhi3 = modelR->index(1, 2, QModelIndex());
	modelR->setData(indexhi3, QString::fromStdString(uint32ToHex(VM.readHI())));
	QModelIndex indexlo3 = modelR->index(2, 2, QModelIndex());
	modelR->setData(indexlo3, QString::fromStdString(uint32ToHex(VM.readLO())));

	for (int i = 0; i < 32; i++) {
		QModelIndex indexR3 = modelR->index(i + 3, 2, QModelIndex());
		modelR->setData(indexR3, QString::fromStdString(uint32ToHex(VM.readReg(i))));
	}

	for (int i = 0; i < 255; i++) {
		QModelIndex indexM2 = modelM->index(i, 1, QModelIndex());
		uint8_t get = VM.readMEM(i, 1);
		std::string mem = uint8ToHex(&get);
		modelM->setData(indexM2, QString::fromStdString(mem));
	}

	if (VM.getStatus() == VM_Status::Error) {
		status->setText("Error");
	}

}


void VirtualMachineGUI::highlightCurrLine(std::size_t lineNum) {
	QTextBlock block = text->document()->findBlockByNumber(lineNum);
	int pos = block.position();
	QList<QTextEdit::ExtraSelection> extraSelections;
	QTextEdit::ExtraSelection selection;
	QColor lineColor = QColor(Qt::yellow).lighter(160);
	selection.format.setBackground(lineColor);
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	selection.cursor = * new QTextCursor(text->document());
	selection.cursor.setPosition(pos);
	selection.cursor.clearSelection();
	extraSelections.append(selection);
	text->setExtraSelections(extraSelections);
}

void VirtualMachineGUI::stepNext() {
	qDebug("step GUI");
	//VM.simulation();
	mqueue.push(Message::step);
	VM = w1.getVM();
	refresh();
}

void VirtualMachineGUI::_run_() {
	qDebug("running GUI");
	mqueue.push(Message::run);
	break_button->setEnabled(true);
	run->setEnabled(false);
}
void VirtualMachineGUI::_break_() {
	qDebug("breaking GUI");
	mqueue.push(Message::_break_);
	break_button->setEnabled(false);
	run->setEnabled(true);
	VM = w1.getVM();
	refresh();
	int currLine = VM.getInstrVector().back().readLineNum() - 1;
	highlightCurrLine(currLine);
}


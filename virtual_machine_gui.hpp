#ifndef VIRTUAL_MACHINE_GUI_HPP
#define VIRTUAL_MACHINE_GUI_HPP

#include <QString>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QStandardItemModel>
#include <QStringList>
#include <QModelIndex>
#include <QTextBlock>
#include "thread.hpp"
#include "lexer.hpp"
#include <fstream>
#include <sstream>
#include <istream>
#include <iostream>

// TODO define the GUI class

static std::unordered_map<int, std::string> numtoReg{ {0, "zero"}, {1, "at"}, {2, "v0"}, {3, "v1"}, {4, "a0"}, {5, "a1"}, {6, "a2"}, {7, "a3"},
													 {8, "t0"}, {9, "t1"}, {10, "t2"}, {11, "t3"}, {12, "t4"}, {13, "t5"}, {14, "t6"}, {15, "t7"},
													 {16, "s0"}, {17, "s1"}, {18, "s2"}, {19, "s3"}, {20, "s4"}, {21, "s5"}, {22, "s6"}, {23, "s7"},
													 {24, "t8"}, {25, "t9"}, {26, "k0"}, {27, "k1"}, {28, "gp"}, {29, "sp"}, {30, "fp"}, {31, "ra"} };


class QPlainTextEdit;

class QTableView;

class QStandardItemModel;   

class QLineEdit;

class QPushButton;

class VirtualMachineGUI : public QWidget {
	Q_OBJECT
public:
	VirtualMachineGUI(QWidget* parent = nullptr);			// initialize
	~VirtualMachineGUI();
	void load(QString filename);
	void writeVM(VirtualMachine VM);
	

signals:
	void stepped();


private slots:
	void highlightCurrLine(std::size_t lineNum);
	void stepNext();
	void _run_();
	void _break_();
	void refresh();
private:
	QPlainTextEdit* text;
	QTableView* registers;
	QTableView* memory;
	QLineEdit* status;
	QPushButton* step;
	QPushButton* run;
	QPushButton* break_button;
	QStandardItemModel* modelR;
	QStandardItemModel* modelM;
	VirtualMachine VM;

	int startLine = 0;
	
	message_queue<Message> mqueue;
	Worker w1;
	std::thread sim_th;
};


#endif

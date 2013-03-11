#ifndef VIRTUALMACHINE_H
#define VIRTUALMACHINE_H

#include <QObject>
#include <QVector>
#include <QMap>

class VirtualMachine : public QObject
{
	Q_OBJECT

	public:
		enum Register
		{
			ACU = 0
		};

		enum Instruction
		{
			HLT = 0,
			INC,
			DEC,
			CPA,
			STO,
			ADD,
			SUB,
			BRA,
			BRN,
			MUL,
			BRZ
		};

	public:
		static int memoryToInt(const int &mem);
		static int intToMemory(const int &val);

	public:
		explicit VirtualMachine(QObject *parent = 0);

		void assemble(const QString &code);
		int assembleInstruction(const int &cellNo, const QString &mnemonic, const QString &strValue);
		bool exec();

		const QVector<int> &memory() const;
		void setMemory(const QVector<int> &memory);

		void setMemorySize(const int &memorySize);
		int memorySize() const;

		const QVector<int> &registers();
		void setRegisters(const QVector<int> &registers);

		QString registerName(const int &registerNo) const;
		void setRegisterCount(const int &registerCount);
		int registerCount() const;

		QVector<int> labels();
		QString labelName(const int &labelNo) const;
		int labelCount() const;

		void setExecCell(const int &execCell);
		int execCell() const;

	public slots:
		void resetRegisters();
		void resetMemory();

	signals:
		void memoryChanged(const QVector<int> &memory);
		void registersChanged(const QVector<int> &registers);
		void labelsChanged();

	private:
		QMap<QString, int> m_labels;
		QVector<int> m_registers;
		QVector<int> m_memory;
		int m_execCell;
};

#endif // VIRTUALMACHINE_H

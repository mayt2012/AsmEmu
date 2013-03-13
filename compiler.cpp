#include "compiler.h"

#include "virtualmachine.h"

#include <QRegularExpression>

const QMap<QString, VirtualMachine::Instruction> mnemonicMap(std::map<QString, VirtualMachine::Instruction>({{"HLT", VirtualMachine::HLT},
																											 {"INC", VirtualMachine::INC},
																											 {"DEC", VirtualMachine::DEC},
																											 {"CPA", VirtualMachine::CPA},
																											 {"STO", VirtualMachine::STO},
																											 {"ADD", VirtualMachine::ADD},
																											 {"SUB", VirtualMachine::SUB},
																											 {"BRA", VirtualMachine::BRA},
																											 {"BRN", VirtualMachine::BRN},
																											 {"MUL", VirtualMachine::MUL},
																											 {"BRZ", VirtualMachine::BRZ}}));


Compiler::Compiler(const QString &code)
{
	m_lines = code.split('\n', QString::KeepEmptyParts);

	const QRegularExpression labelPattern("^\\s?([a-zęóąśłżźćńĘÓĄŚŁŻŹĆŃ_][_a-zęóąśłżźćńĘÓĄŚŁŻŹĆŃ0-9]*:)\\s?$", QRegularExpression::CaseInsensitiveOption);


	for(int i = 0; i < m_lines.size(); ++i) {
		//strip comments
		m_lines[i].replace(QRegularExpression(";(.*)$"), "");

		//trim whitespace
		m_lines[i].replace(QRegularExpression("\\s+"), " ");
		m_lines[i] = m_lines[i].trimmed();

		//move single labels to next lines
		QRegularExpressionMatch labelMatch = labelPattern.match(m_lines[i]);
		if(labelMatch.hasMatch() && i < m_lines.size() - 1) {
			m_lines[i + 1].prepend(labelMatch.captured(1) + " ");
		}
	}
}

const QMap<int, int> &Compiler::lineMap() const
{
	return m_lineMap;
}

const QMap<QString, int> &Compiler::labelMap() const
{
	return m_labelMap;
}

int Compiler::startCell() const
{
	return m_startCell;
}

int Compiler::assembleInstruction(const int &cellNo, const QString &mnemonic, const QString &strValue)
{
	VirtualMachine::Instruction instr = mnemonicMap.value(mnemonic.toUpper());

	const bool isConst = strValue.contains('$');
	const bool isAddress = strValue.contains('[');
	const int value = QString(strValue).remove(QRegularExpression("\\$|\\[|\\]")).toInt();

	switch(instr) {
		case VirtualMachine::HLT:
		{
			emit memoryChanged(cellNo, VirtualMachine::intToMemory(0));
			return 1;
		}
		break;

		case VirtualMachine::INC:
		case VirtualMachine::DEC:
		{
			const QList<int> varList = {
				9300, 9390,
				9400, 9490,
				 100,  200,
				9000, 9010
			};

			const int diff = instr - VirtualMachine::INC;

			if(isConst) {
				return 0;
			} else if(isAddress) {
				if(value < 10) {
					emit memoryChanged(cellNo, varList[0 + diff] + value);
					return 1;
				} else {
					emit memoryChanged(cellNo, varList[2 + diff]);
					emit memoryChanged(cellNo + 1, value);
					return 2;
				}
			} else {
				if(value < 100) {
					emit memoryChanged(cellNo, varList[4 + diff] + value);
					return 1;
				} else {
					emit memoryChanged(cellNo, varList[6 + diff]);
					emit memoryChanged(cellNo + 1, value);
					return 2;
				}
			}
		}
		break;

		case VirtualMachine::CPA:
		case VirtualMachine::STO:
		case VirtualMachine::ADD:
		case VirtualMachine::SUB:
		case VirtualMachine::BRA:
		case VirtualMachine::BRN:
		case VirtualMachine::MUL:
		case VirtualMachine::BRZ:
		{
			const QList<int> varList = {
			//	CPA   STO   ADD   SUB   BRA   BRN   MUL   BRZ
				9110, 9120, 9130, 9140, 9150, 9160, 9170, 9180, // const < 10
				9210, 9220, 9210, 9240, 9250, 9260, 9270, 9280, // const >= 10
				9310, 9320, 9330, 9340, 9350, 9360, 9370, 9380, // pointer < 10
				9410, 9420, 9430, 9440, 9450, 9460, 9470, 9480, // pointer >= 10
				1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, // value < 1000
				9010, 9020, 9030, 9040, 9050, 9060, 9070, 9080  // value >= 1000
			};

			const int diff = instr - VirtualMachine::CPA;

			if(isConst) {
				if(value < 10) {
					emit memoryChanged(cellNo, varList[0 + diff] + value);
					return 1;
				} else {
					emit memoryChanged(cellNo, varList[8 + diff]);
					emit memoryChanged(cellNo + 1, value);
					return 2;
				}
			} else if(isAddress) {
				if(value < 10) {
					emit memoryChanged(cellNo, varList[16 + diff] + value);
					return 1;
				} else {
					emit memoryChanged(cellNo, varList[24 + diff]);
					emit memoryChanged(cellNo + 1, value);
					return 2;
				}
			} else {
				if(value < 1000) {
					emit memoryChanged(cellNo, varList[32 + diff] + value);
					return 1;
				} else {
					emit memoryChanged(cellNo, varList[40 + diff]);
					emit memoryChanged(cellNo + 1, value);
					return 2;
				}
			}
		}
		break;
	}

	return 0;
}

bool Compiler::compile()
{
	int dataStart = 0;
	int codeStart = 0;

	for(int i = 0; i < m_lines.size(); ++i) {
		const QString line = m_lines[i];
		const QStringList parts = line.split(' ');
		if(!parts.size())
			continue;

		if(parts.size() == 3) {
			/* 3 parts so it's one of:
			 * - label mnemonic value
			 * - cell mnemonic value
			 */

			int cellNumber = parts[0].toInt();

			//labeled instruction
			if(parts[0].contains(':')) {
				const QString label = parts[0].left(parts[0].size() - 1);

				m_labelMap.insert(label, codeStart);
				cellNumber = codeStart;
			}

			//labeled value
			QString realVal = parts[2];
			if(m_labelMap.contains(parts[2]))
				realVal = QString::number(m_labelMap.value(parts[2]));

			const int size = assembleInstruction(cellNumber, parts[1], realVal);

			m_lineMap.insert(i + 1, cellNumber);
			codeStart = cellNumber + size;
		} else if(parts.size() == 2) {
			/*2 parts so one of:
			* - directive value
			* - label value
			* - (sequential) mnemonic value
			* - cell value
			* - cell mnemonic
			*/

			if(parts[0].startsWith('.')) {
				//directive

				if(parts[0].toLower() == ".data") {
					dataStart = parts[1].toInt();
				} else if(parts[0].toLower() == ".code") {
					codeStart = parts[1].toInt();
					m_startCell = codeStart;
				}
			} else if(parts[0].contains(':')) {
				const QString label = parts[0].left(parts[0].size() - 1);

				bool isNumber = false;
				const int value = parts[1].toInt(&isNumber);

				if(isNumber) {
					//variable definition

					m_lineMap.insert(i + 1, dataStart);
					m_labelMap.insert(label, dataStart);
					emit memoryChanged(dataStart, value);
					++dataStart;
				} else {
					//labeled HLT

					m_lineMap.insert(i + 1, codeStart);
					m_labelMap.insert(label, codeStart);
					emit memoryChanged(codeStart, 0);
					++codeStart;
				}
			} else {
				bool hasCellNumber = false;
				const int cellNumber = parts[0].toInt(&hasCellNumber);

				if(hasCellNumber) {
					//has cell number

					bool isNumber = false;
					const int value = parts[1].toInt(&isNumber);

					if(isNumber) {
						//memory value
						emit memoryChanged(cellNumber, value);
						dataStart = cellNumber + 1;
					} else if(parts[1].toLower() == "hlt") {
						//a HLT mnemonic
						emit memoryChanged(cellNumber, VirtualMachine::intToMemory(0));
						codeStart = cellNumber + 1;
					}

					m_lineMap.insert(i + 1, cellNumber);
				} else {
					//mnemonic and value

					m_lineMap.insert(i + 1, codeStart);

					//labeled value
					QString realVal = parts[1];
					if(m_labelMap.contains(parts[1]))
						realVal = QString::number(m_labelMap.value(parts[1]));

					const int size = assembleInstruction(codeStart, parts[0], realVal);

					m_lineMap.insert(i + 1, codeStart);
					codeStart += size;
				}
			}
		} else if(parts.size() == 1) {
			/* 1 part is
			 * - (sequential) mnemonic (HLT)
			 * - (sequential) value
			 * - label:
			 */

			bool isNumber = false;
			const int value = parts[0].toInt(&isNumber);

			if(isNumber) {
				//sequential memory value
				emit memoryChanged(dataStart, value);
				++dataStart;
			} else if(parts[0].toLower() == "hlt") {
				//a HLT mnemonic
				emit memoryChanged(codeStart, VirtualMachine::intToMemory(0));
				m_lineMap.insert(i + 1, codeStart);

				++codeStart;
			} else if(parts[0].contains(':')) {
				//label
				m_lineMap.insert(i + 1, codeStart);
			}
		}
	}

	return true;
}

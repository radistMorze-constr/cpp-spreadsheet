#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <unordered_set>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

	// Можете дополнить ваш класс нужными полями и методами
    bool IsInsideSheet(Position pos);
private:
	// Можете дополнить ваш класс нужными полями и методами
    Size size_;
    std::unordered_map<Position, Cell, PositionHasher> cells_;

    enum class PrintType {
        Values,
        Text
    };
    void Print(std::ostream& output, Sheet::PrintType type) const;
};
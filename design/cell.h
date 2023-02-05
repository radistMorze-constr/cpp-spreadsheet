#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;
    void InvalidateCache();

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    //using member "sheet_"
    void ThrowIfCircularDependency(const std::vector<Position>& cells, Cell* current_cell, std::unordered_set<Position, PositionHasher>& verified_cells);

    Sheet& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cached_value_;
    std::unordered_set<Cell*> referenced_cells_;
    std::unordered_set<Cell*> dependent_cells_;
};
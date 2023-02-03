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

    void Set(Position current_pos, std::string text);
    void Clear(Position pos);

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;
    void InvalidateCache();

    void HasCircularDependency(const std::vector<Position>& cells, Position current_pos, std::unordered_set<Position, PositionHasher>& verified_cells);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    Sheet& sheet_;
    std::unique_ptr<Impl> impl_;
    mutable std::optional<Value> cached_value_;
    std::unordered_set<Position, PositionHasher> referenced_cells_;
    std::unordered_set<Position, PositionHasher> dependent_cells_;
};
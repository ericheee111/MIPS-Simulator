#include "memory_model.hpp"
#include "mips/numbers.hpp"
#include <QString>
#include <algorithm>
#include <stdexcept>
namespace mips {
int MemoryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(total_);
}
int MemoryModel::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : 2; }
QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) return QStringLiteral("Address (Hex)");
        if (section == 1) return QStringLiteral("Value (Hex)");
    }
    return QVariant();
}
QVariant MemoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= total_ ||
        index.column() < 0 || index.column() >= 2 || role != Qt::DisplayRole) return QVariant();
    const uint64_t address = static_cast<uint64_t>(index.row());
    if (index.column() == 0) return QString::fromStdString(hexValue(static_cast<uint32_t>(address)));
    if (address < base_ || address - base_ >= bytes_.size()) return QStringLiteral("--");
    return QString::fromStdString(hexValue(bytes_[static_cast<std::size_t>(address-base_)],2));
}
void MemoryModel::update(const DebugState& state) {
    if (state.memorySize > MaxMemoryBytes || state.memory.size() > MaxObservationBytes ||
        state.memory.size() > state.memorySize || state.memoryBase > state.memorySize-state.memory.size())
        throw std::invalid_argument("invalid debugger memory window");
    if (total_ != state.memorySize) {
        // Allocate before changing model invariants or entering a reset notification.
        auto replacement = state.memory;
        beginResetModel(); total_ = state.memorySize; base_ = state.memoryBase; bytes_.swap(replacement); endResetModel();
        return;
    }
    if (base_ != state.memoryBase || bytes_.size() != state.memory.size()) {
        const auto oldBase = base_; const auto oldSize = bytes_.size();
        auto replacement = state.memory;
        base_ = state.memoryBase; bytes_.swap(replacement);
        if (oldSize) emit dataChanged(index(static_cast<int>(oldBase),1),index(static_cast<int>(oldBase+oldSize-1),1));
        if (!bytes_.empty()) emit dataChanged(index(static_cast<int>(base_),1),index(static_cast<int>(base_+bytes_.size()-1),1));
        return;
    }
    auto first = std::mismatch(bytes_.begin(),bytes_.end(),state.memory.begin()).first;
    if (first == bytes_.end()) return;
    const auto start = static_cast<std::size_t>(first-bytes_.begin());
    std::size_t last = bytes_.size()-1;
    while (last > start && bytes_[last] == state.memory[last]) --last;
    bytes_ = state.memory;
    emit dataChanged(index(static_cast<int>(base_+start),1),index(static_cast<int>(base_+last),1));
}
}

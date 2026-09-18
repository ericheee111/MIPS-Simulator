#ifndef MIPS_GUI_MEMORY_MODEL_HPP
#define MIPS_GUI_MEMORY_MODEL_HPP
#include "mips/controller.hpp"
#include <QAbstractTableModel>
namespace mips {
// Logical rows span memory; storage contains only one bounded, coherent window.
class MemoryModel : public QAbstractTableModel {
public:
    explicit MemoryModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    void update(const DebugState& state);
    std::size_t cachedBytes() const { return bytes_.size(); }
private:
    std::size_t total_ = 0;
    uint64_t base_ = 0;
    std::vector<uint8_t> bytes_;
};
}
#endif

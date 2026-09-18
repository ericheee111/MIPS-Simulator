#ifndef MIPS_GUI_SOURCE_EDITOR_HPP
#define MIPS_GUI_SOURCE_EDITOR_HPP
#include <QPlainTextEdit>
class QPaintEvent;
namespace mips {
class SourceEditor;
class LineGutter : public QWidget {
public:
    explicit LineGutter(SourceEditor* editor);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    SourceEditor* editor_;
};
class SourceEditor : public QPlainTextEdit {
public:
    explicit SourceEditor(QWidget* parent = nullptr);
    int gutterWidth() const;
    void paintGutter(QPaintEvent* event);
protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
private:
    void updateWidth();
    LineGutter* gutter_;
};
}
#endif

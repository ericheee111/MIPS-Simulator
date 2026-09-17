#include "source_editor.hpp"
#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>
#include <QEvent>
namespace mips {
LineGutter::LineGutter(SourceEditor* editor) : QWidget(editor), editor_(editor) { setObjectName("lineNumbers"); }
QSize LineGutter::sizeHint() const { return QSize(editor_->gutterWidth(),0); }
void LineGutter::paintEvent(QPaintEvent* event) { editor_->paintGutter(event); }
SourceEditor::SourceEditor(QWidget* parent) : QPlainTextEdit(parent), gutter_(new LineGutter(this)) {
    connect(this,&QPlainTextEdit::blockCountChanged,this,[this] { updateWidth(); });
    connect(this,&QPlainTextEdit::updateRequest,this,[this](const QRect& rectangle,int dy) {
        if (dy) gutter_->scroll(0,dy);
        else gutter_->update(0,rectangle.y(),gutter_->width(),rectangle.height());
        if (rectangle.contains(viewport()->rect())) updateWidth();
    });
    updateWidth();
}
int SourceEditor::gutterWidth() const {
    return 12 + fontMetrics().boundingRect(QString::number(qMax(1,blockCount()))).width();
}
void SourceEditor::updateWidth() {
    setViewportMargins(gutterWidth(),0,0,0);
    const QRect rectangle = contentsRect();
    gutter_->setGeometry(QRect(rectangle.left(),rectangle.top(),gutterWidth(),rectangle.height()));
    gutter_->update();
}
void SourceEditor::resizeEvent(QResizeEvent* event) { QPlainTextEdit::resizeEvent(event); updateWidth(); }
void SourceEditor::changeEvent(QEvent* event) {
    QPlainTextEdit::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::PaletteChange) updateWidth();
}
void SourceEditor::paintGutter(QPaintEvent* event) {
    QPainter painter(gutter_);
    painter.fillRect(event->rect(),palette().alternateBase());
    painter.setPen(palette().text().color());
    QTextBlock block = firstVisibleBlock();
    int number = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top())
            painter.drawText(0,top,gutter_->width()-6,fontMetrics().height(),Qt::AlignRight,QString::number(number+1));
        block = block.next(); top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height()); ++number;
    }
}
}

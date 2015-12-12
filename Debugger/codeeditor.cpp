/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>

#include "codeeditor.h"
#include <QDebug>
#include <QTextCursor>

extern void setBreakPointInGDB(int);


int isjuststarted = 1;
int lastturncount = 0;

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    scrolled = 7;
    diffscroll = 0;
    lastselectionline = 0;
}



int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 10 + fontMetrics().width(QLatin1Char('11')) * digits;

    return space;
}



void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}



void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy){
        lineNumberArea->scroll(0, dy);
        scrolled = 7;
        diffscroll -= dy;
//        qDebug()<<dy;
//        breakPointRect.setCoords(breakPointRect.x(),breakPointRect.y()+dy,breakPointRect.width(),breakPointRect.height());
        breakPointRect.setRect(breakPointRect.x(),breakPointRect.y()+dy,breakPointRect.width(),breakPointRect.height());
//        debugPointRect.setRect(debugPointRect.x(),debugPointRect.y()+dy,debugPointRect.width(),debugPointRect.height());
    }
    else{
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
//        scrolled = 1;
        diffscroll += 0;
    }

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}



void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}



void CodeEditor::highlightCurrentLine()
{
    if(!extraSelections.isEmpty()){
        if(lastselectionline==0){
            extraSelections.removeLast();
        }else{
            if(extraSelections.size()>=2){
                extraSelections.removeFirst();
            }
        }
    }
    lastselectionline = 0;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}



void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    if(isPressed){
        painter.fillRect(breakPointRect,Qt::red);        
        //isPressed = false;
    }


    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void LineNumberArea::mousePressEvent(QMouseEvent *e){

    int x = e->x();
    int y = e->y();



    int lineNumber = 0;

    if(x>=0 && x<=codeEditor->lineNumberAreaWidth()){
        lineNumber = (y/fontMetrics().height())+1;
        codeEditor->setBreakPoint(e,lineNumber);
    }
}


void CodeEditor::highlightCurrentDebugLine(int lineNumber){

    if(!isjuststarted){
        qDebug()<<"lastturncount "<<lastturncount;
        extraSelections.removeAt(extraSelections.size()-lastturncount-1);
        lastturncount = 0;
//        extraSelections.removeLast();
    }else{
        lastturncount=0;
        isjuststarted = 0;
    }
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::green).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        QString text = this->toPlainText();
        QStringList texts = text.split("\n");
        int totalshift = 0;
        qDebug()<<texts.length();
        for(int i=0;i<texts.length();i++){
            qDebug()<<texts.at(i).length();
            if(i>=(lineNumber-1)) break;
            totalshift += texts.at(i).length()+1;
        }
        selection.cursor.setPosition(totalshift,QTextCursor::MoveAnchor);
        lastselection = selection;
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

void CodeEditor::highlightDebugLine(int lineNumber){
//    QList<QTextEdit::ExtraSelection> extraSelections;

//    qDebug()<<breakPointRect.y();

    lastturncount++;

    if(!extraSelections.isEmpty()){
        if(lastselectionline==1){
//            extraSelections.removeLast();
        }else{
            if(extraSelections.size()>=2){
                extraSelections.removeFirst();
            }
        }
    }
    lastselectionline = 1;

    if (!isReadOnly()) {
        qDebug()<<"Here";
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::blue).lighter(160);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
//        selection.cursor.setVerticalMovementX(lineNumber-1);
//        qDebug()<<selection.cursor.blockNumber();
//        qDebug()<<this->textCursor().movePosition(QTextCursor::NextRow,QTextCursor::MoveAnchor,2);
//        qDebug()<<selection.cursor.
        QString text = this->toPlainText();
        QStringList texts = text.split("\n");
        int totalshift = 0;
        qDebug()<<texts.length();
        for(int i=0;i<texts.length();i++){
            qDebug()<<texts.at(i).length();
            if(i>=(lineNumber-1)) break;
            totalshift += texts.at(i).length()+1;
        }
        selection.cursor.setPosition(totalshift,QTextCursor::MoveAnchor);
        extraSelections.append(selection);
    }

//    extraSelections.append();
    setExtraSelections(extraSelections);
//    extraSelections.

}


void CodeEditor::setBreakPoint(QMouseEvent *e,int lineNumber){

//    qDebug()<<diffscroll;
//    qDebug()<<diffscroll;
//    qDebug()<<((lineNumber-1)*fontMetrics().height())/fontMetrics().height();

    isPressed = true;
//    QPainter painter(lineNumberArea);
//    qDebug()<<scrolled;
    QRect rec(0,((lineNumber-1)*fontMetrics().height()+fontMetrics().height()/2)-scrolled,lineNumberAreaWidth(),fontMetrics().height());


//    QRect breakrec(20,((lineNumber-1)*fontMetrics().height()+fontMetrics().height()/2)-scrolled,100,fontMetrics().height());

    int line = ((lineNumber-1)*fontMetrics().height()+diffscroll)/fontMetrics().height();
    qDebug()<<line;
    highlightDebugLine(line+1);
    breakPointRect = rec;
//    debugPointRect = breakrec;

    setBreakPointInGDB(lineNumber);
    update();

//    painter.fillRect(rec, Qt::red);
}





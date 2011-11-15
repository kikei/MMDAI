/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2011  hkrn                                    */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the MMDAI project team nor the names of     */
/*   its contributors may be used to endorse or promote products     */
/*   derived from this software without specific prior written       */
/*   permission.                                                     */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#include "TimelineWidget.h"
#include "BoneMotionModel.h"
#include "FaceMotionModel.h"

#include <QtGui/QtGui>
#include <vpvl/vpvl.h>
#include "util.h"

namespace {

class TimelineItemDelegate : public QItemDelegate
{
public:
    TimelineItemDelegate(QObject *parent = 0) : QItemDelegate(parent) {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        /* ボーンまたは頂点モーフの名前を表示する部分は継承元のクラスに描画を任せる */
        if (index.column() == 0) {
            QItemDelegate::paint(painter, option, index);
            return;
        }
        /* キーフレームのインデックスが5で割り切れる場合は背景を白ではない色にする */
        if (MotionBaseModel::toFrameIndex(index) % 5 == 0)
            painter->fillRect(option.rect, qApp->palette().alternateBase());
        painter->setPen(Qt::NoPen);
        painter->setRenderHint(QPainter::Antialiasing);
        /* モデルのデータにキーフレームのバイナリが含まれていればダイアモンドマークを表示する */
        if (index.data(MotionBaseModel::kBinaryDataRole).canConvert(QVariant::ByteArray)) {
            if (option.state & QStyle::State_Selected)
                painter->setBrush(Qt::red);
            else
                painter->setBrush(option.palette.foreground());
            drawDiamond(painter, option);
        }
        else if (option.state & QStyle::State_Selected) {
            painter->setBrush(option.palette.highlight());
            drawDiamond(painter, option);
        }
    }

private:
    void drawDiamond(QPainter *painter, const QStyleOptionViewItem &option) const {
        const QRect &rect = option.rect;
        int width = rect.height();
        int height = width;
        int xoffset = rect.x() + ((rect.width() - width) / 2);
        int yoffset = rect.y() + ((rect.height() - height) / 2);
        QPolygon polygon;
        polygon.append(QPoint(xoffset, yoffset + height / 2));
        polygon.append(QPoint(xoffset + width / 2, yoffset + height));
        polygon.append(QPoint(xoffset + width, yoffset + height / 2));
        polygon.append(QPoint(xoffset + width / 2, yoffset ));
        painter->drawPolygon(polygon);
    }
};

}

TimelineTreeView::TimelineTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setExpandsOnDoubleClick(true);
    setUniformRowHeights(true);
    setSortingEnabled(false);
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(addCollapsed(QModelIndex)));
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(addExpanded(QModelIndex)));
}

TimelineTreeView::~TimelineTreeView()
{
}

void TimelineTreeView::selectFrameIndex(int frameIndex)
{
    /* 現在のキーフレームのインデックスから全てのボーンまたは頂点モーフを選択する処理 */
    QItemSelection selection;
    MotionBaseModel *mbm = static_cast<MotionBaseModel *>(model());
    if (mbm->isTreeModel()) {
        PMDMotionModel *pmm = static_cast<PMDMotionModel *>(mbm);
        foreach (PMDMotionModel::ITreeItem *item, pmm->keys().values()) {
            const QModelIndex &index = pmm->frameIndexToModelIndex(item, frameIndex);
            selection.append(QItemSelectionRange(index));
        }
        QItemSelectionModel *sm = selectionModel();
        sm->select(selection, QItemSelectionModel::ClearAndSelect);
    }
}

void TimelineTreeView::mousePressEvent(QMouseEvent *event)
{
    /* ボーンまたは頂点モーフの名前から対象を選択する処理 */
    const QModelIndex &index = indexAt(event->pos());
    MotionBaseModel *mbm = static_cast<MotionBaseModel *>(model());
    /* 場面タブは除外する */
    if (index.column() == 0 && mbm->isTreeModel()) {
        PMDMotionModel::ITreeItem *item = static_cast<PMDMotionModel::ITreeItem *>(index.internalPointer());
        /* ルートでもカテゴリでもなく、ボーンまたは頂点フレームのキーフレームが選択されていることを確認する */
        if (!item->isRoot() && !item->isCategory()) {
            selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            mbm->selectByModelIndex(index);
        }
    }
    QTreeView::mousePressEvent(event);
}

const QModelIndexList &TimelineTreeView::expandedModelIndices() const
{
    return m_expanded;
}

void TimelineTreeView::addCollapsed(const QModelIndex &index)
{
    m_expanded.removeOne(index);
}

void TimelineTreeView::addExpanded(const QModelIndex &index)
{
    if (!m_expanded.contains(index))
        m_expanded.append(index);
}

TimelineHeaderView::TimelineHeaderView(Qt::Orientation orientation, QWidget *parent)
    : QHeaderView(orientation, parent)
{
    setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    setResizeMode(QHeaderView::Fixed);
    setClickable(true);
    setMovable(false);
    setStretchLastSection(true);
    setSortIndicatorShown(false);
    setDefaultSectionSize(16);
}

TimelineHeaderView::~TimelineHeaderView()
{
}

void TimelineHeaderView::mousePressEvent(QMouseEvent *e)
{
    /* setMovable(false) にすると何故か sectionPressed が効かなくなるので mousePressEvent でシグナルを発行する */
    int modelIndex = logicalIndexAt(e->pos());
    emit frameIndexDidSelect(MotionBaseModel::toFrameIndex(modelIndex));
    QHeaderView::mousePressEvent(e);
}

TimelineWidget::TimelineWidget(MotionBaseModel *base,
                               QWidget *parent) :
    QWidget(parent)
{
    TimelineTreeView *treeView = new TimelineTreeView();
    treeView->setModel(base);
    treeView->setSelectionBehavior(QAbstractItemView::SelectItems);
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    TimelineHeaderView *header = new TimelineHeaderView(Qt::Horizontal);
    connect(header, SIGNAL(frameIndexDidSelect(int)), this, SLOT(setCurrentFrameIndex(int)));
    treeView->setHeader(header);
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    TimelineItemDelegate *delegate = new TimelineItemDelegate(this);
    treeView->setItemDelegate(delegate);
    m_spinBox = new QSpinBox();
    m_spinBox->setMaximum(base->maxFrameCount());
    connect(m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(setCurrentFrameIndex(int)));
    m_label = new QLabel();
    m_button = new QPushButton();
    QHBoxLayout *spinboxLayout = new QHBoxLayout();
    spinboxLayout->addWidget(m_label);
    spinboxLayout->addWidget(m_spinBox);
    spinboxLayout->addWidget(m_button);
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(spinboxLayout);
    mainLayout->addWidget(treeView);
    mainLayout->setAlignment(spinboxLayout, Qt::AlignCenter);
    mainLayout->setContentsMargins(QMargins());
    QItemSelectionModel *sm = treeView->selectionModel();
    connect(sm, SIGNAL(currentColumnChanged(QModelIndex,QModelIndex)), this, SLOT(setCurrentColumnIndex(QModelIndex)));
    connect(base, SIGNAL(motionDidUpdate(vpvl::PMDModel*)), this, SLOT(reexpand()));
    connect(base, SIGNAL(motionDidUpdate(vpvl::PMDModel*)), this, SLOT(setCurrentFrameIndexBySpinBox()));
    retranslate();
    setLayout(mainLayout);
    m_treeView = treeView;
    m_headerView = header;
}

TimelineWidget::~TimelineWidget()
{
}

void TimelineWidget::retranslate()
{
    m_label->setText(tr("Frame Index"));
    m_button->setText(tr("Register"));
}

int TimelineWidget::frameIndex() const
{
    /* 選択状態のモデルインデックスの最初のインデックスからキーフレームのインデックスを求める */
    const QModelIndexList &indices = m_treeView->selectionModel()->selectedIndexes();
    if (!indices.isEmpty()) {
        const QModelIndex &index = indices.first();
        if (index.isValid())
            return MotionBaseModel::toFrameIndex(index);
    }
    return 0;
}

void TimelineWidget::setCurrentColumnIndex(const QModelIndex &index)
{
    int frameIndex = MotionBaseModel::toFrameIndex(index);
    setCurrentFrameIndex(frameIndex);
}

void TimelineWidget::setCurrentFrameIndex(int frameIndex)
{
    /* キーフレームのインデックスを現在の位置として設定し、フレームの列を全て選択状態にした上でスクロールを行う */
    MotionBaseModel *model = static_cast<MotionBaseModel *>(m_treeView->model());
    model->setFrameIndex(frameIndex);
    m_treeView->selectFrameIndex(frameIndex);
    m_treeView->scrollTo(model->index(0, frameIndex + 1));
    m_spinBox->setValue(frameIndex);
    /* モーション移動を行わせるようにシグナルを発行する */
    emit motionDidSeek(static_cast<float>(frameIndex));
}

void TimelineWidget::setCurrentFrameIndexBySpinBox()
{
    setCurrentFrameIndex(m_spinBox->value());
}

void TimelineWidget::reexpand()
{
    /* QAbstractTableModel#reset が行われると QTreeView では閉じてしまうので、開閉状態を reset 前に戻す */
    TimelineTreeView *view = static_cast<TimelineTreeView *>(m_treeView);
    foreach (const QModelIndex &index, view->expandedModelIndices())
        m_treeView->expand(index);
}

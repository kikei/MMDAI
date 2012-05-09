/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2010-2012  hkrn                                    */
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

#include "widgets/InterpolationGraphWidget.h"
#include "widgets/TimelineTabWidget.h"

#include <QtGui/QtGui>
#include <vpvl2/vpvl2.h>

using namespace vpvl2;

InterpolationGraphWidget::InterpolationGraphWidget(BoneMotionModel *bmm, SceneMotionModel *smm, QWidget *parent)
    : QWidget(parent),
      m_boneMotionModel(bmm),
      m_sceneMotionModel(smm),
      m_type(kBone),
      m_index(0),
      m_p1Clicked(false),
      m_p2Clicked(false)
{
    setDefault(m_boneIP.x);
    setDefault(m_boneIP.y);
    setDefault(m_boneIP.z);
    setDefault(m_boneIP.rotation);
    setDefault(m_cameraIP.x);
    setDefault(m_cameraIP.y);
    setDefault(m_cameraIP.z);
    setDefault(m_cameraIP.rotation);
    setDefault(m_cameraIP.fovy);
    setDefault(m_cameraIP.distance);
    int max = kMax + 1;
    setMinimumSize(max, max);
    setMaximumSize(max, max);
}

InterpolationGraphWidget::~InterpolationGraphWidget()
{
}

void InterpolationGraphWidget::setBoneKeyFrames(const QList<BoneMotionModel::KeyFramePtr> &frames)
{
    bool enabled = false;
    if (frames.count() == 1) {
        IBoneKeyframe *frame = frames.first().data();
        frame->getInterpolationParameter(IBoneKeyframe::kX, m_boneIP.x);
        frame->getInterpolationParameter(IBoneKeyframe::kY, m_boneIP.y);
        frame->getInterpolationParameter(IBoneKeyframe::kZ, m_boneIP.z);
        frame->getInterpolationParameter(IBoneKeyframe::kRotation, m_boneIP.rotation);
        updateValues(true);
        enabled = true;
    }
    setEnabled(enabled);
}

void InterpolationGraphWidget::setCameraKeyFrames(const QList<SceneMotionModel::KeyFramePtr> &frames)
{
    ICameraKeyframe *frame = reinterpret_cast<ICameraKeyframe *>(frames.last().data());
    frame->getInterpolationParameter(ICameraKeyframe::kX, m_cameraIP.x);
    frame->getInterpolationParameter(ICameraKeyframe::kY, m_cameraIP.y);
    frame->getInterpolationParameter(ICameraKeyframe::kZ, m_cameraIP.z);
    frame->getInterpolationParameter(ICameraKeyframe::kRotation, m_cameraIP.rotation);
    frame->getInterpolationParameter(ICameraKeyframe::kFovy, m_cameraIP.fovy);
    frame->getInterpolationParameter(ICameraKeyframe::kDistance, m_cameraIP.distance);
    updateValues(true);
}

void InterpolationGraphWidget::setX1(int value)
{
    m_p1.setX(value);
    updateValues(false);
    emit x1ValueDidChange(value);
}

void InterpolationGraphWidget::setX2(int value)
{
    m_p2.setX(value);
    updateValues(false);
    emit x2ValueDidChange(value);
}

void InterpolationGraphWidget::setY1(int value)
{
    m_p1.setY(value);
    updateValues(false);
    emit y1ValueDidChange(value);
}

void InterpolationGraphWidget::setY2(int value)
{
    m_p2.setY(value);
    updateValues(false);
    emit y2ValueDidChange(value);
}

void InterpolationGraphWidget::mousePressEvent(QMouseEvent *event)
{
    int width = kCircleWidth, half = width / 2;
    QPoint p1 = m_p1, p2 = m_p2;
    p1.setY(127 - p1.y());
    p2.setY(qAbs(p2.y() - 127));
    QRect rect1(p1.x() - half, p1.y() - half, width, width);
    if (rect1.contains(event->pos())) {
        m_p1Clicked = true;
        return;
    }
    QRect rect2(p2.x() - half, p2.y() - half, width, width);
    if (rect2.contains(event->pos())) {
        m_p2Clicked = true;
        return;
    }
}

void InterpolationGraphWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = event->pos();
    const int min = kMin, max = kMax;
    if (m_p1Clicked) {
        p.setX(qBound(min, p.x(), max));
        p.setY(qBound(min, max - p.y(), max));
        m_p1 = p;
        emit x1ValueDidChange(p.x());
        emit y1ValueDidChange(p.y());
    }
    else if (m_p2Clicked) {
        p.setX(qBound(min, p.x(), max));
        p.setY(qBound(min, qAbs(p.y() - max), max));
        m_p2 = p;
        emit x2ValueDidChange(p.x());
        emit y2ValueDidChange(p.y());
    }
    update();
}

void InterpolationGraphWidget::mouseReleaseEvent(QMouseEvent * /* event */)
{
    m_p1Clicked = false;
    m_p2Clicked = false;
}

void InterpolationGraphWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    if (isEnabled()) {
        QPainterPath path;
        QPoint p1 = m_p1, p2 = m_p2;
        p1.setY(127 - p1.y());
        p2.setY(qAbs(p2.y() - 127));
        path.moveTo(QPoint(0, 127));
        path.cubicTo(p1, p2, QPoint(127, 0));
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPath(path);
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        int half = kCircleWidth / 2;
        painter.drawEllipse(p1.x() - half, p1.y() - half, kCircleWidth, kCircleWidth);
        painter.drawEllipse(p2.x() - half, p2.y() - half, kCircleWidth, kCircleWidth);
    }
}

void InterpolationGraphWidget::setIndex(int value)
{
    m_index = value;
    updateValues(true);
}

void InterpolationGraphWidget::updateValues(bool import)
{
    switch (m_type) {
    case kBone:
        switch (m_index) {
        case 0:
            setValue(m_boneIP.x, import);
            break;
        case 1:
            setValue(m_boneIP.y, import);
            break;
        case 2:
            setValue(m_boneIP.z, import);
            break;
        case 3:
            setValue(m_boneIP.rotation, import);
            break;
        default:
            qWarning("Out of bone combo index: %d", m_index);
            break;
        }
        m_boneMotionModel->setInterpolationParameter(m_boneIP);
        break;
    case kCamera:
        switch (m_index) {
        case 0:
            setValue(m_cameraIP.x, import);
            break;
        case 1:
            setValue(m_cameraIP.y, import);
            break;
        case 2:
            setValue(m_cameraIP.z, import);
            break;
        case 3:
            setValue(m_cameraIP.rotation, import);
            break;
        case 4:
            setValue(m_cameraIP.distance, import);
            break;
        case 5:
            setValue(m_cameraIP.fovy, import);
            break;
        default:
            qWarning("Out of camera combo index: %d", m_index);
            break;
        }
        m_sceneMotionModel->setCameraInterpolationParameter(m_cameraIP);
        break;
    }
    update();
}

void InterpolationGraphWidget::setValue(QuadWord &q, bool import)
{
    if (import) {
        m_p1.setX(q.x());
        m_p1.setY(q.y());
        m_p2.setX(q.z());
        m_p2.setY(q.w());
        emit x1ValueDidChange(q.x());
        emit y1ValueDidChange(q.y());
        emit x2ValueDidChange(q.z());
        emit y2ValueDidChange(q.w());
    }
    else {
        q.setValue(m_p1.x(), m_p1.y(), m_p2.x(), m_p2.y());
    }
}

void InterpolationGraphWidget::setDefault(QuadWord &q)
{
    q.setValue(20, 20, 107, 107);
}
/*
  Q Light Controller Plus
  patchgridwidget.cpp

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QApplication>
#include <QScrollBar>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QToolTip>
#include <QDebug>

#include "qlcfixturemode.h"
#include "qlcchannel.h"
#include "patchcolor.h"
#include "universe.h"
#include "fixture.h"
#include "doc.h"

#include "patchgridwidget.h"

#define RULER_WIDTH  28
#define RULER_HEIGHT 16

PatchGridWidget::PatchGridWidget(Doc* doc, QWidget* parent)
    : QAbstractScrollArea(parent)
    , m_doc(doc)
    , m_universe(0)
    , m_cellSize(22)
    , m_orientation(RowMajor)
    , m_readOnly(false)
    , m_hoverAddress(-1)
    , m_dragFixture(Fixture::invalidId())
    , m_dragGrabOffset(0)
    , m_dragTargetAddress(-1)
    , m_dragging(false)
{
    Q_ASSERT(doc != NULL);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);

    refresh();
}

PatchGridWidget::~PatchGridWidget()
{
}

void PatchGridWidget::setUniverse(int universe)
{
    if (m_universe == universe)
        return;

    m_universe = universe;
    m_hoverAddress = -1;
    refresh();
}

int PatchGridWidget::universe() const
{
    return m_universe;
}

void PatchGridWidget::setCellSize(int size)
{
    size = qBound(10, size, 48);
    if (m_cellSize == size)
        return;

    m_cellSize = size;
    updateScrollBars();
    viewport()->update();
}

int PatchGridWidget::cellSize() const
{
    return m_cellSize;
}

void PatchGridWidget::setOrientation(Orientation orientation)
{
    if (m_orientation == orientation)
        return;

    m_orientation = orientation;
    updateScrollBars();
    viewport()->update();
}

PatchGridWidget::Orientation PatchGridWidget::orientation() const
{
    return m_orientation;
}

void PatchGridWidget::setReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
}

bool PatchGridWidget::isReadOnly() const
{
    return m_readOnly;
}

void PatchGridWidget::setSelectedFixtures(const QList <quint32>& ids)
{
    if (m_selected == ids)
        return;

    m_selected = ids;
    viewport()->update();
}

QList <quint32> PatchGridWidget::selectedFixtures() const
{
    return m_selected;
}

void PatchGridWidget::refresh()
{
    m_cells.clear();

    for (int addr = 0; addr < UNIVERSE_SIZE; addr++)
    {
        QList <quint32> owners = m_doc->fixturesAtAddress((m_universe * UNIVERSE_SIZE) + addr);
        if (owners.isEmpty() == false)
            m_cells.insert(addr, owners);
    }

    updateScrollBars();
    viewport()->update();
}

/****************************************************************************
 * Geometry
 ****************************************************************************/

int PatchGridWidget::rows() const
{
    return UNIVERSE_SIZE / columns();
}

int PatchGridWidget::columns() const
{
    return 32;
}

QRect PatchGridWidget::cellRect(int column, int row) const
{
    return QRect(RULER_WIDTH + (column * m_cellSize),
                 RULER_HEIGHT + (row * m_cellSize),
                 m_cellSize - 1, m_cellSize - 1);
}

QRect PatchGridWidget::cellRect(int address) const
{
    if (address < 0 || address >= UNIVERSE_SIZE)
        return QRect();

    QPoint pos = cellPositionForAddress(address);
    QRect rect = cellRect(pos.x(), pos.y());

    return rect.translated(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
}

int PatchGridWidget::addressAtCell(int column, int row) const
{
    if (column < 0 || column >= columns() || row < 0 || row >= rows())
        return -1;

    if (m_orientation == RowMajor)
        return (row * columns()) + column;
    else
        return (column * rows()) + row;
}

QPoint PatchGridWidget::cellPositionForAddress(int address) const
{
    if (m_orientation == RowMajor)
        return QPoint(address % columns(), address / columns());
    else
        return QPoint(address / rows(), address % rows());
}

int PatchGridWidget::addressAt(const QPoint& pos) const
{
    int x = pos.x() + horizontalScrollBar()->value() - RULER_WIDTH;
    int y = pos.y() + verticalScrollBar()->value() - RULER_HEIGHT;

    if (x < 0 || y < 0)
        return -1;

    return addressAtCell(x / m_cellSize, y / m_cellSize);
}

quint32 PatchGridWidget::fixtureAt(const QPoint& pos) const
{
    QList <quint32> owners = fixturesAt(pos);
    if (owners.isEmpty() == true)
        return Fixture::invalidId();

    return owners.first();
}

QList <quint32> PatchGridWidget::fixturesAt(const QPoint& pos) const
{
    int address = addressAt(pos);
    if (address < 0)
        return QList <quint32> ();

    return m_cells.value(address);
}

int PatchGridWidget::fixtureStartInUniverse(quint32 id) const
{
    Fixture* fxi = m_doc->fixture(id);
    if (fxi == NULL)
        return -1;

    quint32 universeStart = m_universe * UNIVERSE_SIZE;
    quint32 start = fxi->universeAddress();

    if (start >= universeStart && start < universeStart + UNIVERSE_SIZE)
        return start - universeStart;

    // The fixture crossed into this universe from the previous one
    if (start < universeStart)
        return 0;

    return -1;
}

void PatchGridWidget::updateScrollBars()
{
    int contentWidth = RULER_WIDTH + (columns() * m_cellSize);
    int contentHeight = RULER_HEIGHT + (rows() * m_cellSize);

    horizontalScrollBar()->setRange(0, qMax(0, contentWidth - viewport()->width()));
    horizontalScrollBar()->setPageStep(viewport()->width());
    horizontalScrollBar()->setSingleStep(m_cellSize);

    verticalScrollBar()->setRange(0, qMax(0, contentHeight - viewport()->height()));
    verticalScrollBar()->setPageStep(viewport()->height());
    verticalScrollBar()->setSingleStep(m_cellSize);
}

/****************************************************************************
 * Painting
 ****************************************************************************/

void PatchGridWidget::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    updateScrollBars();
}

void PatchGridWidget::drawCell(QPainter& painter, int address)
{
    QPoint pos = cellPositionForAddress(address);
    QRect rect = cellRect(pos.x(), pos.y());
    QList <quint32> owners = m_cells.value(address);

    QColor base = palette().base().color();
    QColor fill = (pos.x() + pos.y()) % 2 ? base.lighter(108) : base;

    if (owners.isEmpty() == false)
    {
        Fixture* fxi = m_doc->fixture(owners.first());
        if (fxi != NULL)
        {
            QColor color = PatchColor::fixtureColor(fxi->id());
            fill = m_selected.contains(fxi->id()) ? color.lighter(130) : color;
        }
    }

    painter.fillRect(rect, fill);

    if (owners.isEmpty() == false)
    {
        bool isSelected = false;
        foreach (quint32 id, owners)
        {
            if (m_selected.contains(id) == true)
            {
                isSelected = true;
                break;
            }
        }

        if (owners.count() > 1)
        {
            // Address shared by multiple fixtures
            painter.fillRect(rect, QBrush(QColor(0xB0, 0x20, 0x20, 130), Qt::BDiagPattern));
            painter.setPen(QPen(QColor(0xD0, 0x30, 0x30), 1));
            painter.drawRect(rect);

            // Report how many fixtures share this channel
            if (m_cellSize >= 22)
            {
                painter.setPen(Qt::white);
                QFont font = painter.font();
                font.setPointSize(6);
                font.setBold(true);
                painter.setFont(font);
                painter.drawText(rect.adjusted(0, 0, -2, 0), Qt::AlignBottom | Qt::AlignRight,
                                 QString::number(owners.count()));
                font.setBold(false);
                painter.setFont(font);
            }
        }
        else if (isSelected == true)
        {
            painter.setPen(QPen(palette().highlight().color(), 2));
            painter.drawRect(rect.adjusted(1, 1, -1, -1));
        }
        else
        {
            painter.setPen(QPen(PatchColor::darker(fill), 1));
            painter.drawRect(rect);
        }

        if (address == m_hoverAddress)
        {
            painter.setPen(QPen(palette().highlightedText().color(), 1, Qt::DotLine));
            painter.drawRect(rect);
        }

        // Channel address number (top-left corner)
        if (m_cellSize >= 22)
        {
            painter.setPen(fill.lightness() > 140 ? Qt::black : Qt::white);
            QFont font = painter.font();
            font.setPointSize(6);
            painter.setFont(font);
            painter.drawText(rect.adjusted(2, 0, 0, 0), Qt::AlignTop | Qt::AlignLeft,
                             QString::number(address + 1));
        }
    }
    else
    {
        painter.setPen(QPen(palette().mid().color(), 1));
        painter.drawRect(rect);
    }
}

void PatchGridWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(viewport());
    painter.fillRect(viewport()->rect(), palette().base());

    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());

    // Rulers
    QColor rulerColor = palette().button().color();
    painter.fillRect(QRect(0, 0, RULER_WIDTH + (columns() * m_cellSize), RULER_HEIGHT), rulerColor);
    painter.fillRect(QRect(0, 0, RULER_WIDTH, RULER_HEIGHT + (rows() * m_cellSize)), rulerColor);

    QFont rulerFont = painter.font();
    rulerFont.setPointSize(6);
    painter.setFont(rulerFont);
    painter.setPen(palette().buttonText().color());

    for (int col = 0; col < columns(); col++)
    {
        if (col % 5 == 0)
        {
            QRect rect(RULER_WIDTH + (col * m_cellSize), 0, m_cellSize * 3, RULER_HEIGHT);
            painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter,
                             QString::number(addressAtCell(col, 0) + 1));
        }
    }

    for (int row = 0; row < rows(); row++)
    {
        QRect rect(0, RULER_HEIGHT + (row * m_cellSize), RULER_WIDTH - 2, m_cellSize);
        painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(addressAtCell(0, row) + 1));
    }

    // Channel cells
    for (int address = 0; address < UNIVERSE_SIZE; address++)
        drawCell(painter, address);

    // Fixture names over their channel blocks
    QFont nameFont = painter.font();
    nameFont.setPointSize(7);
    painter.setFont(nameFont);

    QHash <quint32, QRect> blocks;
    for (int address = 0; address < UNIVERSE_SIZE; address++)
    {
        QList <quint32> owners = m_cells.value(address);
        if (owners.isEmpty() == true)
            continue;

        quint32 id = owners.first();
        QPoint pos = cellPositionForAddress(address);
        QRect rect = cellRect(pos.x(), pos.y());

        if (blocks.contains(id) == true)
            blocks[id] = blocks[id].united(rect);
        else
            blocks.insert(id, rect);
    }

    QHashIterator <quint32, QRect> blockIt(blocks);
    while (blockIt.hasNext() == true)
    {
        blockIt.next();
        if (blockIt.value().width() < 70)
            continue;

        Fixture* fxi = m_doc->fixture(blockIt.key());
        if (fxi == NULL)
            continue;

        QColor color = PatchColor::fixtureColor(fxi->id());
        QColor textColor = color.lightness() > 140 ? Qt::black : Qt::white;
        QColor shadow = textColor == Qt::black ? QColor(255, 255, 255, 170) : QColor(0, 0, 0, 170);

        // Draw a shadow to keep the name readable on any fill/hatch
        painter.setPen(shadow);
        painter.drawText(blockIt.value().adjusted(3, 1, -1, 1), Qt::AlignCenter, fxi->name());
        painter.setPen(textColor);
        painter.drawText(blockIt.value().adjusted(2, 0, -2, 0), Qt::AlignCenter, fxi->name());
    }

    // Drag preview
    if (m_dragging == true && m_dragTargetAddress >= 0)
    {
        Fixture* fxi = m_doc->fixture(m_dragFixture);
        if (fxi != NULL)
        {
            bool free = true;
            for (quint32 i = 0; i < fxi->channels(); i++)
            {
                int addr = m_dragTargetAddress + int(i);
                if (addr >= UNIVERSE_SIZE)
                    break;
                if (m_cells.value(addr).isEmpty() == false)
                {
                    QList <quint32> owners = m_cells.value(addr);
                    if (owners.count() != 1 || owners.first() != m_dragFixture)
                    {
                        free = false;
                        break;
                    }
                }
            }

            QColor preview = free ? QColor(0x40, 0xC0, 0x50, 110) : QColor(0xE0, 0x90, 0x30, 110);
            QPoint pos = cellPositionForAddress(m_dragTargetAddress);
            QRect rect = cellRect(pos.x(), pos.y());
            rect.setWidth(qMin(int(fxi->channels()), UNIVERSE_SIZE - m_dragTargetAddress) * m_cellSize);

            painter.fillRect(rect, preview);
            painter.setPen(QPen(free ? QColor(0x20, 0x90, 0x30) : QColor(0xC0, 0x60, 0x10), 2));
            painter.drawRect(rect);
        }
    }
}

QString PatchGridWidget::tooltipForAddress(int address) const
{
    QList <quint32> owners = m_cells.value(address);
    if (owners.isEmpty() == true)
        return QString();

    QString text = QString("<B>%1: %2</B>").arg(tr("Address")).arg(address + 1);
    foreach (quint32 id, owners)
    {
        Fixture* fxi = m_doc->fixture(id);
        if (fxi == NULL)
            continue;

        quint32 channel = (m_universe * UNIVERSE_SIZE) + address - fxi->universeAddress();
        const QLCChannel* ch = fxi->channel(channel);
        QString chName = ch != NULL ? ch->name() : tr("Channel");

        text += QString("<BR>%1 - %2 %3: %4")
                    .arg(fxi->name())
                    .arg(tr("channel")).arg(channel + 1)
                    .arg(chName);
    }

    if (owners.count() > 1)
        text += QString("<BR><B>%1</B>").arg(tr("Address shared by multiple fixtures"));

    return text;
}

/****************************************************************************
 * Interaction
 ****************************************************************************/

void PatchGridWidget::mousePressEvent(QMouseEvent* event)
{
    setFocus();

    int address = addressAt(event->pos());
    if (address < 0)
    {
        m_dragFixture = Fixture::invalidId();
        return;
    }

    QList <quint32> owners = m_cells.value(address);
    if (owners.isEmpty() == true)
    {
        m_dragFixture = Fixture::invalidId();
        return;
    }

    emit fixtureClicked(owners.first(), event->modifiers());

    if (m_readOnly == false && event->button() == Qt::LeftButton && owners.count() == 1)
    {
        m_dragFixture = owners.first();
        m_dragStartPos = event->pos();
        m_dragGrabOffset = address - fixtureStartInUniverse(m_dragFixture);
        m_dragTargetAddress = -1;
        m_dragging = false;
    }
}

void PatchGridWidget::mouseMoveEvent(QMouseEvent* event)
{
    int address = addressAt(event->pos());

    if (m_dragFixture != Fixture::invalidId() && (event->buttons() & Qt::LeftButton))
    {
        if (m_dragging == false &&
            (event->pos() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance())
        {
            m_dragging = true;
        }

        if (m_dragging == true && address >= 0)
        {
            m_dragTargetAddress = qBound(0, address - m_dragGrabOffset, UNIVERSE_SIZE - 1);
            viewport()->update();
        }
        return;
    }

    if (address != m_hoverAddress)
    {
        m_hoverAddress = address;
        viewport()->update();

        if (address >= 0)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QToolTip::showText(event->globalPosition().toPoint(), tooltipForAddress(address), this);
#else
            QToolTip::showText(event->globalPos(), tooltipForAddress(address), this);
#endif
        }
        else
        {
            QToolTip::hideText();
        }
    }
}

void PatchGridWidget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)

    if (m_dragging == true && m_dragTargetAddress >= 0 &&
        m_dragFixture != Fixture::invalidId())
    {
        emit fixtureMoveRequested(m_dragFixture,
                                  quint32(m_universe * UNIVERSE_SIZE + m_dragTargetAddress));
    }

    m_dragFixture = Fixture::invalidId();
    m_dragTargetAddress = -1;
    m_dragging = false;
    viewport()->update();
}

void PatchGridWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    quint32 id = fixtureAt(event->pos());
    if (id != Fixture::invalidId())
        emit fixtureDoubleClicked(id);
}

void PatchGridWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event)

    m_hoverAddress = -1;
    viewport()->update();
}

void PatchGridWidget::contextMenuEvent(QContextMenuEvent* event)
{
    quint32 id = fixtureAt(event->pos());
    if (id != Fixture::invalidId())
        emit fixtureContextMenuRequested(id, event->globalPos());
}

void PatchGridWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_selected.count() == 1 && m_readOnly == false)
    {
        Fixture* fxi = m_doc->fixture(m_selected.first());
        if (fxi != NULL)
        {
            int step = (event->modifiers() & Qt::ShiftModifier) ? 10 : 1;
            int delta = 0;

            switch (event->key())
            {
                case Qt::Key_Left: delta = -step; break;
                case Qt::Key_Right: delta = step; break;
                case Qt::Key_Up:
                    delta = -(m_orientation == RowMajor ? columns() : rows()) * step;
                    break;
                case Qt::Key_Down:
                    delta = (m_orientation == RowMajor ? columns() : rows()) * step;
                    break;
                default:
                    QAbstractScrollArea::keyPressEvent(event);
                    return;
            }

            qint64 target = qint64(fxi->universeAddress()) + delta;
            if (target < 0)
                target = 0;

            emit fixtureMoveRequested(fxi->id(), quint32(target));
            return;
        }
    }

    QAbstractScrollArea::keyPressEvent(event);
}

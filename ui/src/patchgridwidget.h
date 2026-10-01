/*
  Q Light Controller Plus
  patchgridwidget.h

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

#ifndef PATCHGRIDWIDGET_H
#define PATCHGRIDWIDGET_H

#include <QAbstractScrollArea>
#include <QHash>
#include <QList>

class QPainter;
class Fixture;
class Doc;

/** @addtogroup ui_fixtures
 * @{
 */

/**
 * A visual representation of a DMX universe: one cell per channel, coloured
 * by fixture. Addresses shared by multiple fixtures are highlighted, and a
 * fixture block can be dragged to another address.
 */
class PatchGridWidget final : public QAbstractScrollArea
{
    Q_OBJECT
    Q_DISABLE_COPY(PatchGridWidget)

public:
    enum Orientation
    {
        RowMajor = 0,
        ColumnMajor
    };

    PatchGridWidget(Doc* doc, QWidget* parent = NULL);
    ~PatchGridWidget();

    /** Set the displayed universe (zero-based index) */
    void setUniverse(int universe);

    /** Get the displayed universe (zero-based index) */
    int universe() const;

    /** Set the size in pixels of a single DMX channel cell */
    void setCellSize(int size);

    /** Get the size in pixels of a single DMX channel cell */
    int cellSize() const;

    /** Set the cells filling order */
    void setOrientation(Orientation orientation);

    /** Get the cells filling order */
    Orientation orientation() const;

    /** When read-only, fixture blocks cannot be dragged around */
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;

    /** Set the currently selected fixtures */
    void setSelectedFixtures(const QList <quint32>& ids);

    /** Get the currently selected fixtures */
    QList <quint32> selectedFixtures() const;

    /** Rebuild the channel map from the current Doc contents */
    void refresh();

    /** Get the primary fixture occupying the given viewport position.
     *  Returns Fixture::invalidId() if the cell is free */
    quint32 fixtureAt(const QPoint& pos) const;

    /** Get all the fixtures occupying the given viewport position */
    QList <quint32> fixturesAt(const QPoint& pos) const;

    /** Get the in-universe address (0-511) at the given viewport position.
     *  Returns -1 if the position is outside the channel cells */
    int addressAt(const QPoint& pos) const;

    /** Get the rectangle of the given in-universe address, in viewport coords */
    QRect cellRect(int address) const;

signals:
    /** A fixture has been clicked. $modifiers reports Ctrl/Shift state */
    void fixtureClicked(quint32 id, Qt::KeyboardModifiers modifiers);

    /** A fixture has been double clicked */
    void fixtureDoubleClicked(quint32 id);

    /** Context menu requested on a fixture */
    void fixtureContextMenuRequested(quint32 id, const QPoint& globalPos);

    /** A fixture block has been dragged to a new absolute address */
    void fixtureMoveRequested(quint32 id, quint32 universeAddress);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    int rows() const;
    int columns() const;
    void updateScrollBars();
    QRect cellRect(int column, int row) const;
    int addressAtCell(int column, int row) const;
    QPoint cellPositionForAddress(int address) const;
    int fixtureStartInUniverse(quint32 id) const;
    QString tooltipForAddress(int address) const;
    void drawCell(QPainter& painter, int address);

private:
    Doc* m_doc;
    int m_universe;
    int m_cellSize;
    Orientation m_orientation;
    bool m_readOnly;

    /** in-universe address (0-511) -> occupying fixture IDs (primary first) */
    QHash <int, QList <quint32> > m_cells;

    QList <quint32> m_selected;
    int m_hoverAddress;

    // drag state
    quint32 m_dragFixture;
    QPoint m_dragStartPos;
    int m_dragGrabOffset;
    int m_dragTargetAddress;
    bool m_dragging;
};

/** @} */

#endif // PATCHGRIDWIDGET_H

/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A user-interactible node grapg implemented as a subclass of QGraphicsScene.
 *
 * Usage is along the lines of:
 *
 *     // Establish the scene and set up a view to draw it.
 *     QGraphicsView *view = new QGraphicsView(this);
 *     QGraphicsScen *scene = new InteractibleNodeGraph(this);
 *     view->setScene(scene);
 *
 *     // Create a node to be displayed in the scene.
 *     InteractibleNodeGraphNode *node = new InteractibleNodeGraphNode("Test");
 *     node->setFlags(node->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
 *     scene->addItem(node);
 *
 * Please see Qt's documentation on QGraphicsScene (and QGraphicsView) for more
 * information on how to use the node graph.
 *
 * Each node in the graph has one or more input and/or output edges, via which
 * connections can be established to other nodes.
 *
 * The user can connect nodes to each other by pressing the mouse button over a
 * node's edge and releasing the press over the target edge. In-code, this is
 * implemented as a 'connection event' (node_connection_event_s).
 *
 */

#include <QApplication>
#include <QMessageBox>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QDebug>
#include "InteractibleNodeGraph.h"
#include "common/assert.h"

InteractibleNodeGraph::InteractibleNodeGraph(QObject *parent) :
    QGraphicsScene(parent)
{
    return;
}

void InteractibleNodeGraph::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // If this press was over a node edge, start an edge connection event.
        const auto intersectedItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect);
        for (const auto item: intersectedItems)
        {
            const auto node = dynamic_cast<InteractibleNodeGraphNode*>(item);

            if (node)
            {
                // Make the clicked node the top-most in the graph.
                double maxZ = 0;
                const auto sceneItems = this->items();
                for (auto item: sceneItems)
                {
                    if (item->zValue() > maxZ)
                    {
                        maxZ = item->zValue();
                    }
                }
                node->setZValue(maxZ+1);

                const auto edge = node->intersected_edge(event->scenePos());

                if (edge)
                {
                    this->hasExclusiveMouseClickFocus = true;
                    this->start_connection_event(edge, event->scenePos());
                    return;
                }

                break;
            }
        }
    }

    QGraphicsScene::mousePressEvent(event);

    return;
}

void InteractibleNodeGraph::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // If this release was over a node edge, finalize the current edge connection event.
        const auto intersectedItems = this->items(event->scenePos(), Qt::IntersectsItemBoundingRect);
        for (const auto item: intersectedItems)
        {
            const auto node = dynamic_cast<InteractibleNodeGraphNode*>(item);

            if (node)
            {
                const auto edge = node->intersected_edge(event->scenePos());

                if (edge)
                {
                    complete_connection_event(edge);
                    this->hasExclusiveMouseClickFocus = false;
                    return;
                }

                // In case the user released the mouse button after dragging a node.
                this->update_scene_connections();

                break;
            }
        }

        this->reset_current_connection_event();
    }

    QGraphicsScene::mouseReleaseEvent(event);

    if (!this->connectionEvent.sourceEdge)
    {
        this->hasExclusiveMouseClickFocus = false;
    }

    return;
}

void InteractibleNodeGraph::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);

    // Assume that if the user is holding the mouse button down while moving
    // the cursor, they're dragging one of the items in the scene.
    if (QApplication::mouseButtons() == Qt::LeftButton)
    {
        // Dragging a node.
        if (this->selectedItems().count())
        {
            this->update_scene_connections();
            emit this->nodeMoved();
        }
        // Dragging a connection from an edge.
        else if (this->connectionEvent.sourceEdge)
        {
            this->update_scene_connections();
        }

        this->connectionEvent.mousePos = event->scenePos();
    }

    return;
}

void InteractibleNodeGraph::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Draw a grid over the background.
    {
        const QRect intRect = rect.toRect();
        int x = (intRect.left() - (intRect.left() % this->grid_size()));
        int y = (intRect.top() - (intRect.top() % this->grid_size()));

        const auto line_opacity_at = [this](const int value)->int
        {
            return ((value % (this->grid_size() * 5))? 60 : 120);
        };

        while (x <= intRect.right())
        {
            painter->setPen(QPen(QColor(0, 0, 0, line_opacity_at(x)), 1, Qt::SolidLine));
            painter->drawLine(x, intRect.top(), x, intRect.bottom());
            x += this->grid_size();
        }

        while (y <= intRect.bottom())
        {
            painter->setPen(QPen(QColor(0, 0, 0, line_opacity_at(y)), 1, Qt::SolidLine));
            painter->drawLine(intRect.left(), y, intRect.right(), y);
            y += this->grid_size();
        }
    }

    QGraphicsScene::drawBackground(painter, rect);

    return;
}

// Reposition any connections between nodes in the scene, to account for any changes
// in the nodes' positioning etc.
void InteractibleNodeGraph::update_scene_connections(void)
{
    // Refresh connections between nodes.
    for (const auto &connection: this->edgeConnections)
    {
        const QPoint p1 = QPoint(connection.edge1->parentNode->mapToScene(connection.edge1->rect.center()).x(),
                                 connection.edge1->parentNode->mapToScene(connection.edge1->rect.center()).y());

        const QPoint p2 = QPoint(connection.edge2->parentNode->mapToScene(connection.edge2->rect.center()).x(),
                                 connection.edge2->parentNode->mapToScene(connection.edge2->rect.center()).y());

        const QLine line = QLine(p1, p2);

        connection.line->setLine(line);
    }

    // If the user is dragging a new connection, draw a line from the source
    // edge to the cursor.
    if (this->connectionEvent.sourceEdge)
    {
        const QPoint p1 = QPoint(this->connectionEvent.sourceEdge->parentNode->mapToScene(this->connectionEvent.sourceEdge->rect.center()).x(),
                                 this->connectionEvent.sourceEdge->parentNode->mapToScene(this->connectionEvent.sourceEdge->rect.center()).y());

        const QPoint p2 = QPoint(this->connectionEvent.mousePos.x(), this->connectionEvent.mousePos.y());

        const QLine line = QLine(p1, p2);

        if (this->connectionEvent.graphicsLine)
        {
            this->connectionEvent.graphicsLine->setLine(line);
            this->connectionEvent.graphicsLine->setZValue(std::numeric_limits<qreal>::max());
        }
        else
        {
            this->connectionEvent.graphicsLine = this->addLine(line, QPen(QColor("white"), 2));
        }
    }

    return;
}

bool InteractibleNodeGraph::get_user_permission_for_node_deletion(void) const
{
    // For now, let's not bother the user with queries.
    return true;

    return (
        QMessageBox::Yes == QMessageBox::question(
            nullptr,
            "Confirm node deletion",
            "Are you sure you want to delete the selected node(s)?",
            (QMessageBox::No | QMessageBox::Yes)
        )
    );
}

void InteractibleNodeGraph::remove_selected_nodes(void)
{
    const QList<QGraphicsItem*> selectedNodes = this->selectedItems();

    if (
        selectedNodes.count() &&
        this->get_user_permission_for_node_deletion()
    ){
        for (QGraphicsItem *node: selectedNodes)
        {
            this->remove_node(dynamic_cast<InteractibleNodeGraphNode*>(node), true);
        }
    }

    return;
}

void InteractibleNodeGraph::remove_node(InteractibleNodeGraphNode *const node, const bool haveUserPermission)
{
    if (node && (haveUserPermission || this->get_user_permission_for_node_deletion()))
    {
        node->disconnect_all_edges();
        this->removeItem(node);
        emit this->nodeRemoved(node);
    }

    return;
}

void InteractibleNodeGraph::disconnect_scene_edges(
    const node_edge_s *const sourceEdge,
    const node_edge_s *const targetEdge,
    const bool noEmit
)
{
    const auto existingConnection = std::find_if(this->edgeConnections.begin(), this->edgeConnections.end(), [=](const node_edge_connection_s &connection)
    {
        return ((connection.edge1 == sourceEdge && connection.edge2 == targetEdge) ||
                (connection.edge1 == targetEdge && connection.edge2 == sourceEdge));
    });

    k_assert((existingConnection != this->edgeConnections.end()), "Was asked to disconnected node edges that were not connected in the first place.");

    this->removeItem((*existingConnection).line);
    delete (*existingConnection).line;
    this->edgeConnections.erase(existingConnection);

    if (!noEmit)
    {
        emit this->edgeConnectionRemoved(sourceEdge, targetEdge);
    }

    return;
}

// Connect the two edges to each other in the scene using a line.
void InteractibleNodeGraph::connect_scene_edges(
    const node_edge_s *const sourceEdge,
    const node_edge_s *const targetEdge
)
{
    const QPoint p1 = QPoint(sourceEdge->parentNode->mapToScene(sourceEdge->rect.center()).x(),
                             sourceEdge->parentNode->mapToScene(sourceEdge->rect.center()).y());

    const QPoint p2 = QPoint(targetEdge->parentNode->mapToScene(targetEdge->rect.center()).x(),
                             targetEdge->parentNode->mapToScene(targetEdge->rect.center()).y());

    const QLine line = QLine(p1, p2);

    // Find whether a line between these edges already exists.
    const auto existingConnection = std::find_if(this->edgeConnections.begin(), this->edgeConnections.end(), [=](const node_edge_connection_s &connection)
    {
        return ((connection.edge1 == sourceEdge && connection.edge2 == targetEdge) ||
                (connection.edge1 == targetEdge && connection.edge2 == sourceEdge));
    });

    if (existingConnection != this->edgeConnections.end())
    {
        (*existingConnection).line->setLine(line);
    }
    else
    {
        this->edgeConnections.push_back(node_edge_connection_s(sourceEdge, targetEdge, this->addLine(line, QPen(QColor("#d0d0d0"), 2))));
    }

    emit this->edgeConnectionAdded(sourceEdge, targetEdge);

    return;
}

void InteractibleNodeGraph::start_connection_event(
    node_edge_s *const sourceEdge,
    const QPointF mousePos
)
{
    if (this->connectionEvent.sourceEdge)
    {
        qCritical() << "The previous connect event must finish before another can be started.";
        return;
    }

    this->connectionEvent.sourceEdge = sourceEdge;
    this->connectionEvent.mousePos = mousePos;

    return;
}

void InteractibleNodeGraph::complete_connection_event(node_edge_s *const finalEdge)
{
    if (!this->connectionEvent.sourceEdge)
    {
        qCritical() << "Can't finalize a connect event without a source edge.";
        return;
    }

    this->connectionEvent.sourceEdge->connect_to(finalEdge);
    this->reset_current_connection_event();

    return;
}

void InteractibleNodeGraph::reset_current_connection_event(void)
{
    this->connectionEvent.sourceEdge = nullptr;
    this->connectionEvent.mousePos = QPointF();

    if (this->connectionEvent.graphicsLine)
    {
        this->removeItem(this->connectionEvent.graphicsLine);
        delete this->connectionEvent.graphicsLine;
        this->connectionEvent.graphicsLine = nullptr;
    }

    return;
}

void InteractibleNodeGraph::reset_scene(void)
{
    this->clear();
    this->edgeConnections.clear();

    return;
}

int InteractibleNodeGraph::grid_size(void) const
{
    return this->gridSize;
}

bool InteractibleNodeGraph::has_exclusive_mouse_click_focus(void) const
{
    return this->hasExclusiveMouseClickFocus;
}

/*
 * 2019 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_SUBCLASSES_QGRAPHICSSCENE_INTERACTIBLE_NODE_GRAPH_H
#define VCS_DISPLAY_QT_SUBCLASSES_QGRAPHICSSCENE_INTERACTIBLE_NODE_GRAPH_H

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QDebug>
#include "QGraphicsItem_interactible_node_graph_node.h"

class InteractibleNodeGraph : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit InteractibleNodeGraph(QObject *parent = 0);

    void update_scene_connections(void);
    void connect_scene_edges(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge);
    void disconnect_scene_edges(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge, const bool noEmit = false);
    void remove_node(InteractibleNodeGraphNode *const node, const bool haveUserPermission = false);
    void remove_selected_nodes(void);
    void reset_scene(void);
    int grid_size(void) const;

signals:
    // Emitted when the given two edges are connected in the scene.
    void edgeConnectionAdded(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge);

    // Emitted when the given two edges are disconnected in the scene.
    void edgeConnectionRemoved(const node_edge_s *const sourceEdge, const node_edge_s *const targetEdge);

    // Emitted when the given node is removed from the scene.
    void nodeRemoved(InteractibleNodeGraphNode *const node);

    // Emitted when one or mode nodes in the scene are moved.
    void nodeMoved(void);

private:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void drawBackground(QPainter *painter, const QRectF &rect);

    bool get_user_permission_for_node_deletion(void) const;
    void complete_connection_event(node_edge_s *const finalEdge);
    void start_connection_event(node_edge_s *const sourceEdge, const QPointF mousePos);
    void reset_current_connection_event(void);

    node_connection_event_s connectionEvent;
    std::vector<node_edge_connection_s> edgeConnections;

    int gridSize = 15;
};

#endif

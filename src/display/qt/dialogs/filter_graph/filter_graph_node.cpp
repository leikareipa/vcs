/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QPainter>
#include <QStyle>
#include "filter/filter.h"
#include "common/globals.h"
#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "display/qt/subclasses/QGraphicsScene_interactible_node_graph.h"

FilterGraphNode::FilterGraphNode(const filter_node_type_e type,
                                 const QString title,
                                 const unsigned width,
                                 const unsigned height) :
    InteractibleNodeGraphNode(title, width, height),
    filterType(type)
{
    this->generate_right_click_menu();

    return;
}

FilterGraphNode::~FilterGraphNode()
{
    kf_delete_filter_instance(this->associatedFilter);

    this->rightClickMenu->close();
    this->rightClickMenu->deleteLater();

    return;
}

QRectF FilterGraphNode::boundingRect(void) const
{
    const int margin = 7;

    return QRectF(-margin,
                  -margin,
                  (this->width + (margin * 2)),
                  (this->height + (margin * 2)));
}

void FilterGraphNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)widget;

    QFont bodyFont = painter->font();
    QFont titleFont = painter->font();

    const unsigned edgePenThickness = 2;
    const unsigned borderRadius = 3;

    // Draw the node's body.
    {
        painter->setFont(bodyFont);

        // Background.
        {
            // Node's background.
            painter->setPen(QPen(QColor((option->state & QStyle::State_Selected)? "#c0c0c0" : this->current_background_color()), edgePenThickness, Qt::SolidLine));
            painter->setBrush(QBrush(QColor("#555555")));
            painter->drawRoundedRect(QRect(0, 0, this->width, this->height), borderRadius, borderRadius);

            // Title bar's background.
            painter->setPen(QPen(QColor("transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(QColor("black"), (this->is_enabled()? Qt::SolidPattern : Qt::BDiagPattern)));
            painter->drawRoundedRect(QRect(6, 8, (this->width - 12), 24), borderRadius, borderRadius);
        }

        // Connection points (edges).
        for (const auto &edge: this->edges)
        {
            painter->setPen(QPen(QColor("black"), edgePenThickness, Qt::SolidLine));
            painter->setBrush(QBrush(QColor("lightseagreen")));
            painter->drawRoundedRect(edge.rect, borderRadius, borderRadius);
        }
    }

    // Draw the title.
    const QString elidedTitle = QFontMetrics(titleFont).elidedText(this->title, Qt::ElideRight, (this->width - 42));
    painter->setFont(titleFont);
    painter->setPen(QColor(this->is_enabled()? "white" : "lightgray"));
    painter->drawText((this->input_edge()? 22 : 14), 26, elidedTitle);

    return;
}

void FilterGraphNode::set_background_color(const QString &colorName)
{
    // If we recognize this color name.
    if ((this->backgroundColorList.indexOf(colorName) >= 0) &&
        (this->backgroundColor != colorName))
    {
        this->backgroundColor = colorName;
        this->update();

        emit this->background_color_changed(colorName);
    }

    return;
}

const QList<QString>& FilterGraphNode::background_color_list(void)
{
    return this->backgroundColorList;
}

const QString& FilterGraphNode::current_background_color_name(void)
{
    return this->backgroundColor;
}

const QColor FilterGraphNode::current_background_color(void)
{
    const QString currentColor = this->backgroundColor.toLower();

    if (currentColor == "black") return QColor("black");
    else if (currentColor == "green") return QColor("lawngreen");
    else if (currentColor == "magenta") return QColor("mediumvioletred");

    return this->backgroundColor;
}

bool FilterGraphNode::is_enabled(void) const
{
    return this->isEnabled;
}

void FilterGraphNode::set_enabled(const bool isEnabled)
{
    this->isEnabled = isEnabled;
    emit this->enabled_state_set(isEnabled);

    dynamic_cast<InteractibleNodeGraph*>(this->scene())->update();

    return;
}

void FilterGraphNode::generate_right_click_menu(void)
{
    if (this->rightClickMenu)
    {
        this->rightClickMenu->deleteLater();
    }

    this->rightClickMenu = new QMenu();

    this->rightClickMenu->addAction(this->title);
    this->rightClickMenu->actions().at(0)->setEnabled(false);

    // Add an option to enable/disable this node. Nodes that are disabled will act
    // as passthroughs, and their corresponding filter won't be applied.
    if (this->filterType != filter_node_type_e::gate)
    {
        this->rightClickMenu->addSeparator();

        QAction *enabled = new QAction("Active", this->rightClickMenu);

        enabled->setCheckable(true);
        enabled->setChecked(this->isEnabled);

        connect(this, &FilterGraphNode::enabled_state_set, this, [=](const bool isEnabled)
        {
            enabled->setChecked(isEnabled);
        });

        connect(enabled, &QAction::triggered, this, [=]
        {
            this->set_enabled(!this->isEnabled);
            kd_recalculate_filter_graph_chains();
        });

        this->rightClickMenu->addAction(enabled);
    }

    // Add options to change the node's color.
    if (!this->background_color_list().empty())
    {
        QMenu *colorMenu = new QMenu("Background", this->rightClickMenu);

        for (const auto &color : this->backgroundColorList)
        {
            QAction *colorAction = new QAction(color, colorMenu);
            colorMenu->addAction(colorAction);

            connect(colorAction, &QAction::triggered, this, [=]
            {
                this->set_background_color(color);
            });
        }

        this->rightClickMenu->addMenu(colorMenu);
    }

    // Add the option to delete this node.
    {
        this->rightClickMenu->addSeparator();

        connect(this->rightClickMenu->addAction("Remove"), &QAction::triggered, this, [=]
        {
            dynamic_cast<InteractibleNodeGraph*>(this->scene())->remove_node(this);
        });
    }

    return;
}

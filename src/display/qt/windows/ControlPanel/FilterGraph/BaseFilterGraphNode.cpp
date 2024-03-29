/*
 * 2019 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 * 
 */

#include <QStyleOptionGraphicsItem>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include "filter/filter.h"
#include "filter/abstract_filter.h"
#include "common/globals.h"
#include "common/assert.h"
#include "display/qt/windows/ControlPanel/FilterGraph/BaseFilterGraphNode.h"
#include "display/qt/widgets/InteractibleNodeGraph.h"

BaseFilterGraphNode::BaseFilterGraphNode(
    const filter_node_type_e type,
    const QString title,
    const unsigned width,
    const unsigned height
) :
    InteractibleNodeGraphNode(title, width, height),
    filterType(type)
{
    return;
}

BaseFilterGraphNode::~BaseFilterGraphNode()
{
    kf_delete_filter_instance(this->associatedFilter);

    this->rightClickMenu->close();
    this->rightClickMenu->deleteLater();

    return;
}

QRectF BaseFilterGraphNode::boundingRect(void) const
{
    const int margin = 7;

    return QRectF(
        -margin,
        -margin,
        (this->width + (margin * 2)),
        (this->height + (margin * 2))
    );
}

void BaseFilterGraphNode::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    (void)widget;

    const unsigned borderRadius = 0;
    const unsigned titleBarHeight = 24;
    const QRect titleBarRect = QRect(4, 4, (this->width - 8), (titleBarHeight + 4));
    const QRect titleBarTextRect = QRect(12, 4, (this->width - 18), (titleBarHeight + 4));

    // Draw the node's body.
    {
        // Background.
        {
            const bool isSelected = (option->state & QStyle::State_Selected);

            // Node's background.
            painter->setPen(QPen((isSelected? "#e0e0e0" : this->current_background_color()), 2, Qt::SolidLine));
            painter->setBrush(QBrush(QColor(69, 69, 69, 215)));
            painter->drawRoundedRect(QRect(0, 0, this->width, this->height), borderRadius, borderRadius);

            // Title bar's background.
            painter->setPen(QPen(QColor("transparent"), 1, Qt::SolidLine));
            painter->setBrush(QBrush(this->current_background_color(), (this->is_enabled()? Qt::SolidPattern : Qt::BDiagPattern)));
            painter->drawRoundedRect(titleBarRect, borderRadius, borderRadius);
        }

        // Connection points.
        for (const auto &edge: this->edges)
        {
            painter->setPen(QPen(QColor("#d0d0d0"), 1.5, Qt::SolidLine));
            painter->setBrush(QBrush(QColor("#151515")));
            painter->drawEllipse(edge.rect.center(), 7, 7);
        }
    }

    // Draw the node's title.
    {
        const QColor color = (
            ((this->backgroundColor == "Yellow") && this->is_enabled())
            ? "black"
            : (this->associatedFilter->uuid() == KF_PLACEHOLDER_FILTER->uuid())
                ? "red"
                : this->is_enabled()
                    ? "white"
                    : "lightgray"
        );
        const QString elidedTitle = QFontMetrics(painter->font()).elidedText(this->title, Qt::ElideRight, titleBarTextRect.width());
        painter->setPen(color);
        painter->drawText(titleBarTextRect, (Qt::AlignLeft | Qt::AlignVCenter), elidedTitle);
    }

    return;
}

void BaseFilterGraphNode::set_background_color(const QString &colorName)
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

const QList<QString>& BaseFilterGraphNode::background_color_list(void)
{
    return this->backgroundColorList;
}

const QString& BaseFilterGraphNode::current_background_color_name(void)
{
    return this->backgroundColor;
}

const QColor BaseFilterGraphNode::current_background_color(void)
{
    const QString currentColor = this->backgroundColor.toLower();

    if (currentColor == "blue") return "#157efd";
    else if (currentColor == "black") return "black";
    else if (currentColor == "cyan") return "#1d918f";
    else if (currentColor == "green") return "#39a535";
    else if (currentColor == "magenta") return "#b832b1";
    else if (currentColor == "red") return "#d52f3e";
    else if (currentColor == "yellow") return "#ffdc00";

    return this->backgroundColor;
}

bool BaseFilterGraphNode::is_enabled(void) const
{
    return this->isEnabled;
}

void BaseFilterGraphNode::set_enabled(const bool isEnabled)
{
    this->isEnabled = isEnabled;
    emit this->enabled_state_set(isEnabled);

    dynamic_cast<InteractibleNodeGraph*>(this->scene())->update();

    return;
}

void BaseFilterGraphNode::generate_right_click_menu(void)
{
    k_assert(this->associatedFilter, "No filter associated with this graph node.");

    if (this->rightClickMenu)
    {
        this->rightClickMenu->deleteLater();
    }

    this->rightClickMenu = new QMenu();

    this->rightClickMenu->addAction(QString::fromStdString(this->associatedFilter->name()));
    this->rightClickMenu->actions().at(0)->setEnabled(false);
    this->rightClickMenu->addSeparator();

    // Add an option to enable/disable this node. Nodes that are disabled will act
    // as passthroughs, and their corresponding filter won't be applied.
    if (this->filterType == filter_node_type_e::filter)
    {
        QAction *enabled = new QAction("Active", this->rightClickMenu);

        enabled->setCheckable(true);
        enabled->setChecked(this->isEnabled);

        connect(this, &BaseFilterGraphNode::enabled_state_set, this, [=](const bool isEnabled)
        {
            enabled->setChecked(isEnabled);
        });

        connect(enabled, &QAction::triggered, this, [this]
        {
            this->set_enabled(!this->isEnabled);
            kd_recalculate_filter_graph_chains();
        });

        this->rightClickMenu->addAction(enabled);
    }

    // Add options to change the node's color.
    if (!this->background_color_list().empty())
    {
        QMenu *colorMenu = new QMenu("Color code", this->rightClickMenu);

        for (const auto &color : this->backgroundColorList)
        {
            QAction *colorAction = new QAction(color, colorMenu);
            colorMenu->addAction(colorAction);

            connect(colorAction, &QAction::triggered, this, [color, this]
            {
                this->set_background_color(color);
            });
        }

        this->rightClickMenu->addMenu(colorMenu);
    }

    // Add the option to delete this node.
    {
        this->rightClickMenu->addSeparator();

        connect(this->rightClickMenu->addAction("Delete"), &QAction::triggered, this, [this]
        {
            dynamic_cast<InteractibleNodeGraph*>(this->scene())->remove_node(this);
        });
    }

    return;
}

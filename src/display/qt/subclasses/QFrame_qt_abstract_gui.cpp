/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QDoubleSpinBox>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QWidget>
#include <QFrame>
#include <QLabel>
#include "display/qt/subclasses/QFrame_qt_abstract_gui.h"
#include "filter/filter.h"
#include "filter/abstract_filter.h"

QtAbstractGUI::QtAbstractGUI(const abstract_gui_s &gui) : QFrame()
{
    auto *const widgetLayout = new QFormLayout(this);

    widgetLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    widgetLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);

    if (gui.fields.empty())
    {
        auto *const emptyLabel = new QLabel("No parameters", this);

        emptyLabel->setStyleSheet("font-style: italic;");
        emptyLabel->setAlignment(Qt::AlignCenter);

        widgetLayout->addWidget(emptyLabel);
    }
    else
    {
        for (const auto &field: gui.fields)
        {
            auto *const rowContainer = new QFrame(this);
            auto *const containerLayout = new QHBoxLayout(rowContainer);

            containerLayout->setContentsMargins(0, 0, 0, 0);
            rowContainer->setFrameShape(QFrame::Shape::NoFrame);

            for (auto *component: field.components)
            {
                QWidget *widget = nullptr;

                if (dynamic_cast<abstract_gui::label*>(component))
                {
                    auto *const c = ((abstract_gui::label*)component);
                    auto *const label = qobject_cast<QLabel*>(widget = new QLabel(QString::fromStdString(c->text), this));

                    label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
                    label->setAlignment(Qt::AlignCenter);
                    label->setStyleSheet("margin-bottom: .25em;");
                }
                else if (dynamic_cast<abstract_gui::text_edit*>(component))
                {
                    auto *const c = ((abstract_gui::text_edit*)component);
                    auto *const textEdit = qobject_cast<QPlainTextEdit*>(widget = new QPlainTextEdit(QString::fromStdString(c->text), this));

                    textEdit->setMinimumWidth(150);
                    textEdit->setMaximumHeight(70);
                    textEdit->setTabChangesFocus(true);
                    textEdit->insertPlainText(QString::fromStdString(c->get_text()));

                    connect(textEdit, &QPlainTextEdit::textChanged, [=, this]
                    {
                        QString text = textEdit->toPlainText();

                        textEdit->setProperty("overLengthLimit", (std::size_t(text.length()) > c->maxLength)? "true" : "false");
                        this->style()->polish(textEdit);

                        text.resize(std::min(std::size_t(text.length()), c->maxLength));
                        c->set_text(text.toStdString());

                        emit this->mutated(textEdit);
                    });
                }
                else if (dynamic_cast<abstract_gui::checkbox*>(component))
                {
                    auto *const c = ((abstract_gui::checkbox*)component);
                    auto *const checkbox = qobject_cast<QCheckBox*>(widget = new QCheckBox(this));

                    checkbox->setChecked(c->get_value());
                    checkbox->setText(QString::fromStdString(c->label));
                    checkbox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

                    connect(checkbox, &QCheckBox::toggled, [=, this](const bool isChecked)
                    {
                        c->set_value(isChecked);
                        emit this->mutated(checkbox);
                    });
                }
                else if (dynamic_cast<abstract_gui::combo_box*>(component))
                {
                    auto *const c = ((abstract_gui::combo_box*)component);
                    auto *const combobox = qobject_cast<QComboBox*>(widget = new QComboBox(this));

                    for (const auto &item: c->items)
                    {
                        combobox->addItem(QString::fromStdString(item));
                    }

                    combobox->setCurrentIndex(c->get_value());

                    connect(combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=, this](const int currentIdx)
                    {
                        c->set_value(currentIdx);
                        emit this->mutated(combobox);
                    });
                }
                else if (dynamic_cast<abstract_gui::spinner*>(component))
                {
                    auto *const c = ((abstract_gui::spinner*)component);
                    auto *const spinbox = qobject_cast<QSpinBox*>(widget = new QSpinBox(this));

                    spinbox->setRange(c->minValue, c->maxValue);
                    spinbox->setValue(c->get_value());
                    spinbox->setPrefix(QString::fromStdString(c->prefix));
                    spinbox->setSuffix(QString::fromStdString(c->suffix));
                    spinbox->setButtonSymbols(
                        (c->alignment == abstract_gui::horizontal_align::left)
                        ? QAbstractSpinBox::UpDownArrows
                        : QAbstractSpinBox::NoButtons
                    );

                    switch (c->alignment)
                    {
                        case abstract_gui::horizontal_align::left: spinbox->setAlignment(Qt::AlignLeft); break;
                        case abstract_gui::horizontal_align::right: spinbox->setAlignment(Qt::AlignRight); break;
                        case abstract_gui::horizontal_align::center: spinbox->setAlignment(Qt::AlignHCenter); break;
                        default: k_assert(0, "Unrecognized filter GUI alignment enumerator."); break;
                    }

                    connect(spinbox, QOverload<int>::of(&QSpinBox::valueChanged), [=, this](const double newValue)
                    {
                        c->set_value(newValue);
                        emit this->mutated(spinbox);
                    });
                }
                else if (dynamic_cast<abstract_gui::double_spinner*>(component))
                {
                    auto *const c = ((abstract_gui::double_spinner*)component);
                    auto *const doublespinbox = qobject_cast<QDoubleSpinBox*>(widget = new QDoubleSpinBox(this));

                    doublespinbox->setRange(c->minValue, c->maxValue);
                    doublespinbox->setDecimals(c->numDecimals);
                    doublespinbox->setValue(c->get_value());
                    doublespinbox->setSingleStep(c->stepSize);
                    doublespinbox->setPrefix(QString::fromStdString(c->prefix));
                    doublespinbox->setSuffix(QString::fromStdString(c->suffix));
                    doublespinbox->setButtonSymbols(
                        (c->alignment == abstract_gui::horizontal_align::left)
                        ? QAbstractSpinBox::UpDownArrows
                        : QAbstractSpinBox::NoButtons
                    );

                    switch (c->alignment)
                    {
                        case abstract_gui::horizontal_align::left: doublespinbox->setAlignment(Qt::AlignLeft); break;
                        case abstract_gui::horizontal_align::right: doublespinbox->setAlignment(Qt::AlignRight); break;
                        case abstract_gui::horizontal_align::center: doublespinbox->setAlignment(Qt::AlignHCenter); break;
                        default: k_assert(0, "Unrecognized filter GUI alignment enumerator."); break;
                    }

                    connect(doublespinbox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=, this](const double newValue)
                    {
                        c->set_value(newValue);
                        emit this->mutated(doublespinbox);
                    });
                }
                else
                {
                    k_assert(0, "Unrecognized filter GUI component.");
                }

                k_assert(widget, "Expected the filter GUI widget to have been initialized.");
                widget->setEnabled(component->isInitiallyEnabled);
                component->set_enabled = [widget](const bool isEnabled){widget->setEnabled(isEnabled);};

                containerLayout->addWidget(widget);
            }

            auto *const label = (field.label.empty()? nullptr : new QLabel(QString::fromStdString(field.label), this));

            widgetLayout->addRow(label, rowContainer);
        }
    }

    return;
}
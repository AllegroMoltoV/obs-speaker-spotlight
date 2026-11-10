/*
Voice Spotlight
Copyright (C) 2026 AllegroMoltoV <contact@allegromoltov.jp>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "settings-widgets.h"

#include <obs-module.h>

#include <QColorDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

namespace vspot {

namespace {

QString locale_text(std::string_view key)
{
	return QString::fromUtf8(obs_module_text(std::string(key).c_str()));
}

QColor to_qcolor(const core::Rgb &c)
{
	return QColor(c.r, c.g, c.b);
}

} // namespace

CollapsibleSection::CollapsibleSection(const QString &title, bool expanded, QWidget *parent) : QWidget(parent)
{
	header_ = new QToolButton(this);
	header_->setText(title);
	header_->setCheckable(true);
	header_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	header_->setAutoRaise(true);

	body_ = new QWidget(this);
	form_ = new QFormLayout(body_);
	form_->setContentsMargins(16, 0, 0, 0);
	// ドックが狭いときは、長い項目名を欄の上に置いて折り返す。ドックの最小の幅を抑えるため
	form_->setRowWrapPolicy(QFormLayout::WrapLongRows);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(header_);
	layout->addWidget(body_);

	connect(header_, &QToolButton::toggled, this, &CollapsibleSection::set_expanded);
	set_expanded(expanded);
}

void CollapsibleSection::set_expanded(bool expanded)
{
	header_->blockSignals(true);
	header_->setChecked(expanded);
	header_->blockSignals(false);
	header_->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
	body_->setVisible(expanded);
}

ColorField::ColorField(QWidget *parent) : QWidget(parent)
{
	button_ = new QPushButton(this);
	clear_button_ = new QToolButton(this);
	clear_button_->setText(locale_text("Field.Reset"));
	clear_button_->setToolTip(locale_text("Field.ResetTooltip"));

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(button_, 1);
	layout->addWidget(clear_button_);

	connect(button_, &QPushButton::clicked, this, &ColorField::choose);
	connect(clear_button_, &QToolButton::clicked, this, [this]() {
		set_color(std::nullopt);
		emit changed();
	});
	update_view();
}

void ColorField::set_color(const std::optional<core::Rgb> &color)
{
	color_ = color;
	update_view();
}

void ColorField::choose()
{
	const QColor initial = color_ ? to_qcolor(*color_) : QColor(Qt::white);
	const QColor chosen = QColorDialog::getColor(initial, this);
	if (!chosen.isValid())
		return;
	set_color(core::Rgb{static_cast<std::uint8_t>(chosen.red()), static_cast<std::uint8_t>(chosen.green()),
			    static_cast<std::uint8_t>(chosen.blue())});
	emit changed();
}

void ColorField::update_view()
{
	if (color_) {
		QPixmap swatch(16, 16);
		swatch.fill(to_qcolor(*color_));
		button_->setIcon(QIcon(swatch));
		button_->setText(QString::fromStdString(core::to_hex_color(*color_)));
	} else {
		button_->setIcon(QIcon());
		button_->setText(locale_text("Field.Default"));
	}
	clear_button_->setEnabled(color_.has_value());
}

SliderField::SliderField(int min, int max, const QString &suffix, const QString &special_text, const QString &name,
			 QWidget *parent)
	: QWidget(parent)
{
	slider_ = new QSlider(Qt::Horizontal, this);
	slider_->setRange(min, max);
	slider_->setAccessibleName(name);
	// 狭いドックでもつまみを動かせる幅を残す。足りないときは項目名が上の行に折り返される
	slider_->setMinimumWidth(100);
	spin_ = new QSpinBox(this);
	spin_->setRange(min, max);
	spin_->setSuffix(suffix);
	if (!special_text.isEmpty())
		spin_->setSpecialValueText(special_text);
	spin_->setAccessibleName(name);
	spin_->setButtonSymbols(QAbstractSpinBox::NoButtons);
	spin_->setAlignment(Qt::AlignRight);
	spin_->setMinimumWidth(64);

	auto *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(slider_, 1);
	layout->addWidget(spin_);

	// スライダーと数値の欄を同じ値にそろえ、利用者の操作で変わったときだけ通知する
	connect(slider_, &QSlider::valueChanged, this, [this](int v) {
		if (spin_->value() == v)
			return;
		const QSignalBlocker blocker(spin_);
		spin_->setValue(v);
		emit valueChanged(v);
	});
	connect(spin_, &QSpinBox::valueChanged, this, [this](int v) {
		if (slider_->value() == v)
			return;
		const QSignalBlocker blocker(slider_);
		slider_->setValue(v);
		emit valueChanged(v);
	});
}

void SliderField::setValue(int value)
{
	const QSignalBlocker slider_blocker(slider_);
	const QSignalBlocker spin_blocker(spin_);
	slider_->setValue(value);
	spin_->setValue(value);
}

SettingsPanel::SettingsPanel(const std::vector<core::SectionDef> &sections, QWidget *parent)
	: QWidget(parent),
	  sections_(sections)
{
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	bool first = true;
	for (const auto &section : sections_) {
		// 項目が多いため、最初の区画だけを開いて表示する
		auto *box = new CollapsibleSection(locale_text(section.title_key), first, this);
		first = false;
		for (const auto &item : section.items) {
			auto *label = new QLabel(locale_text(item.label_key), box);
			QWidget *field = create_field(item);
			label->setBuddy(field);
			box->form()->addRow(label, field);
			fields_.back().label = label;
		}
		layout->addWidget(box);
		boxes_.push_back(box);
	}
	connect(this, &SettingsPanel::changed, this, &SettingsPanel::update_enabled);
	update_enabled();
}

void SettingsPanel::update_enabled()
{
	const core::StyleValues current = values();
	for (const auto &field : fields_) {
		const bool effective = core::item_effective(*field.item, sections_, current);
		field.widget->setEnabled(effective);
		if (field.label)
			field.label->setEnabled(effective);
	}
}

QSize SettingsPanel::expanded_minimum_size()
{
	std::vector<bool> states;
	for (auto *box : boxes_) {
		states.push_back(box->expanded());
		box->set_expanded(true);
	}
	const QSize size = minimumSizeHint();
	for (size_t i = 0; i < boxes_.size(); ++i)
		boxes_[i]->set_expanded(states[i]);
	return size;
}

QWidget *SettingsPanel::create_field(const core::ItemDef &item)
{
	const std::string key(item.key);
	Field field;
	QWidget *widget = nullptr;

	switch (item.kind) {
	case core::ItemKind::Number:
	case core::ItemKind::Opacity: {
		// 既定値の無い項目は、最小値の 1 つ下を「既定」として表示する
		const bool has_special = !item.default_value.has_value();
		const int special = item.min - 1;
		const QString suffix =
			item.unit.empty() ? QString()
					  : QStringLiteral(" ") + QString::fromUtf8(std::string(item.unit).c_str());
		auto *slider = new SliderField(has_special ? special : item.min, item.max, suffix,
					       has_special ? locale_text("Field.Default") : QString(),
					       locale_text(item.label_key), this);
		const int initial = has_special ? special : *item.default_value;
		slider->setValue(initial);
		field.read = [slider, key, initial](core::StyleValues &values) {
			if (slider->value() != initial)
				values.numbers[key] = slider->value();
		};
		field.write = [slider, key, initial](const core::StyleValues &values) {
			const auto it = values.numbers.find(key);
			slider->setValue(it != values.numbers.end() ? it->second : initial);
		};
		field.blockers.push_back(slider);
		connect(slider, &SliderField::valueChanged, this, &SettingsPanel::changed);
		widget = slider;
		break;
	}
	case core::ItemKind::Toggle: {
		auto *check = new QCheckBox(this);
		const bool initial = item.default_value.value_or(0) != 0;
		check->setChecked(initial);
		field.read = [check, key, initial](core::StyleValues &values) {
			if (check->isChecked() != initial)
				values.numbers[key] = check->isChecked() ? 1 : 0;
		};
		field.write = [check, key, initial](const core::StyleValues &values) {
			const auto it = values.numbers.find(key);
			check->setChecked(it != values.numbers.end() ? it->second != 0 : initial);
		};
		field.blockers.push_back(check);
		connect(check, &QCheckBox::toggled, this, &SettingsPanel::changed);
		widget = check;
		break;
	}
	case core::ItemKind::Color: {
		auto *color = new ColorField(this);
		field.read = [color, key](core::StyleValues &values) {
			if (const auto c = color->color())
				values.colors[key] = *c;
		};
		field.write = [color, key](const core::StyleValues &values) {
			const auto it = values.colors.find(key);
			color->set_color(it != values.colors.end() ? std::optional<core::Rgb>(it->second)
								   : std::nullopt);
		};
		connect(color, &ColorField::changed, this, &SettingsPanel::changed);
		widget = color;
		break;
	}
	case core::ItemKind::Choice: {
		auto *combo = new QComboBox(this);
		for (const auto &value : item.choices) {
			const std::string label_key = "Choice." + std::string(value);
			combo->addItem(locale_text(label_key), QString::fromUtf8(std::string(value).c_str()));
		}
		field.read = [combo, key](core::StyleValues &values) {
			if (combo->currentIndex() > 0)
				values.texts[key] = combo->currentData().toString().toStdString();
		};
		field.write = [combo, key](const core::StyleValues &values) {
			const auto it = values.texts.find(key);
			const int index = it != values.texts.end() ? combo->findData(QString::fromStdString(it->second))
								   : 0;
			combo->setCurrentIndex(index >= 0 ? index : 0);
		};
		field.blockers.push_back(combo);
		connect(combo, &QComboBox::currentIndexChanged, this, &SettingsPanel::changed);
		widget = combo;
		break;
	}
	case core::ItemKind::Font: {
		// 書体の候補は、利用者の PC に入っている書体だけにする
		auto *combo = new QComboBox(this);
		combo->addItem(locale_text("Field.Default"), QString());
		for (const QString &family : QFontDatabase::families())
			combo->addItem(family, family);
		combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
		combo->setMinimumContentsLength(8);
		field.read = [combo, key](core::StyleValues &values) {
			const QString family = combo->currentData().toString();
			if (!family.isEmpty())
				values.texts[key] = family.toStdString();
		};
		field.write = [combo, key](const core::StyleValues &values) {
			const auto it = values.texts.find(key);
			if (it == values.texts.end()) {
				combo->setCurrentIndex(0);
				return;
			}
			const QString family = QString::fromStdString(it->second);
			int index = combo->findData(family);
			// プリセットを作った PC にだけある書体も、名前を残して選べるようにする
			if (index < 0) {
				combo->addItem(family, family);
				index = combo->count() - 1;
			}
			combo->setCurrentIndex(index);
		};
		field.blockers.push_back(combo);
		connect(combo, &QComboBox::currentIndexChanged, this, &SettingsPanel::changed);
		widget = combo;
		break;
	}
	}

	field.item = &item;
	field.widget = widget;
	fields_.push_back(std::move(field));
	return widget;
}

core::StyleValues SettingsPanel::values() const
{
	core::StyleValues values;
	for (const auto &field : fields_)
		field.read(values);
	return values;
}

void SettingsPanel::set_values(const core::StyleValues &values)
{
	std::vector<QSignalBlocker> blockers;
	for (const auto &field : fields_)
		for (QObject *object : field.blockers)
			blockers.emplace_back(object);
	for (const auto &field : fields_)
		field.write(values);
	update_enabled();
}

} // namespace vspot

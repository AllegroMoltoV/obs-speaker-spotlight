/*
Speaker Spotlight
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

#pragma once

#include "core/style-schema.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QWidget>

#include <functional>
#include <string>
#include <vector>

namespace sspot {

// 見出しを押すと中身を開閉する区画
class CollapsibleSection : public QWidget {
	Q_OBJECT

public:
	CollapsibleSection(const QString &title, bool expanded, QWidget *parent = nullptr);

	QFormLayout *form() const { return form_; }
	void set_expanded(bool expanded);
	bool expanded() const { return !body_->isHidden(); }

private:
	QToolButton *header_{nullptr};
	QWidget *body_{nullptr};
	QFormLayout *form_{nullptr};
};

// 数値をスライダーで選ぶ欄。右に今の値を示す数値の欄を置き、直接の入力もできるようにする
class SliderField : public QWidget {
	Q_OBJECT

public:
	SliderField(int min, int max, const QString &suffix, const QString &special_text, const QString &name,
		    QWidget *parent = nullptr);

	int value() const { return spin_->value(); }
	void setValue(int value);

signals:
	void valueChanged(int value);

private:
	QSlider *slider_{nullptr};
	QSpinBox *spin_{nullptr};
};

// 表の項目から入力欄を作り、値を読み書きする
class SettingsPanel : public QWidget {
	Q_OBJECT

public:
	SettingsPanel(const std::vector<core::SectionDef> &sections, QWidget *parent = nullptr);

	core::StyleValues values() const;
	void set_values(const core::StyleValues &values);

	// すべての区画を開いたときの最小の大きさ。閉じた区画の欄は大きさの計算に入らないため、一時的に開いて求める
	QSize expanded_minimum_size();

signals:
	void changed();

private:
	std::vector<CollapsibleSection *> boxes_;
	// 項目ごとの読み書き。read は値を values に足し、write は values から欄へ写す
	struct Field {
		std::function<void(core::StyleValues &)> read;
		std::function<void(const core::StyleValues &)> write;
		std::vector<QObject *> blockers;
		const core::ItemDef *item = nullptr;
		QWidget *widget = nullptr;
		QWidget *label = nullptr;
	};

	QWidget *create_field(const core::ItemDef &item);
	// 前提が整っていない項目の欄を押せない状態にする
	void update_enabled();

	const std::vector<core::SectionDef> &sections_;
	std::vector<Field> fields_;
};

} // namespace sspot

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

#pragma once

#include "core/portrait.h"

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <vector>

namespace vspot {

// ImageProblem を利用者に示す文言
QString image_problem_text(core::ImageProblem problem);

// 立ち絵の一覧の 1 行
class PortraitRow : public QGroupBox {
	Q_OBJECT

public:
	explicit PortraitRow(QWidget *parent = nullptr);

	core::PortraitEntry entry() const;
	void set_entry(const core::PortraitEntry &entry);
	void set_position(int index, int count);
	QString image_path() const { return image_path_; }
	void set_image_path(const QString &path);

signals:
	void changed();
	void choose_requested(PortraitRow *row);
	void move_requested(PortraitRow *row, int offset);
	void remove_requested(PortraitRow *row);

private:
	QLineEdit *user_id_edit_{nullptr};
	QLabel *image_label_{nullptr};
	QPushButton *choose_button_{nullptr};
	QLineEdit *memo_edit_{nullptr};
	QToolButton *up_button_{nullptr};
	QToolButton *down_button_{nullptr};
	QPushButton *remove_button_{nullptr};
	QString image_path_;
};

// ユーザー ID と画像のファイルの組を何行でも登録する一覧。行の並びが表示の順になる
class PortraitList : public QWidget {
	Q_OBJECT

public:
	explicit PortraitList(QWidget *parent = nullptr);

	std::vector<core::PortraitEntry> entries() const;
	void set_entries(const std::vector<core::PortraitEntry> &entries);

signals:
	void changed();

private:
	PortraitRow *add_row(const core::PortraitEntry &entry);
	void choose_image(PortraitRow *row);
	void move_row(PortraitRow *row, int offset);
	void remove_row(PortraitRow *row);
	// 行の並びを rows_ に合わせ、見出しの番号と移動のボタンを更新する
	void relayout();

	QVBoxLayout *rows_layout_{nullptr};
	QPushButton *add_button_{nullptr};
	std::vector<PortraitRow *> rows_;
};

} // namespace vspot

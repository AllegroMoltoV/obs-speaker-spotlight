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

#include "core/portrait.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <optional>
#include <string>

namespace sspot {

// ImageProblem を利用者に示す文言
QString image_problem_text(core::ImageProblem problem);

// 表示する 1 人のユーザー ID と立ち絵の欄。立ち絵は選んだ時点で縮小して書き出し、元のファイルは以後読まない
class SpeakerForm : public QWidget {
	Q_OBJECT

public:
	explicit SpeakerForm(QWidget *parent = nullptr);

	// 前後の空白を除いたユーザー ID
	std::string user_id() const;
	void set_user_id(const std::string &user_id);
	const std::optional<core::PortraitImage> &portrait() const { return portrait_; }
	void set_portrait(const std::optional<core::PortraitImage> &portrait);

signals:
	void user_id_changed();
	void portrait_changed();

private:
	void choose_image();
	void update_portrait_view();

	QLineEdit *user_id_edit_{nullptr};
	QLabel *portrait_label_{nullptr};
	QPushButton *choose_button_{nullptr};
	QPushButton *clear_button_{nullptr};
	std::optional<core::PortraitImage> portrait_;
	// ファイルを選ぶダイアログを、前に選んだフォルダーで開くため
	QString last_dir_;
};

} // namespace sspot

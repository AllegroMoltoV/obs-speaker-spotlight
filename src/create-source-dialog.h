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

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QString>

namespace sspot {

// 貼り付けた StreamKit の音声ウィジェットの URL を表示するブラウザーソースを、現在のシーンに作るダイアログ
class CreateSourceDialog : public QDialog {
	Q_OBJECT

public:
	// initial_url は URL の欄の初期値
	explicit CreateSourceDialog(const QString &initial_url, QWidget *parent = nullptr);

	// 作ったソースの UUID。作らなかったときは空
	QString created_uuid() const { return created_uuid_; }

private slots:
	void update_state();
	void create_source();

private:
	QString entered_url() const;

	QLineEdit *url_edit_{nullptr};
	QLabel *warning_label_{nullptr};
	QDialogButtonBox *buttons_{nullptr};
	QString created_uuid_;
};

} // namespace sspot

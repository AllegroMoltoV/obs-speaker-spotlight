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

#include <obs.h>

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <string>

namespace vspot {

// 一覧を開くたびに候補を作り直すコンボボックス
class SourceComboBox : public QComboBox {
	Q_OBJECT

public:
	using QComboBox::QComboBox;
	void showPopup() override;

signals:
	void popup_requested();
};

class VoiceSpotlightDock : public QWidget {
	Q_OBJECT

public:
	explicit VoiceSpotlightDock(QWidget *parent = nullptr);

	void refresh_sources();
	void clear_sources();

private slots:
	void on_source_changed();
	void on_apply();
	void on_undo();
	void on_write_css_file();

private:
	// 選択中のソース。参照を 1 つ持つため、呼び出し側が obs_source_release() で返す
	obs_source_t *selected_source() const;
	// generated を選択中のソースの css の管理範囲に書き、元に戻す先を記録する
	void write_managed_css(const std::string &generated);
	void update_view();
	void set_status(const QString &message);

	SourceComboBox *source_combo_{nullptr};
	QLabel *url_warning_label_{nullptr};
	QPushButton *apply_button_{nullptr};
	QPushButton *undo_button_{nullptr};
	QPushButton *write_css_file_button_{nullptr};
	QLabel *status_label_{nullptr};
};

} // namespace vspot

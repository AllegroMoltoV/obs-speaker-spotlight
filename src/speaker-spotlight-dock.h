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

#include <obs.h>

#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>

#include <optional>
#include <string>

namespace sspot {

class CollapsibleSection;
class PortraitList;
class SettingsPanel;

// 一覧を開くたびに候補を作り直すコンボボックス
class SourceComboBox : public QComboBox {
	Q_OBJECT

public:
	using QComboBox::QComboBox;
	void showPopup() override;

signals:
	void popup_requested();
};

class SpeakerSpotlightDock : public QWidget {
	Q_OBJECT

public:
	explicit SpeakerSpotlightDock(QWidget *parent = nullptr);

	void refresh_sources();
	void clear_sources();

private slots:
	void on_source_changed();
	void on_settings_changed();
	void on_create_source();
	void on_apply();
	void on_undo();
	void on_copy_css();
	void on_preset_save();
	void on_preset_load();
	void on_preset_delete();

private:
	// 生成した CSS。作れなかったときは css が空で、error に理由を入れる
	struct BuiltCss {
		std::optional<std::string> css;
		QString error;
	};

	// 選択中のソース。参照を 1 つ持つため、呼び出し側が obs_source_release() で返す
	obs_source_t *selected_source() const;
	// 選択中のソースに保存した設定を欄へ写す。ソースが無ければ既定に戻す
	void load_settings();
	// 欄の設定と立ち絵の画像のファイルから CSS を作る。画像のファイルはその都度読み直して確かめる
	BuiltCss build_css() const;
	// generated を選択中のソースの css の管理範囲に書き、元に戻す先を記録する
	void write_managed_css(const std::string &generated);
	void update_view();
	void update_css_view();
	void refresh_presets(const QString &select_name = {});
	void set_status(const QString &message);

	SourceComboBox *source_combo_{nullptr};
	QPushButton *create_source_button_{nullptr};
	QLabel *url_warning_label_{nullptr};
	CollapsibleSection *portraits_section_{nullptr};
	PortraitList *portraits_{nullptr};
	SettingsPanel *panel_{nullptr};
	QWidget *preset_row_{nullptr};
	QComboBox *preset_combo_{nullptr};
	QPushButton *preset_load_button_{nullptr};
	QPushButton *preset_save_button_{nullptr};
	QPushButton *preset_delete_button_{nullptr};
	QPushButton *apply_button_{nullptr};
	QPushButton *undo_button_{nullptr};
	QPushButton *copy_css_button_{nullptr};
	QLabel *status_label_{nullptr};
	QToolButton *css_toggle_button_{nullptr};
	QPlainTextEdit *css_view_{nullptr};
};

} // namespace sspot

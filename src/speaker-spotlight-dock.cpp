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

#include "speaker-spotlight-dock.h"

#include "core/managed-css.h"
#include "core/portrait.h"
#include "core/streamkit-url.h"
#include "core/voice-style.h"
#include "create-source-dialog.h"
#include "portrait-list.h"
#include "preset-store.h"
#include "settings-widgets.h"
#include "source-state.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

namespace sspot {

namespace {

constexpr const char *BROWSER_SOURCE_ID = "browser_source";

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

// 候補の名前と UUID。UUID で選ぶと、名前を変えられても同じソースを指せる
using SourceEntry = std::pair<QString, QString>;

bool collect_browser_source(void *param, obs_source_t *source)
{
	const char *id = obs_source_get_unversioned_id(source);
	if (!id || std::strcmp(id, BROWSER_SOURCE_ID) != 0)
		return true;

	auto *entries = static_cast<std::vector<SourceEntry> *>(param);
	entries->emplace_back(QString::fromUtf8(obs_source_get_name(source)),
			      QString::fromUtf8(obs_source_get_uuid(source)));
	return true;
}

QString source_name(obs_source_t *source)
{
	return QString::fromUtf8(obs_source_get_name(source));
}

QString entry_problem_text(core::EntryProblem problem)
{
	switch (problem) {
	case core::EntryProblem::None:
		break;
	case core::EntryProblem::MissingUserId:
		return text("Portrait.MissingUserId");
	case core::EntryProblem::InvalidUserId:
		return text("Portrait.InvalidUserId").arg(QString::number(core::MAX_USER_ID_DIGITS));
	case core::EntryProblem::DuplicateUserId:
		return text("Portrait.DuplicateUserId");
	case core::EntryProblem::MissingImage:
		return text("Portrait.MissingImage");
	}
	return {};
}

std::size_t file_size(const QFileInfo &info)
{
	return static_cast<std::size_t>(std::max<qint64>(info.size(), 0));
}

} // namespace

void SourceComboBox::showPopup()
{
	emit popup_requested();
	QComboBox::showPopup();
}

SpeakerSpotlightDock::SpeakerSpotlightDock(QWidget *parent) : QWidget(parent)
{
	auto *content = new QWidget();
	auto *layout = new QVBoxLayout(content);

	// ブラウザーソース
	auto *source_label = new QLabel(text("BrowserSource"), content);
	source_combo_ = new SourceComboBox(content);
	source_combo_->setPlaceholderText(text("NoBrowserSource"));
	source_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	source_label->setBuddy(source_combo_);
	create_source_button_ = new QPushButton(text("CreateSource.Button"), content);
	auto *source_row = new QHBoxLayout();
	source_row->addWidget(source_combo_, 1);
	source_row->addWidget(create_source_button_);
	url_warning_label_ = new QLabel(content);
	url_warning_label_->setWordWrap(true);

	// 立ち絵
	portraits_section_ = new CollapsibleSection(text("Section.Portraits"), true, content);
	portraits_ = new PortraitList(portraits_section_);
	portraits_section_->form()->addRow(portraits_);

	// 見た目の設定
	panel_ = new SettingsPanel(core::voice_sections(), content);

	// プリセット
	preset_row_ = new QWidget(content);
	auto *preset_layout = new QGridLayout(preset_row_);
	preset_layout->setContentsMargins(0, 0, 0, 0);
	auto *preset_label = new QLabel(text("Preset"), preset_row_);
	preset_combo_ = new QComboBox(preset_row_);
	preset_combo_->setPlaceholderText(text("Preset.None"));
	preset_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	preset_label->setBuddy(preset_combo_);
	preset_load_button_ = new QPushButton(text("Preset.Load"), preset_row_);
	preset_save_button_ = new QPushButton(text("Preset.Save"), preset_row_);
	preset_delete_button_ = new QPushButton(text("Preset.Delete"), preset_row_);
	// ドックの最小の幅を抑えるため、ボタンは名前の欄の下の行に置く
	auto *preset_buttons = new QHBoxLayout();
	preset_buttons->addWidget(preset_load_button_);
	preset_buttons->addWidget(preset_save_button_);
	preset_buttons->addWidget(preset_delete_button_);
	preset_layout->addWidget(preset_label, 0, 0);
	preset_layout->addWidget(preset_combo_, 0, 1);
	preset_layout->addLayout(preset_buttons, 1, 1);
	preset_layout->setColumnStretch(1, 1);

	// 適用、元に戻す、クリップボードへ写す
	apply_button_ = new QPushButton(text("Apply"), content);
	undo_button_ = new QPushButton(text("Undo"), content);
	undo_button_->setToolTip(text("Undo.Tooltip"));
	copy_css_button_ = new QPushButton(text("CopyCss"), content);
	auto *action_row = new QHBoxLayout();
	action_row->addStretch();
	action_row->addWidget(apply_button_);
	action_row->addWidget(undo_button_);
	action_row->addWidget(copy_css_button_);

	status_label_ = new QLabel(content);
	status_label_->setWordWrap(true);

	// 生成した CSS。画像の data URI は長いため、本文を省いて表示する
	css_toggle_button_ = new QToolButton(content);
	css_toggle_button_->setText(text("GeneratedCss"));
	css_toggle_button_->setCheckable(true);
	css_toggle_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	css_toggle_button_->setArrowType(Qt::RightArrow);
	css_toggle_button_->setAutoRaise(true);
	css_view_ = new QPlainTextEdit(content);
	css_view_->setReadOnly(true);
	css_view_->setMinimumHeight(120);
	css_view_->setVisible(false);

	// 実機の検証で、用意した CSS を管理範囲へ書くための操作。B2 で削除する
	write_css_file_button_ = new QPushButton(text("WriteCssFile"), content);

	layout->addWidget(source_label);
	layout->addLayout(source_row);
	layout->addWidget(url_warning_label_);
	layout->addWidget(portraits_section_);
	layout->addWidget(panel_);
	layout->addWidget(preset_row_);
	layout->addLayout(action_row);
	layout->addWidget(status_label_);
	layout->addWidget(css_toggle_button_);
	layout->addWidget(css_view_);
	layout->addWidget(write_css_file_button_);
	layout->addStretch();

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidget(content);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->addWidget(scroll);

	// 横にスクロールさせず、欄が右端で切れないよう、ドックの最小の幅を中身の最小の幅に合わせる。
	// 閉じた区画の欄は大きさの計算に入らないため、一時的に開いて求める
	const int margins = layout->contentsMargins().left() + layout->contentsMargins().right();
	const int content_width =
		std::max(content->minimumSizeHint().width(), panel_->expanded_minimum_size().width() + margins);
	setMinimumWidth(content_width + scroll->verticalScrollBar()->sizeHint().width());

	connect(source_combo_, &SourceComboBox::popup_requested, this, &SpeakerSpotlightDock::refresh_sources);
	connect(source_combo_, &QComboBox::currentIndexChanged, this, &SpeakerSpotlightDock::on_source_changed);
	connect(create_source_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_create_source);
	connect(portraits_, &PortraitList::changed, this, &SpeakerSpotlightDock::on_settings_changed);
	connect(panel_, &SettingsPanel::changed, this, &SpeakerSpotlightDock::on_settings_changed);
	connect(apply_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_apply);
	connect(undo_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_undo);
	connect(copy_css_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_copy_css);
	connect(write_css_file_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_write_css_file);
	connect(preset_load_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_load);
	connect(preset_save_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_save);
	connect(preset_delete_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_delete);
	connect(preset_combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
		preset_load_button_->setEnabled(index >= 0);
		preset_delete_button_->setEnabled(index >= 0);
	});
	connect(css_toggle_button_, &QToolButton::toggled, this, [this](bool open) {
		css_toggle_button_->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
		css_view_->setVisible(open);
		update_css_view();
	});

	refresh_presets();
	refresh_sources();
	load_settings();
}

void SpeakerSpotlightDock::refresh_sources()
{
	const QString selected_uuid = source_combo_->currentData().toString();

	std::vector<SourceEntry> entries;
	obs_enum_sources(collect_browser_source, &entries);

	source_combo_->blockSignals(true);
	source_combo_->clear();
	for (const auto &[name, uuid] : entries)
		source_combo_->addItem(name, uuid);
	source_combo_->setCurrentIndex(source_combo_->findData(selected_uuid));
	source_combo_->blockSignals(false);

	// 選んでいたソースが消えたときは、そのソースの設定を欄に残さない
	if (source_combo_->currentData().toString() != selected_uuid)
		load_settings();
	update_view();
}

void SpeakerSpotlightDock::clear_sources()
{
	source_combo_->blockSignals(true);
	source_combo_->clear();
	source_combo_->blockSignals(false);
	load_settings();
	update_view();
}

obs_source_t *SpeakerSpotlightDock::selected_source() const
{
	const QString uuid = source_combo_->currentData().toString();
	if (uuid.isEmpty())
		return nullptr;
	return obs_get_source_by_uuid(uuid.toUtf8().constData());
}

void SpeakerSpotlightDock::load_settings()
{
	obs_source_t *source = selected_source();
	if (source) {
		panel_->set_values(load_style(source));
		portraits_->set_entries(load_portraits(source));
		obs_source_release(source);
	} else {
		panel_->set_values({});
		portraits_->set_entries({});
	}
	update_css_view();
}

void SpeakerSpotlightDock::update_view()
{
	obs_source_t *source = selected_source();
	const bool has_source = source != nullptr;
	portraits_section_->setEnabled(has_source);
	panel_->setEnabled(has_source);
	preset_row_->setEnabled(has_source);
	apply_button_->setEnabled(has_source);
	copy_css_button_->setEnabled(has_source);
	write_css_file_button_->setEnabled(has_source);
	if (!source) {
		url_warning_label_->clear();
		url_warning_label_->setVisible(false);
		undo_button_->setEnabled(false);
		return;
	}

	// StreamKit の URL の形が変わっても使い続けられるよう、警告だけを出して適用は止めない
	const bool streamkit = core::is_streamkit_voice_url(get_source_url(source));
	url_warning_label_->setText(streamkit ? QString() : text("UrlWarning"));
	url_warning_label_->setVisible(!streamkit);
	undo_button_->setEnabled(load_source_state(source).has_previous);
	obs_source_release(source);
}

void SpeakerSpotlightDock::update_css_view()
{
	// 画像のファイルを読み直すため、閉じている間は作らない
	if (!css_toggle_button_->isChecked())
		return;
	const BuiltCss built = build_css();
	if (!built.css) {
		css_view_->setPlainText(built.error);
		return;
	}
	const std::string shown =
		core::abbreviate_data_uris(*built.css, text("GeneratedCss.Abbreviated").toStdString());
	css_view_->setPlainText(QString::fromStdString(shown));
}

void SpeakerSpotlightDock::set_status(const QString &message)
{
	status_label_->setText(message);
}

void SpeakerSpotlightDock::on_source_changed()
{
	set_status({});
	load_settings();
	update_view();
}

void SpeakerSpotlightDock::on_settings_changed()
{
	// 設定は操作のたびにソースへ保存する。css を書き換えるとページが読み込み直されるため、適用はボタンでだけ行う
	obs_source_t *source = selected_source();
	if (source) {
		save_style(source, panel_->values());
		save_portraits(source, portraits_->entries());
		obs_source_release(source);
	}
	update_css_view();
}

void SpeakerSpotlightDock::on_create_source()
{
	CreateSourceDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	refresh_sources();
	const int index = source_combo_->findData(dialog.created_uuid());
	if (index >= 0)
		source_combo_->setCurrentIndex(index);
	set_status(text("CreateSource.Created"));
}

SpeakerSpotlightDock::BuiltCss SpeakerSpotlightDock::build_css() const
{
	const std::vector<core::PortraitEntry> entries = portraits_->entries();
	const core::EntryCheck check = core::check_entries(entries);
	const auto problem_at = [](std::size_t index, const QString &reason) {
		return BuiltCss{std::nullopt, text("Portrait.Problem").arg(QString::number(index + 1), reason)};
	};
	if (check.problem != core::EntryProblem::None)
		return problem_at(check.index, entry_problem_text(check.problem));

	std::vector<core::Portrait> portraits;
	std::vector<std::size_t> sizes;
	for (std::size_t i = 0; i < entries.size(); ++i) {
		const core::PortraitEntry &entry = entries[i];
		if (entry.user_id.empty() && entry.image_path.empty())
			continue;

		const QString path = QString::fromStdString(entry.image_path);
		const QFileInfo info(path);
		if (!info.isFile())
			return problem_at(i, text("Portrait.NotFound").arg(QDir::toNativeSeparators(path)));
		// 選んだ後にファイルが差し替えられていることがあるため、選んだときと同じ検査をやり直す。
		// 大きすぎるファイルを読み込まないよう、読む前に大きさを確かめる
		core::ImageProblem problem = core::check_image_size(file_size(info));
		if (problem != core::ImageProblem::None)
			return problem_at(i, image_problem_text(problem));

		QFile file(path);
		if (!file.open(QIODevice::ReadOnly))
			return problem_at(i, text("Portrait.ReadFailed"));
		const QByteArray bytes = file.readAll();
		const std::string_view view(bytes.constData(), static_cast<std::size_t>(bytes.size()));
		problem = core::check_image(info.fileName().toStdString(), view);
		if (problem == core::ImageProblem::None)
			problem = core::check_total_image_size(sizes, view.size());
		if (problem != core::ImageProblem::None)
			return problem_at(i, image_problem_text(problem));

		sizes.push_back(view.size());
		core::Portrait portrait;
		portrait.user_id = entry.user_id;
		portrait.format = core::detect_image_format(view);
		portrait.base64 = core::base64_encode(view);
		portrait.order = static_cast<int>(portraits.size());
		portrait.memo = entry.memo;
		portraits.push_back(std::move(portrait));
	}
	return {core::generate_css(panel_->values(), portraits), {}};
}

void SpeakerSpotlightDock::write_managed_css(const std::string &generated)
{
	obs_source_t *source = selected_source();
	if (!source) {
		set_status(text("SourceNotFound"));
		refresh_sources();
		return;
	}
	const QString name = source_name(source);
	const SourceState state = load_source_state(source);

	const std::string current = get_source_css(source);
	const auto updated = core::replace_block(current, generated);
	if (!updated) {
		obs_source_release(source);
		set_status(text("Apply.BrokenMarkers"));
		return;
	}
	if (*updated == current) {
		obs_source_release(source);
		set_status(text("Apply.Unchanged").arg(name));
		return;
	}

	// 元に戻す先は、プラグインが最初に書き換える前の css にする
	save_apply_record(source, state.has_previous ? state.previous_css : current, *updated);
	set_source_css(source, *updated);
	obs_log(LOG_INFO, "applied CSS to browser source '%s' (%zu bytes)", name.toUtf8().constData(), updated->size());
	obs_source_release(source);

	set_status(text("Apply.Done").arg(name));
	update_view();
}

void SpeakerSpotlightDock::on_apply()
{
	obs_source_t *source = selected_source();
	if (!source) {
		set_status(text("SourceNotFound"));
		refresh_sources();
		return;
	}
	obs_source_release(source);

	// 見つからない画像や不正な画像があるときは、書き込んだ CSS を変えずに止める
	const BuiltCss built = build_css();
	if (!built.css) {
		set_status(built.error);
		return;
	}
	write_managed_css(*built.css);
	update_css_view();
}

void SpeakerSpotlightDock::on_copy_css()
{
	const BuiltCss built = build_css();
	if (!built.css) {
		set_status(built.error);
		return;
	}
	QApplication::clipboard()->setText(QString::fromStdString(*built.css));
	set_status(text("CopyCss.Done"));
}

void SpeakerSpotlightDock::on_write_css_file()
{
	const QString path =
		QFileDialog::getOpenFileName(this, text("WriteCssFile.Title"), QString(), text("WriteCssFile.Filter"));
	if (path.isEmpty())
		return;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		set_status(text("WriteCssFile.ReadFailed").arg(path));
		return;
	}
	const QByteArray bytes = file.readAll();
	write_managed_css(std::string(bytes.constData(), static_cast<size_t>(bytes.size())));
}

void SpeakerSpotlightDock::on_undo()
{
	obs_source_t *source = selected_source();
	if (!source) {
		set_status(text("SourceNotFound"));
		refresh_sources();
		return;
	}
	const QString name = source_name(source);
	const SourceState state = load_source_state(source);
	const std::string current = get_source_css(source);

	switch (core::decide_undo(state.has_previous, current, state.applied_css)) {
	case core::UndoDecision::Nothing:
		obs_source_release(source);
		set_status(text("Undo.Nothing"));
		update_view();
		return;
	case core::UndoDecision::RestoreWithConfirm:
		if (QMessageBox::question(this, text("DockTitle"), text("Undo.ConfirmEdited")) != QMessageBox::Yes) {
			obs_source_release(source);
			return;
		}
		break;
	case core::UndoDecision::Restore:
		break;
	}

	// 適用はボタンで行うため、ドックの設定と書き込んだ css は一致するとは限らない。
	// 入力した立ち絵の組を失わないよう、戻すのは css だけにして、設定は残す
	clear_apply_record(source);
	set_source_css(source, state.previous_css);
	obs_log(LOG_INFO, "restored previous CSS of browser source '%s'", name.toUtf8().constData());
	obs_source_release(source);

	set_status(text("Undo.Done").arg(name));
	update_view();
}

void SpeakerSpotlightDock::refresh_presets(const QString &select_name)
{
	const QSignalBlocker blocker(preset_combo_);
	preset_combo_->clear();
	for (const auto &preset : load_presets())
		preset_combo_->addItem(QString::fromStdString(preset.name));
	preset_combo_->setCurrentIndex(select_name.isEmpty() ? -1 : preset_combo_->findText(select_name));
	preset_load_button_->setEnabled(preset_combo_->currentIndex() >= 0);
	preset_delete_button_->setEnabled(preset_combo_->currentIndex() >= 0);
}

void SpeakerSpotlightDock::on_preset_save()
{
	bool ok = false;
	const QString name = QInputDialog::getText(this, text("Preset.SaveTitle"), text("Preset.SaveName"),
						   QLineEdit::Normal, preset_combo_->currentText(), &ok)
				     .trimmed();
	if (!ok || name.isEmpty())
		return;

	std::vector<Preset> presets = load_presets();
	const std::string key = name.toStdString();
	auto it = std::find_if(presets.begin(), presets.end(), [&](const Preset &p) { return p.name == key; });
	if (it != presets.end()) {
		if (QMessageBox::question(this, text("Preset.SaveTitle"), text("Preset.Overwrite").arg(name)) !=
		    QMessageBox::Yes)
			return;
		it->values = panel_->values();
		it->portraits = portraits_->entries();
	} else {
		presets.push_back(Preset{key, panel_->values(), portraits_->entries()});
	}
	if (!save_presets(presets)) {
		set_status(text("Preset.SaveFailed"));
		return;
	}
	refresh_presets(name);
	set_status(text("Preset.Saved").arg(name));
}

void SpeakerSpotlightDock::on_preset_load()
{
	if (preset_combo_->currentIndex() < 0)
		return;
	const QString name = preset_combo_->currentText();
	for (const auto &preset : load_presets()) {
		if (preset.name != name.toStdString())
			continue;
		// 欄に写したうえで、設定を変えたときと同じ経路でソースへ保存する
		panel_->set_values(preset.values);
		portraits_->set_entries(preset.portraits);
		on_settings_changed();
		set_status(text("Preset.Loaded").arg(name));
		return;
	}
	set_status(text("Preset.NotFound"));
	refresh_presets();
}

void SpeakerSpotlightDock::on_preset_delete()
{
	if (preset_combo_->currentIndex() < 0)
		return;
	const QString name = preset_combo_->currentText();
	if (QMessageBox::question(this, text("Preset"), text("Preset.ConfirmDelete").arg(name)) != QMessageBox::Yes)
		return;
	std::vector<Preset> presets = load_presets();
	presets.erase(std::remove_if(presets.begin(), presets.end(),
				     [&](const Preset &p) { return p.name == name.toStdString(); }),
		      presets.end());
	if (!save_presets(presets)) {
		set_status(text("Preset.SaveFailed"));
		return;
	}
	refresh_presets();
	set_status(text("Preset.Deleted").arg(name));
}

} // namespace sspot

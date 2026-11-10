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

#include "voice-spotlight-dock.h"

#include "core/managed-css.h"
#include "core/streamkit-url.h"
#include "core/voice-style.h"
#include "source-state.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>

#include <cstring>
#include <utility>
#include <vector>

namespace vspot {

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

} // namespace

void SourceComboBox::showPopup()
{
	emit popup_requested();
	QComboBox::showPopup();
}

VoiceSpotlightDock::VoiceSpotlightDock(QWidget *parent) : QWidget(parent)
{
	auto *content = new QWidget();
	auto *layout = new QVBoxLayout(content);

	// ブラウザーソース
	auto *source_label = new QLabel(text("BrowserSource"), content);
	source_combo_ = new SourceComboBox(content);
	source_combo_->setPlaceholderText(text("NoBrowserSource"));
	source_combo_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
	source_label->setBuddy(source_combo_);
	url_warning_label_ = new QLabel(content);
	url_warning_label_->setWordWrap(true);

	// 適用と元に戻す
	apply_button_ = new QPushButton(text("Apply"), content);
	undo_button_ = new QPushButton(text("Undo"), content);
	undo_button_->setToolTip(text("Undo.Tooltip"));
	auto *action_row = new QHBoxLayout();
	action_row->addStretch();
	action_row->addWidget(apply_button_);
	action_row->addWidget(undo_button_);

	// 実機の検証で、用意した CSS を管理範囲へ書くための操作。機能の実装の後に削除する
	write_css_file_button_ = new QPushButton(text("WriteCssFile"), content);

	status_label_ = new QLabel(content);
	status_label_->setWordWrap(true);

	layout->addWidget(source_label);
	layout->addWidget(source_combo_);
	layout->addWidget(url_warning_label_);
	layout->addLayout(action_row);
	layout->addWidget(write_css_file_button_);
	layout->addWidget(status_label_);
	layout->addStretch();

	auto *scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidget(content);
	auto *outer = new QVBoxLayout(this);
	outer->setContentsMargins(0, 0, 0, 0);
	outer->addWidget(scroll);

	connect(source_combo_, &SourceComboBox::popup_requested, this, &VoiceSpotlightDock::refresh_sources);
	connect(source_combo_, &QComboBox::currentIndexChanged, this, &VoiceSpotlightDock::on_source_changed);
	connect(apply_button_, &QPushButton::clicked, this, &VoiceSpotlightDock::on_apply);
	connect(undo_button_, &QPushButton::clicked, this, &VoiceSpotlightDock::on_undo);
	connect(write_css_file_button_, &QPushButton::clicked, this, &VoiceSpotlightDock::on_write_css_file);

	refresh_sources();
}

void VoiceSpotlightDock::refresh_sources()
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

	update_view();
}

void VoiceSpotlightDock::clear_sources()
{
	source_combo_->blockSignals(true);
	source_combo_->clear();
	source_combo_->blockSignals(false);
	update_view();
}

obs_source_t *VoiceSpotlightDock::selected_source() const
{
	const QString uuid = source_combo_->currentData().toString();
	if (uuid.isEmpty())
		return nullptr;
	return obs_get_source_by_uuid(uuid.toUtf8().constData());
}

void VoiceSpotlightDock::update_view()
{
	obs_source_t *source = selected_source();
	if (!source) {
		url_warning_label_->clear();
		url_warning_label_->setVisible(false);
		apply_button_->setEnabled(false);
		undo_button_->setEnabled(false);
		write_css_file_button_->setEnabled(false);
		return;
	}

	// StreamKit の URL の形が変わっても使い続けられるよう、警告だけを出して適用は止めない
	const bool streamkit = core::is_streamkit_voice_url(get_source_url(source));
	url_warning_label_->setText(streamkit ? QString() : text("UrlWarning"));
	url_warning_label_->setVisible(!streamkit);
	apply_button_->setEnabled(true);
	write_css_file_button_->setEnabled(true);
	undo_button_->setEnabled(load_source_state(source).has_previous);
	obs_source_release(source);
}

void VoiceSpotlightDock::set_status(const QString &message)
{
	status_label_->setText(message);
}

void VoiceSpotlightDock::on_source_changed()
{
	set_status({});
	update_view();
}

void VoiceSpotlightDock::write_managed_css(const std::string &generated)
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

void VoiceSpotlightDock::on_apply()
{
	obs_source_t *source = selected_source();
	if (!source) {
		set_status(text("SourceNotFound"));
		refresh_sources();
		return;
	}
	const std::string generated = core::generate_css(load_style(source));
	obs_source_release(source);
	write_managed_css(generated);
}

void VoiceSpotlightDock::on_write_css_file()
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

void VoiceSpotlightDock::on_undo()
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

	clear_apply_record(source);
	save_style(source, core::StyleValues{});
	set_source_css(source, state.previous_css);
	obs_log(LOG_INFO, "restored previous CSS of browser source '%s'", name.toUtf8().constData());
	obs_source_release(source);

	set_status(text("Undo.Done").arg(name));
	update_view();
}

} // namespace vspot

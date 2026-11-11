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

#include "core/streamkit-url.h"
#include "core/voice-style.h"
#include "create-source-dialog.h"
#include "message-box.h"
#include "preset-store.h"
#include "settings-widgets.h"
#include "source-state.h"
#include "speaker-form.h"

#include <obs-module.h>
#include <plugin-support.h>

#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>
#include <cstring>
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

	// 表示する人
	speaker_section_ = new CollapsibleSection(text("Section.Speaker"), true, content);
	speaker_ = new SpeakerForm(speaker_section_);
	speaker_section_->form()->addRow(speaker_);

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

	status_label_ = new QLabel(content);
	status_label_->setWordWrap(true);

	layout->addWidget(source_label);
	layout->addLayout(source_row);
	layout->addWidget(url_warning_label_);
	layout->addWidget(speaker_section_);
	layout->addWidget(panel_);
	layout->addWidget(preset_row_);
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

	// 横にスクロールさせず、欄が右端で切れないよう、ドックの最小の幅を中身の最小の幅に合わせる。
	// 閉じた区画の欄は大きさの計算に入らないため、一時的に開いて求める
	const int margins = layout->contentsMargins().left() + layout->contentsMargins().right();
	const int content_width =
		std::max(content->minimumSizeHint().width(), panel_->expanded_minimum_size().width() + margins);
	setMinimumWidth(content_width + scroll->verticalScrollBar()->sizeHint().width());

	connect(source_combo_, &SourceComboBox::popup_requested, this, &SpeakerSpotlightDock::refresh_sources);
	connect(source_combo_, &QComboBox::currentIndexChanged, this, &SpeakerSpotlightDock::on_source_changed);
	connect(create_source_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_create_source);
	connect(speaker_, &SpeakerForm::user_id_changed, this, &SpeakerSpotlightDock::on_user_id_changed);
	connect(speaker_, &SpeakerForm::portrait_changed, this, &SpeakerSpotlightDock::on_portrait_changed);
	connect(panel_, &SettingsPanel::changed, this, &SpeakerSpotlightDock::on_settings_changed);
	apply_timer_.setSingleShot(true);
	apply_timer_.setInterval(core::APPLY_DELAY_MS);
	connect(&apply_timer_, &QTimer::timeout, this, &SpeakerSpotlightDock::flush_pending);
	connect(preset_load_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_load);
	connect(preset_save_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_save);
	connect(preset_delete_button_, &QPushButton::clicked, this, &SpeakerSpotlightDock::on_preset_delete);
	connect(preset_combo_, &QComboBox::currentIndexChanged, this, [this](int index) {
		preset_load_button_->setEnabled(index >= 0);
		preset_delete_button_->setEnabled(index >= 0);
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

	// 選んでいたソースが消えたときは、反映待ちを書き込み、そのソースの設定を欄に残さない
	if (source_combo_->currentData().toString() != selected_uuid) {
		flush_pending();
		load_settings();
	}
	update_view();
}

void SpeakerSpotlightDock::clear_sources()
{
	flush_pending();
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
	SpeakerSettings settings;
	if (source) {
		settings = load_speaker_settings(source);
		obs_source_release(source);
	}
	panel_->set_values(settings.style);
	speaker_->set_user_id(settings.user_id);
	speaker_->set_portrait(settings.portrait);
}

void SpeakerSpotlightDock::update_view()
{
	obs_source_t *source = selected_source();
	const bool has_source = source != nullptr;
	speaker_section_->setEnabled(has_source);
	panel_->setEnabled(has_source);
	preset_row_->setEnabled(has_source);
	if (!source) {
		url_warning_label_->clear();
		url_warning_label_->setVisible(false);
		return;
	}

	// StreamKit の URL の形が変わっても使い続けられるよう、警告だけを出して反映は止めない
	const bool streamkit = core::is_streamkit_voice_url(get_source_url(source));
	url_warning_label_->setText(streamkit ? QString() : text("UrlWarning"));
	url_warning_label_->setVisible(!streamkit);
	obs_source_release(source);
}

void SpeakerSpotlightDock::set_status(const QString &message)
{
	status_label_->setText(message);
}

void SpeakerSpotlightDock::on_source_changed()
{
	set_status({});
	flush_pending();
	load_settings();
	update_view();
}

void SpeakerSpotlightDock::on_settings_changed()
{
	// 設定は操作のたびにソースへ保存し、css への反映は待ちの後にまとめて行う
	obs_source_t *source = selected_source();
	if (source) {
		save_style(source, panel_->values());
		obs_source_release(source);
		schedule_apply();
	}
}

void SpeakerSpotlightDock::on_user_id_changed()
{
	obs_source_t *source = selected_source();
	if (source) {
		save_user_id(source, speaker_->user_id());
		obs_source_release(source);
		schedule_apply();
	}
}

void SpeakerSpotlightDock::on_portrait_changed()
{
	obs_source_t *source = selected_source();
	if (source) {
		save_portrait(source, speaker_->portrait());
		obs_source_release(source);
		schedule_apply();
	}
}

void SpeakerSpotlightDock::on_create_source()
{
	QString url;
	if (obs_source_t *source = selected_source()) {
		url = QString::fromStdString(get_source_url(source));
		obs_source_release(source);
	}
	CreateSourceDialog dialog(url, this);
	if (dialog.exec() != QDialog::Accepted)
		return;

	refresh_sources();
	const int index = source_combo_->findData(dialog.created_uuid());
	if (index >= 0)
		source_combo_->setCurrentIndex(index);
	set_status(text("CreateSource.Created"));
}

void SpeakerSpotlightDock::apply_to_source(obs_source_t *source)
{
	const QString name = source_name(source);
	const SpeakerSettings settings = load_speaker_settings(source);
	const std::string current = get_source_css(source);
	const core::CssUpdate update = core::update_css(current, settings.style, settings.user_id, settings.portrait);
	switch (update.problem) {
	case core::CssUpdateProblem::None:
		break;
	case core::CssUpdateProblem::BrokenMarkers:
		set_status(text("Apply.BrokenMarkers"));
		return;
	case core::CssUpdateProblem::InvalidUserId:
		set_status(text("Portrait.InvalidUserId").arg(QString::number(core::MAX_USER_ID_DIGITS)));
		return;
	}
	if (!update.changed) {
		set_status(text("Apply.Unchanged").arg(name));
		return;
	}

	set_source_css(source, update.css);
	obs_log(LOG_INFO, "applied CSS to browser source '%s' (%zu bytes)", name.toUtf8().constData(),
		update.css.size());
	set_status(text(settings.user_id.empty() ? "Apply.Removed" : "Apply.Done").arg(name));
}

void SpeakerSpotlightDock::schedule_apply()
{
	const QString uuid = source_combo_->currentData().toString();
	if (uuid.isEmpty())
		return;
	if (const auto previous = pending_.mark(uuid.toStdString()))
		apply_by_uuid(*previous);
	set_status(text("Apply.Pending"));
	apply_timer_.start();
}

void SpeakerSpotlightDock::flush_pending()
{
	apply_timer_.stop();
	if (const auto uuid = pending_.take())
		apply_by_uuid(*uuid);
}

void SpeakerSpotlightDock::apply_by_uuid(const std::string &uuid)
{
	obs_source_t *source = obs_get_source_by_uuid(uuid.c_str());
	if (!source)
		return;
	if (!obs_source_removed(source))
		apply_to_source(source);
	obs_source_release(source);
}

bool SpeakerSpotlightDock::eventFilter(QObject *watched, QEvent *event)
{
	// 閉じる操作が取り消されても、書き込みが早まるだけで害はない
	if (event->type() == QEvent::Close)
		flush_pending();
	return QWidget::eventFilter(watched, event);
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
	QInputDialog dialog(this);
	dialog.setWindowTitle(text("Preset.SaveTitle"));
	dialog.setLabelText(text("Preset.SaveName"));
	dialog.setTextValue(preset_combo_->currentText());
	dialog.setOkButtonText(text("Dialog.OK"));
	dialog.setCancelButtonText(text("Dialog.Cancel"));
	if (dialog.exec() != QDialog::Accepted)
		return;
	const QString name = dialog.textValue().trimmed();
	if (name.isEmpty())
		return;

	std::vector<Preset> presets = load_presets();
	const std::string key = name.toStdString();
	auto it = std::find_if(presets.begin(), presets.end(), [&](const Preset &p) { return p.name == key; });
	if (it != presets.end()) {
		if (!ask_yes_no(this, text("Preset.SaveTitle"), text("Preset.Overwrite").arg(name)))
			return;
		it->values = panel_->values();
	} else {
		presets.push_back(Preset{key, panel_->values()});
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
	if (!ask_yes_no(this, text("Preset"), text("Preset.ConfirmDelete").arg(name)))
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

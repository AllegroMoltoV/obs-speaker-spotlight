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

#include "create-source-dialog.h"

#include "core/streamkit-url.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>

#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace vspot {

namespace {

constexpr int SOURCE_WIDTH = 400;
constexpr int SOURCE_HEIGHT = 600;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

bool source_name_exists(const QString &name)
{
	obs_source_t *existing = obs_get_source_by_name(name.toUtf8().constData());
	if (!existing)
		return false;
	obs_source_release(existing);
	return true;
}

// OBS のソースの名前は重複させられないため、番号を付けて空いている名前を探す
QString unique_source_name(const QString &base)
{
	if (!source_name_exists(base))
		return base;
	for (int n = 2;; ++n) {
		const QString candidate = QStringLiteral("%1 %2").arg(base).arg(n);
		if (!source_name_exists(candidate))
			return candidate;
	}
}

} // namespace

CreateSourceDialog::CreateSourceDialog(QWidget *parent) : QDialog(parent)
{
	setWindowTitle(text("CreateSource.Title"));

	url_edit_ = new QLineEdit(this);
	url_edit_->setPlaceholderText(text("CreateSource.UrlPlaceholder"));

	warning_label_ = new QLabel(this);
	warning_label_->setWordWrap(true);

	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	buttons_->button(QDialogButtonBox::Ok)->setText(text("CreateSource.Create"));

	auto *form = new QFormLayout();
	form->addRow(text("CreateSource.Url"), url_edit_);

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(warning_label_);
	layout->addWidget(buttons_);

	connect(url_edit_, &QLineEdit::textChanged, this, &CreateSourceDialog::update_state);
	connect(buttons_, &QDialogButtonBox::accepted, this, &CreateSourceDialog::create_source);
	connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);

	setMinimumWidth(420);
	update_state();
}

QString CreateSourceDialog::entered_url() const
{
	return url_edit_->text().trimmed();
}

void CreateSourceDialog::update_state()
{
	const QString url = entered_url();
	// StreamKit の URL の形が変わっても作れるよう、形が違うときは警告だけを出す
	if (url.isEmpty() || core::is_streamkit_voice_url(url.toStdString()))
		warning_label_->clear();
	else
		warning_label_->setText(text("UrlWarning"));
	buttons_->button(QDialogButtonBox::Ok)->setEnabled(!url.isEmpty());
}

void CreateSourceDialog::create_source()
{
	const QString url = entered_url();
	if (url.isEmpty())
		return;

	obs_source_t *scene_source = obs_frontend_get_current_scene();
	obs_scene_t *scene = scene_source ? obs_scene_from_source(scene_source) : nullptr;
	if (!scene) {
		obs_source_release(scene_source);
		QMessageBox::warning(this, windowTitle(), text("CreateSource.NoScene"));
		return;
	}

	const QString name = unique_source_name(text("CreateSource.Name"));

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", url.toUtf8().constData());
	obs_data_set_int(settings, "width", SOURCE_WIDTH);
	obs_data_set_int(settings, "height", SOURCE_HEIGHT);
	obs_source_t *source = obs_source_create("browser_source", name.toUtf8().constData(), settings, nullptr);
	obs_data_release(settings);

	if (source) {
		obs_scene_add(scene, source);
		created_uuid_ = QString::fromUtf8(obs_source_get_uuid(source));
		obs_log(LOG_INFO, "created browser source '%s'", name.toUtf8().constData());
		obs_source_release(source);
	}
	obs_source_release(scene_source);

	if (created_uuid_.isEmpty()) {
		QMessageBox::warning(this, windowTitle(), text("CreateSource.Failed"));
		return;
	}
	accept();
}

} // namespace vspot

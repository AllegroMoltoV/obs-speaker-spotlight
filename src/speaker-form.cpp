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

#include "speaker-form.h"

#include "message-box.h"
#include "portrait-image.h"

#include <obs-module.h>

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include <algorithm>
#include <cstddef>

namespace sspot {

namespace {

constexpr std::size_t BYTES_PER_MB = 1048576;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

QString embed_problem_text(core::EmbedProblem problem)
{
	switch (problem) {
	case core::EmbedProblem::None:
		break;
	case core::EmbedProblem::EncodeFailed:
		return text("Embed.EncodeFailed");
	case core::EmbedProblem::TooLarge:
		return text("Embed.TooLarge").arg(QString::number(core::MAX_IMAGE_BYTES / BYTES_PER_MB));
	}
	return {};
}

// base64 を戻した後のバイト数
std::size_t decoded_size(const std::string &base64)
{
	std::size_t padding = 0;
	for (auto it = base64.rbegin(); it != base64.rend() && *it == '=' && padding < 2; ++it)
		++padding;
	const std::size_t full = base64.size() / 4 * 3;
	return full > padding ? full - padding : 0;
}

} // namespace

QString image_problem_text(core::ImageProblem problem)
{
	switch (problem) {
	case core::ImageProblem::None:
		break;
	case core::ImageProblem::UnsupportedExtension:
		return text("ImageProblem.UnsupportedExtension");
	case core::ImageProblem::UnknownContent:
		return text("ImageProblem.UnknownContent");
	case core::ImageProblem::ExtensionMismatch:
		return text("ImageProblem.ExtensionMismatch");
	case core::ImageProblem::TooLarge:
		return text("ImageProblem.TooLarge").arg(QString::number(core::MAX_SOURCE_FILE_BYTES / BYTES_PER_MB));
	case core::ImageProblem::TooManyPixels:
		return text("ImageProblem.TooManyPixels");
	case core::ImageProblem::Unreadable:
		return text("Portrait.ReadFailed");
	}
	return {};
}

SpeakerForm::SpeakerForm(QWidget *parent) : QWidget(parent)
{
	user_id_edit_ = new QLineEdit(this);
	// 値は CSS のセレクターに入るため、半角数字だけを受け付ける。貼り付けた値の前後の空白は user_id() で除く
	user_id_edit_->setValidator(new QRegularExpressionValidator(
		QRegularExpression(QStringLiteral("[ \\t]*[0-9]{0,%1}[ \\t]*").arg(core::MAX_USER_ID_DIGITS)),
		user_id_edit_));
	user_id_edit_->setInputMethodHints(Qt::ImhDigitsOnly);

	portrait_label_ = new QLabel(this);
	choose_button_ = new QPushButton(text("Portrait.Choose"), this);
	clear_button_ = new QPushButton(text("Portrait.Clear"), this);
	auto *portrait_row = new QHBoxLayout();
	portrait_row->addWidget(portrait_label_, 1);
	portrait_row->addWidget(choose_button_);
	portrait_row->addWidget(clear_button_);

	auto *form = new QFormLayout(this);
	form->setContentsMargins(0, 0, 0, 0);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);
	form->addRow(text("Portrait.UserId"), user_id_edit_);
	form->addRow(text("Portrait.Image"), portrait_row);

	connect(user_id_edit_, &QLineEdit::textEdited, this, &SpeakerForm::user_id_changed);
	connect(choose_button_, &QPushButton::clicked, this, &SpeakerForm::choose_image);
	connect(clear_button_, &QPushButton::clicked, this, [this]() {
		set_portrait(std::nullopt);
		emit portrait_changed();
	});

	update_portrait_view();
}

std::string SpeakerForm::user_id() const
{
	return user_id_edit_->text().trimmed().toStdString();
}

void SpeakerForm::set_user_id(const std::string &user_id)
{
	user_id_edit_->setText(QString::fromStdString(user_id));
}

void SpeakerForm::set_portrait(const std::optional<core::PortraitImage> &portrait)
{
	portrait_ = portrait;
	update_portrait_view();
}

void SpeakerForm::update_portrait_view()
{
	clear_button_->setEnabled(portrait_.has_value());
	if (!portrait_) {
		portrait_label_->setText(text("Portrait.NoImage"));
		return;
	}
	const std::size_t kb = std::max<std::size_t>((decoded_size(portrait_->base64) + 512) / 1024, 1);
	const QString format = portrait_->format == core::ImageFormat::Webp ? QStringLiteral("WebP")
									    : QStringLiteral("PNG");
	portrait_label_->setText(text("Portrait.Embedded").arg(QString::number(kb), format));
}

void SpeakerForm::choose_image()
{
	const QString path =
		QFileDialog::getOpenFileName(this, text("Portrait.ChooseTitle"), last_dir_, text("Portrait.Filter"));
	if (path.isEmpty())
		return;
	const QFileInfo info(path);
	last_dir_ = info.absolutePath();

	// 大きな画像の縮小と書き出しには数秒かかることがある
	QApplication::setOverrideCursor(Qt::WaitCursor);
	const EmbedResult result = embed_portrait_file(path);
	QApplication::restoreOverrideCursor();

	// 受け付けないときは、前の立ち絵を残す
	const QString reason = result.image_problem != core::ImageProblem::None
				       ? image_problem_text(result.image_problem)
				       : embed_problem_text(result.embed_problem);
	if (!result.portrait) {
		show_warning(this, text("Portrait.ChooseTitle"),
			     text("Portrait.Rejected").arg(info.fileName(), reason));
		return;
	}
	set_portrait(result.portrait);
	emit portrait_changed();
}

} // namespace sspot

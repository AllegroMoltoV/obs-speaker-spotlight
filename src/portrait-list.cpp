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

#include "portrait-list.h"

#include <obs-module.h>

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include <algorithm>
#include <string>
#include <string_view>

namespace vspot {

namespace {

constexpr std::size_t BYTES_PER_MB = 1048576;

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
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
		return text("ImageProblem.TooLarge").arg(QString::number(core::MAX_IMAGE_BYTES / BYTES_PER_MB));
	case core::ImageProblem::TotalTooLarge:
		return text("ImageProblem.TotalTooLarge")
			.arg(QString::number(core::MAX_TOTAL_IMAGE_BYTES / BYTES_PER_MB));
	}
	return {};
}

PortraitRow::PortraitRow(QWidget *parent) : QGroupBox(parent)
{
	user_id_edit_ = new QLineEdit(this);
	// 値は CSS のセレクターに入るため、半角数字だけを受け付ける。貼り付けた値の前後の空白は entry() で除く
	user_id_edit_->setValidator(new QRegularExpressionValidator(
		QRegularExpression(QStringLiteral("[ \\t]*[0-9]{0,%1}[ \\t]*").arg(core::MAX_USER_ID_DIGITS)),
		user_id_edit_));
	user_id_edit_->setInputMethodHints(Qt::ImhDigitsOnly);

	image_label_ = new QLabel(this);
	// 長いファイル名でドックの幅が広がらないよう、欄の幅に収まらない分は切って表示する
	image_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	choose_button_ = new QPushButton(text("Portrait.Choose"), this);
	auto *image_row = new QHBoxLayout();
	image_row->addWidget(image_label_, 1);
	image_row->addWidget(choose_button_);

	memo_edit_ = new QLineEdit(this);

	up_button_ = new QToolButton(this);
	up_button_->setArrowType(Qt::UpArrow);
	up_button_->setToolTip(text("Portrait.MoveUp"));
	up_button_->setAccessibleName(text("Portrait.MoveUp"));
	down_button_ = new QToolButton(this);
	down_button_->setArrowType(Qt::DownArrow);
	down_button_->setToolTip(text("Portrait.MoveDown"));
	down_button_->setAccessibleName(text("Portrait.MoveDown"));
	remove_button_ = new QPushButton(text("Portrait.Remove"), this);
	auto *button_row = new QHBoxLayout();
	button_row->addStretch();
	button_row->addWidget(up_button_);
	button_row->addWidget(down_button_);
	button_row->addWidget(remove_button_);

	auto *form = new QFormLayout(this);
	form->setRowWrapPolicy(QFormLayout::WrapLongRows);
	form->addRow(text("Portrait.UserId"), user_id_edit_);
	form->addRow(text("Portrait.Image"), image_row);
	form->addRow(text("Portrait.Memo"), memo_edit_);
	form->addRow(button_row);

	connect(user_id_edit_, &QLineEdit::textEdited, this, &PortraitRow::changed);
	connect(memo_edit_, &QLineEdit::textEdited, this, &PortraitRow::changed);
	connect(choose_button_, &QPushButton::clicked, this, [this]() { emit choose_requested(this); });
	connect(up_button_, &QToolButton::clicked, this, [this]() { emit move_requested(this, -1); });
	connect(down_button_, &QToolButton::clicked, this, [this]() { emit move_requested(this, 1); });
	connect(remove_button_, &QPushButton::clicked, this, [this]() { emit remove_requested(this); });

	set_image_path({});
}

core::PortraitEntry PortraitRow::entry() const
{
	return {user_id_edit_->text().trimmed().toStdString(), image_path_.toStdString(),
		memo_edit_->text().trimmed().toStdString()};
}

void PortraitRow::set_entry(const core::PortraitEntry &entry)
{
	user_id_edit_->setText(QString::fromStdString(entry.user_id));
	memo_edit_->setText(QString::fromStdString(entry.memo));
	set_image_path(QString::fromStdString(entry.image_path));
}

void PortraitRow::set_image_path(const QString &path)
{
	image_path_ = path;
	image_label_->setText(path.isEmpty() ? text("Portrait.NoImage") : QFileInfo(path).fileName());
	image_label_->setToolTip(QDir::toNativeSeparators(path));
}

void PortraitRow::set_position(int index, int count)
{
	setTitle(text("Portrait.Title").arg(index + 1));
	up_button_->setEnabled(index > 0);
	down_button_->setEnabled(index + 1 < count);
}

PortraitList::PortraitList(QWidget *parent) : QWidget(parent)
{
	rows_layout_ = new QVBoxLayout();
	rows_layout_->setContentsMargins(0, 0, 0, 0);
	add_button_ = new QPushButton(text("Portrait.Add"), this);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(rows_layout_);
	layout->addWidget(add_button_);

	connect(add_button_, &QPushButton::clicked, this, [this]() {
		add_row({});
		relayout();
		emit changed();
	});
}

std::vector<core::PortraitEntry> PortraitList::entries() const
{
	std::vector<core::PortraitEntry> result;
	result.reserve(rows_.size());
	for (const PortraitRow *row : rows_)
		result.push_back(row->entry());
	return result;
}

void PortraitList::set_entries(const std::vector<core::PortraitEntry> &entries)
{
	for (PortraitRow *row : rows_)
		delete row;
	rows_.clear();
	for (const auto &entry : entries)
		add_row(entry);
	relayout();
}

PortraitRow *PortraitList::add_row(const core::PortraitEntry &entry)
{
	auto *row = new PortraitRow(this);
	row->set_entry(entry);
	connect(row, &PortraitRow::changed, this, &PortraitList::changed);
	connect(row, &PortraitRow::choose_requested, this, &PortraitList::choose_image);
	connect(row, &PortraitRow::move_requested, this, &PortraitList::move_row);
	connect(row, &PortraitRow::remove_requested, this, &PortraitList::remove_row);
	rows_.push_back(row);
	return row;
}

void PortraitList::relayout()
{
	for (PortraitRow *row : rows_)
		rows_layout_->removeWidget(row);
	const int count = static_cast<int>(rows_.size());
	for (int i = 0; i < count; ++i) {
		PortraitRow *row = rows_[static_cast<size_t>(i)];
		rows_layout_->addWidget(row);
		row->set_position(i, count);
	}
}

void PortraitList::move_row(PortraitRow *row, int offset)
{
	const auto it = std::find(rows_.begin(), rows_.end(), row);
	if (it == rows_.end())
		return;
	const auto index = it - rows_.begin();
	const auto target = index + offset;
	if (target < 0 || target >= static_cast<decltype(target)>(rows_.size()))
		return;
	std::iter_swap(it, rows_.begin() + target);
	relayout();
	emit changed();
}

void PortraitList::remove_row(PortraitRow *row)
{
	const auto it = std::find(rows_.begin(), rows_.end(), row);
	if (it == rows_.end())
		return;
	rows_.erase(it);
	// ボタンのクリックの処理の途中で行が消えないよう、削除はイベントループに戻ってから行う
	rows_layout_->removeWidget(row);
	row->hide();
	row->deleteLater();
	relayout();
	emit changed();
}

void PortraitList::choose_image(PortraitRow *row)
{
	const QString current = row->image_path();
	const QString path = QFileDialog::getOpenFileName(
		this, text("Portrait.ChooseTitle"), current.isEmpty() ? QString() : QFileInfo(current).absolutePath(),
		text("Portrait.Filter"));
	if (path.isEmpty())
		return;

	const QFileInfo info(path);
	const QString name = info.fileName();
	const auto reject = [&](const QString &reason) {
		QMessageBox::warning(this, text("Portrait.ChooseTitle"), text("Portrait.Rejected").arg(name, reason));
	};

	// 大きすぎるファイルを読み込まないよう、読む前に大きさを確かめる
	const auto size = static_cast<std::size_t>(std::max<qint64>(info.size(), 0));
	core::ImageProblem problem = core::check_image_size(size);
	if (problem == core::ImageProblem::None) {
		// 差し替えのときは、差し替える前の画像を合計から除く
		std::vector<std::size_t> others;
		for (const PortraitRow *other : rows_) {
			if (other == row || other->image_path().isEmpty())
				continue;
			const QFileInfo other_info(other->image_path());
			if (other_info.exists())
				others.push_back(static_cast<std::size_t>(std::max<qint64>(other_info.size(), 0)));
		}
		problem = core::check_total_image_size(others, size);
	}
	if (problem != core::ImageProblem::None) {
		reject(image_problem_text(problem));
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		reject(text("Portrait.ReadFailed"));
		return;
	}
	const QByteArray bytes = file.readAll();
	problem = core::check_image(name.toStdString(),
				    std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
	if (problem != core::ImageProblem::None) {
		reject(image_problem_text(problem));
		return;
	}

	row->set_image_path(path);
	emit changed();
}

} // namespace vspot

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

#include "message-box.h"

#include <obs-module.h>

#include <QMessageBox>
#include <QPushButton>

namespace sspot {

namespace {

QString text(const char *key)
{
	return QString::fromUtf8(obs_module_text(key));
}

} // namespace

bool ask_yes_no(QWidget *parent, const QString &title, const QString &message)
{
	QMessageBox box(QMessageBox::Question, title, message, QMessageBox::NoButton, parent);
	QPushButton *yes = box.addButton(text("Dialog.Yes"), QMessageBox::YesRole);
	QPushButton *no = box.addButton(text("Dialog.No"), QMessageBox::NoRole);
	box.setDefaultButton(yes);
	box.setEscapeButton(no);
	box.exec();
	return box.clickedButton() == yes;
}

void show_warning(QWidget *parent, const QString &title, const QString &message)
{
	QMessageBox box(QMessageBox::Warning, title, message, QMessageBox::NoButton, parent);
	QPushButton *ok = box.addButton(text("Dialog.OK"), QMessageBox::AcceptRole);
	box.setDefaultButton(ok);
	box.setEscapeButton(ok);
	box.exec();
}

} // namespace sspot
